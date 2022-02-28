#include <Importer.hpp>
#include <pbrmaterial.h>
#include <set>
#include <scene.h>
#include <postprocess.h>
#include <matrix4x4.h>
#include <cimport.h>
#include "Spectrum.h"
#include "Context.h"
#include "ResourceManager.h"
#include "HWMesh.h"
#include "IO.h"
#include "Logger.h"
#include "Common.h"
#include "HWTexture.h"
#include "HWMaterial.h"
#include "FilePaths.h"
#include "HWLight.h"
#include "ModelLoader.h"

namespace RayTrace
{
    
    std::string  GetTexturePath(const std::string& _path, const std::vector<std::string>& _files) {
        if (_path.size() < 5)
            return std::string("");
        for (const auto& file : _files)
            if (EndsWith(file, _path) && Exists(file))
                return file;
        return std::string("");
    };




    std::string GetPathAssimp(aiTextureType tex_type, const aiMaterial* ai_material) 
    {

        if (ai_material->GetTextureCount(tex_type) > 0) {

            aiString ai_filename;
            std::string filename;

            if (ai_material->GetTexture(tex_type, 0, &ai_filename) == AI_SUCCESS) {
                return ai_filename.C_Str();
            }
        }

        return "";
    }

    std::string GetPathAssimp(const std::string& key, aiTextureType type, const  aiMaterial* ai_material) {

        aiString ai_filename;

        if (ai_material->Get(key.c_str(), type, 0, ai_filename) == AI_SUCCESS)
            return ai_filename.C_Str();
        else
            return "";
    }

    Vector4f GetVecFactorAssimp(const std::string& key, uint32_t type, uint32_t slot, 
        const  aiMaterial* ai_material, const Vector4f& _default = { 1.0f, 1.0f, 1.0f, 1.0f } ) {

        aiColor4D ai_color;

        if (ai_material->Get(key.c_str(), type, slot, ai_color) == AI_SUCCESS)
            return  Vector4f{ ai_color.r, ai_color.g, ai_color.b, ai_color.a };
        else
            return _default;
    }

    float GetNumberFactorAssimp(const std::string& key, uint32_t type, uint32_t slot, const  
        aiMaterial* ai_material, float _default = 1.0f) {

        float value;

        if (ai_material->Get(key.c_str(), type, slot, value) == AI_SUCCESS)
            return value;
        else
            return _default;
    }



    struct CombinedData
    {
        enum eTypes
        {
            TYPE_AO,
            TYPE_ROUGH,
            TYPE_METAL,
            TYPE_COMBINED
        };
        
        
        
        CombinedData(Context* _pContext, const std::vector<std::string>& _files, const aiMaterial* _pMat)
        {
            
            bool firstTime = true;
            for (int i = 0; i < 4; ++i) {
                const auto path = GetPathAssimp(m_types[i], _pMat);
                const auto fullPath = GetTexturePath(ConvertPath(path), _files);
                m_dataFound[i] = ReadImage(fullPath, m_imageData[i]);
                if (m_dataFound[i]) {
                    if (firstTime) {
                        m_width = m_imageData[i].m_width;
                        m_height = m_imageData[i].m_height;
                        firstTime = false;
                    }
                    else
                    {
                        assert(m_width == m_imageData[i].m_width);
                        assert(m_height == m_imageData[i].m_height);
                    }
                }               
            }  
        }
        


        bool Combine(ImageExInfo& _result)
        {
            if (m_width == 0 || m_height == 0)
                return false;

            bool allEmpty = true;
            for( int i = 0; i < 4; ++i )
                if (m_dataFound[i]) {
                    allEmpty = false;
                    break;
                }
            if (allEmpty)
                return false;

            auto ZerosOrEmpty = [](const U8Vector& _values)
            {
                if (_values.empty())
                    return true;
                for (auto& val : _values)
                    if (val != 0)
                        return false;
                return true;
            };

            auto Average = [](const U8Vector& _values)
            {
                float total = 0.0f;
                for (auto val : _values)
                {
                    total += val / 255;
                }

                return total / _values.size();
            };



            U8Vector ao,
                rough,
                metal;

            bool valid = true;
            if (m_dataFound[TYPE_COMBINED])
                valid &= SplitChannels(m_imageData[TYPE_COMBINED], &ao, &metal, &rough, nullptr);
            {
                //only fill data if previous solution got no results
                if (m_dataFound[TYPE_AO]) {
                    ao.clear();
                    valid &= SplitChannels(m_imageData[TYPE_AO], &ao, nullptr, nullptr, nullptr);
                }
                if (m_dataFound[TYPE_ROUGH] ) {
                    rough.clear();
                    valid &= SplitChannels(m_imageData[TYPE_ROUGH], &rough, nullptr, nullptr, nullptr);
                }
                if (m_dataFound[TYPE_METAL]) {
                    metal.clear();
                    valid &= SplitChannels(m_imageData[TYPE_METAL], &metal, nullptr, nullptr, nullptr);
                }
            }
            if (!valid)
                return false;
            if (ZerosOrEmpty(ao) || Average(ao) < 0.5 ) { //fill ao term with white values
                ao.resize(m_width * m_height);
                std::fill(std::begin(ao), std::end(ao), 255);
            }
            if (ZerosOrEmpty(rough))
            {
                rough.resize(m_width * m_height);
                std::fill(std::begin(rough), std::end(rough), 127);
            }
            if (ZerosOrEmpty(metal))
            {
                metal.resize(m_width * m_height);
            }



            return CombineChannels(eInternalFormat::INTERNAL_RGB8, m_width, m_height, _result, &ao, &metal, &rough);
        }

        
        aiTextureType m_types[4] = { aiTextureType::aiTextureType_LIGHTMAP,
                                     aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS,
                                     aiTextureType::aiTextureType_METALNESS,
                                     aiTextureType::aiTextureType_UNKNOWN
        };

        ImageExInfo m_imageData[4];
        bool        m_dataFound[4] = { false };
        std::string m_textureName;

        ImageExInfo m_result;
        int         m_width,
                    m_height;
        eTextureType m_type = eTextureType::TYPE_UNDEFINED;
   
    };


    bool LoadTexture(Context* _pContext, const std::vector<std::string>& _files, 
        const aiMaterial* _pMat, aiTextureType _type, HWMaterial* _curMat, eTextureTypes _engineType)
    {
  
        std::map<aiTextureType, std::string> lookup;
        lookup[aiTextureType_DIFFUSE]               = "__DefaultAlbedo";
        lookup[aiTextureType_BASE_COLOR]            = "__DefaultAlbedo";
        lookup[aiTextureType_NORMAL_CAMERA]         = "__DefaultNormal";
        lookup[aiTextureType_NORMALS]               = "__DefaultNormal";
        lookup[aiTextureType_EMISSION_COLOR]        = "__DefaultEmit";
        lookup[aiTextureType_EMISSIVE]               = "__DefaultEmit";
        //lookup[aiTextureType_UNKNOWN]               = "__DefaultUnknown";

        auto  base_color_factor = GetVecFactorAssimp(AI_MATKEY_COLOR_DIFFUSE,  _pMat);
        auto  metallic_factor   = GetNumberFactorAssimp(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR,  _pMat);
        auto  roughness_factor  = GetNumberFactorAssimp(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, _pMat);


        HWTexture* curTex = _pContext->GetResourceManager().GetTexture(lookup[_type]);

        const auto path = GetPathAssimp(_type, _pMat);
        const auto fullPath = GetTexturePath(ConvertPath(path), _files );
      
        if (fullPath.empty()) {
            _curMat->m_textures[_engineType] = curTex;
            return false;
        }
        const auto relPath = RelativePath(GetWorkDirectory(), fullPath); //relative path wrt. work dir
        if (HWTexture* newTex = _pContext->GetResourceManager().GetTexture(relPath)) { //texture in resource cache?
            _curMat->m_textures[_engineType] = newTex;
            return true;
        }

        HWTexInfo params;
        params.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
        params.m_minFilter = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;;
        params.m_wrapR = params.m_wrapS = params.m_wrapT = eWrapMode::WRAP_MODE_REPEAT;
        params.m_target = eTarget::TARGET_TEXTURE_2D;
        params.m_mips = true;
        auto newTex = CreateTexture(_pContext, fullPath, params);
        if (newTex) {
            if(_pContext->GetResourceManager().AddTexture(relPath, std::unique_ptr<HWTexture>(newTex)))
                _curMat->m_textures[_engineType] = newTex;
            return true;
        }
        else
            _curMat->m_textures[_engineType] = curTex;         
        return false;
    }


    std::vector<WrappedEntity*>  LoadModel(Context* _pContext, const std::string& _path, const std::string& _texPath )
    {
        Assimp::Importer importer;
        uint32_t flags = aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_PreTransformVertices |
            aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FixInfacingNormals | aiProcess_GenNormals |
            aiProcess_FindInvalidData | aiProcess_ValidateDataStructure | 0;
        const auto modelPath = ConvertPath( GetModelDirectory() + _path.c_str() );
        
        if( !Exists(modelPath) )
        {
            AddLogMessage(_pContext, fmt::format("File not found:{}", modelPath.c_str()), eLogLevel::LOG_LEVEL_WARNING);
            return {};
        }

        auto pScene = importer.ReadFile(modelPath, flags);
        if (!pScene) {
            AddLogMessage(_pContext, fmt::format("Failure parsing scene:{}", modelPath.c_str()), eLogLevel::LOG_LEVEL_WARNING );
            return {};
        }


        //MeshAndMaterials retVal;
        std::vector<WrappedEntity*> sceneObjects;
        std::vector<HWMaterial*>    materials;
        std::vector<ShaderProgram*> shaders;

        auto IsTextureFilter = [](const std::string& _file)
        {
            auto lower = ToLower(_file);
            return (EndsWith(lower, ".png") ||
                    EndsWith(lower, ".jpg") ||
                    EndsWith(lower, ".tga") ||
                    EndsWith(lower, ".exr"));
                
        };

        std::string startPath = GetTextureDirectory();
        if (!_texPath.empty()) {
            if (IsDirectory(_texPath))
                startPath = _texPath;
          
            else
                AddLogMessage(_pContext, fmt::format("Texture Path {} not found using default", _texPath), eLogLevel::LOG_LEVEL_WARNING);
        }     
        auto files      = GetFiles(startPath, true, IsTextureFilter);
        auto modelFiles = GetFiles(ParentPath( modelPath ), true, IsTextureFilter); //textures in model directory

        files.insert(files.end(), modelFiles.begin(), modelFiles.end());
       
        for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {

           
            using RemapTex = std::pair<aiTextureType, eTextureTypes >;


            RemapTex  remap[] =
            {
                { aiTextureType_DIFFUSE,              eTextureTypes::ALBEDO},
                { aiTextureType_BASE_COLOR,           eTextureTypes::ALBEDO},
                { aiTextureType_NORMALS,              eTextureTypes::NORMAL},
                { aiTextureType_NORMAL_CAMERA,        eTextureTypes::NORMAL},
                { aiTextureType_EMISSION_COLOR,       eTextureTypes::EMISSION}  ,   
                { aiTextureType_EMISSIVE,             eTextureTypes::EMISSION}
            };

            std::string matName = _path + std::to_string(i);
            auto matPtr = _pContext->GetResourceManager().GetMaterial(matName);
            if (!matPtr)
            {
                matPtr = new HWMaterial();
                _pContext->GetResourceManager().AddMaterial(matName, std::unique_ptr<HWMaterial>(matPtr));
            }

            const auto* pMat = pScene->mMaterials[i];
            //try to load & merge ao, metal roughness
            {
                CombinedData cd(_pContext, files, pMat);
                ImageExInfo image;

                matPtr->m_textures[AO_ROUGH_METAL] = _pContext->GetResourceManager().GetTexture("__DefaultUnknown");
                if (cd.Combine(image))
                {
                    HWTexInfo params;
                    params.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
                    params.m_minFilter = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;
                    params.m_wrapR = params.m_wrapS = params.m_wrapT = eWrapMode::WRAP_MODE_REPEAT;
                    params.m_target  = eTarget::TARGET_TEXTURE_2D;
                    params.m_mips    = true;   
                    params.m_texName = std::string("ao_rough_metal_") + matName;
                   
                    auto texPtr = CreateTexture(_pContext, image, params);
                    if (texPtr) 
                        matPtr->m_textures[AO_ROUGH_METAL] = texPtr;    
                }  
            }
            //parse remaning textures
            for (auto texType : remap) {
                auto idx = texType.second; //engine idx

                if (matPtr->m_textures[idx] && !matPtr->m_textures[idx]->m_texInfo.m_internal) {
                    continue;
                }
                LoadTexture(_pContext, files, pMat, texType.first, matPtr, idx);
            }

            auto shaderPtr  = _pContext->GetResourceManager().GetShader("__DefaultIrradianceShader");

            {//environment textures

                matPtr->m_textures[BRDF_LUT]    = _pContext->GetResourceManager().GetTexture("BRDF_Lut");
                matPtr->m_textures[IRRADIANCE]  = _pContext->GetResourceManager().GetTexture("Env_Irradiance");
                matPtr->m_textures[PREFILTER]   = _pContext->GetResourceManager().GetTexture("Env_PreFiltered");
                matPtr->m_textures[ENV]         = _pContext->GetResourceManager().GetTexture("Env_Cube");
            }



            materials.push_back(matPtr);
            shaders.push_back(shaderPtr);

            //retVal.m_materials.push_back(matPtr);
        }


        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);


        // Initialize the meshes in the scene one by one
        for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
            const aiMesh* paiMesh = pScene->mMeshes[i];
            auto matIdx = paiMesh->mMaterialIndex;

            std::vector<Vector3f>     verts;
            std::vector<Vector3f>     normals;
            std::vector<Vector3f>     tangents;
            std::vector<Vector2f>     uvs;
            std::vector<uint32_t>     indices;

            for (int j = 0; j < (int)paiMesh->mNumVertices; j++)
            {
                const aiVector3D* pPos = &(paiMesh->mVertices[j]);
                const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
                const aiVector3D* pTangent = paiMesh->HasTangentsAndBitangents() ? &(paiMesh->mTangents[j]) : &Zero3D;
                const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;

                verts.push_back({ pPos->x, pPos->y, pPos->z });
                normals.push_back({ pNormal->x, pNormal->y, pNormal->z });
                tangents.push_back({ pTangent->x, pTangent->y, pTangent->z });
                uvs.push_back({ pTexCoord->x, pTexCoord->y });
            }
            for (unsigned int j = 0; j < paiMesh->mNumFaces; j++) {
                const aiFace& face = paiMesh->mFaces[j];
                assert(face.mNumIndices == 3);
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }

            //create new mesh
            auto mesh = CreateIndexedMesh(_pContext, verts.data(), normals.data(), tangents.data(), uvs.data(), indices.data(),
                indices.size(), verts.size(), materials[matIdx], shaders[matIdx]);
            sceneObjects.push_back(mesh);   
        }

        auto  GetLightType = [&](aiLightSourceType _type)
        {
            switch (_type)
            {
            case aiLightSourceType::aiLightSource_POINT:
                return eLightType::POINT_LIGHT;
            case aiLightSourceType::aiLightSource_SPOT:
                return eLightType::SPOT_LIGHT;
            case aiLightSourceType::aiLightSource_DIRECTIONAL:
                return eLightType::DIR_LIGHT;
            case aiLightSourceType::aiLightSource_AREA:
                return eLightType::AREA_LIGHT;
            default:
                AddLogMessage(_pContext, "Unknown LightSource, returning Point", eLogLevel::LOG_LEVEL_WARNING);
            }
            return eLightType::POINT_LIGHT;
        };



        //std::vector<LightBase*> lights;
        ////parse cameras
        //for (unsigned int i = 0; i < pScene->mNumLights; i++) {
        //    const auto* pAiLight = pScene->mLights[i];
        //    LightBase* pNewLight = new LightBase(_pContext);
        //    pNewLight->m_position    = Vector4f { pAiLight->mPosition.x, pAiLight->mPosition.y, pAiLight->mPosition.z, 0.0f };
        //    pNewLight->m_direction   = Vector4f { pAiLight->mDirection.x, pAiLight->mDirection.y, pAiLight->mDirection.z , 0.0f };
        //    pNewLight->m_color       = Vector4f { pAiLight->mColorDiffuse.r, pAiLight->mColorDiffuse.g,  pAiLight->mColorDiffuse.b, 0.0f };
        //    pNewLight->m_angles      = Vector4f(pAiLight->mAngleInnerCone, pAiLight->mAngleOuterCone, 0.0f, 0.0f);
        //    pNewLight->m_attenuation = Vector4f(pAiLight->mAttenuationConstant, pAiLight->mAttenuationLinear,pAiLight->mAttenuationQuadratic, 0.0f);
        //    pNewLight->m_area        = Vector2f( pAiLight->mSize.x, pAiLight->mSize.y);
        //    pNewLight->m_type        = GetLightType(pAiLight->mType);
        //    sceneObjects.push_back(pNewLight);
        //}
        return sceneObjects;
    }

}

