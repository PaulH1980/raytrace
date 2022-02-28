#pragma once
#include "HWRenderer.h"
#include "WrappedEntity.h"
#include "HWShader.h"
#include "ShaderSources.h"
#include "HWMesh.h"
#include "ResourceManager.h"
#include "HWVertexBuffer.h"
#include "HWTexture.h"
#include "HWFrameBuffer.h"
#include "Controller.h"
#include "EventHandler.h"
#include "LayerManager.h"
#include "Spectrum.h"
#include "WorldLayer.h"
#include "SkyLayer.h"
#include "UILayer.h"
#include "Logger.h"
#include "SceneView.h"
#include "HWMaterial.h"
#include "SceneManager.h"
#include "HWLight.h"
#include "Component.h"
#include "OverlayLayer.h"
#include "UILayer.h"
#include "SkyLayer.h"
#include "EditorLayer.h"
#include "WorldLayer.h"
#include "IO.h"
#include "TexInfo.h"
#include "HistoryStack.h"
#include "InputHandler.h"
#include "HWPass.h"
#include "SceneItemCollector.h"
#include "CopyPaste.h"
#include "FilePaths.h"
#include "Tools.h"
#include "Context.h"

namespace RayTrace
{
   
    bool Context::CreateDefaultSystems()
    {
        m_systemMap["Logger"] = std::make_unique<Logger>(this);
        m_logger = static_cast<Logger*>(m_systemMap["Logger"].get());

        m_systemMap["EventHandler"] = std::make_unique<EventHandler>(this);
        m_evtHandler = static_cast<EventHandler*>(m_systemMap["EventHandler"].get());

        m_systemMap["InputHandler"] = std::make_unique<InputHandler>(this);
        m_inputHandler = static_cast<InputHandler*>(m_systemMap["InputHandler"].get());

        m_systemMap["ResourceManager"] = std::make_unique<ResourceManager>(this);
        m_resMan = static_cast<ResourceManager*>(m_systemMap["ResourceManager"].get());

        m_systemMap["LayerManager"] = std::make_unique<LayerManager>(this);
        m_layerManager = static_cast<LayerManager*>(m_systemMap["LayerManager"].get());

        m_systemMap["ComponentSystem"] = std::make_unique<ComponentSystem>(this);
        m_componentSystem = static_cast<ComponentSystem*>(m_systemMap["ComponentSystem"].get());

        m_systemMap["SceneManager"] = std::make_unique<SceneManager>(this);
        m_sceneManager = static_cast<SceneManager*>(m_systemMap["SceneManager"].get());

        m_systemMap["HistoryStack"] = std::make_unique<HistoryStack>(this);
        m_history = static_cast<HistoryStack*>(m_systemMap["HistoryStack"].get());

        m_systemMap["ToolManager"] = std::make_unique<ToolManager>(this);
        m_pToolManager = static_cast<ToolManager*>(m_systemMap["ToolManager"].get());


        m_copyPasteBuffer = std::make_unique<CopyPasteBuffer>(this);

        return true;
    }

    bool Context::CreateDefaultObjects()
    {
        bool valid = true;

        valid &= GetLayerManager().AddLayer(eLayer::LAYER_WORLD_SOLID, std::make_unique<WorldLayer>(this));
        valid &= GetLayerManager().AddLayer(eLayer::LAYER_SKYBOX, std::make_unique<SkyBoxLayer>(this));
        valid &= GetLayerManager().AddLayer(eLayer::LAYER_EDITOR, std::make_unique<EditorLayer>(this));
        if( m_firstTime )
            valid &= GetLayerManager().AddLayer(eLayer::LAYER_UI, std::make_unique<UILayer>(this, m_windowHandle, m_hwContextHandle));
        


        GetToolManager().AddTool("Transform", std::make_unique<GizmoTool>(this));
        //GetToolManager().AddTool("Selection", std::make_unique<SceneSelection>(this));
        GetToolManager().AddTool("Add Entity", std::make_unique<PlaceSceneObjectTool>(this));

        return valid;
    }

    bool Context::CreateRenderer()
    {
        //init renderer after items are created
        m_systemMap["Renderer"] = std::make_unique<HWRenderer>(this);
        m_renderBackend = static_cast<HWRenderer*>(m_systemMap["Renderer"].get());
        return m_renderBackend != nullptr;
    }
    

    bool Context::CreateRendererFrameBuffers()
    {
        bool valid = true;

        HWTexInfo params;
        params.m_width      = params.m_height = 2;
        params.m_wrapR      = params.m_wrapS = params.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
        params.m_minFilter  = eMinFilter::MIN_FILTER_LINEAR;
        params.m_magFilter  = eMagFilter::MAG_FILTER_LINEAR;
        params.m_depth      = 1;
        params.m_target     = eTarget::TARGET_TEXTURE_2D;
        
        //Picking Framebuffer
        valid &= GetResourceManager().AddFrameBuffer("__DefaultPickingFrameBuffer", std::unique_ptr<HWFrameBuffer>(
            CreateFrameBuffer(this, { eInternalFormat::INTERNAL_R32UI, eInternalFormat::INTERNAL_DEPTH_COMPONENT32F }, params, false)));

        //Depth only framebuffer
        valid &= GetResourceManager().AddFrameBuffer("__DefaultDepthOnlyFrameBuffer", std::unique_ptr<HWFrameBuffer>(
            CreateFrameBuffer(this, { eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, params, false)));

        //Default framebuffer
        valid &= GetResourceManager().AddFrameBuffer("__DefaultFrameBuffer", std::unique_ptr<HWFrameBuffer>(
            CreateFrameBuffer(this, { eInternalFormat::INTERNAL_RGBA8, eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, params)));

        //add default camera, controller and sceneview
        valid &= GetResourceManager().AddSceneView("__DefaultSceneView", std::make_unique<SceneView>(this,
                 GetResourceManager().GetFrameBuffer("__DefaultFrameBuffer"), std::make_unique<ArcballCameraController>(this)));
        valid &= GetRenderer().RendererValid();
        return valid;
    }

    bool Context::CreateRendererShaders()
    {
        bool valid = true;



        std::vector<std::string> prepend =
        {
            GLSL_VERSION,
            GLSL_SHARED,
            GLSL_FUNCTIONS,
        };
        const std::string ShaderDir = "assets/shaders/";

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "Irradiance.vert", vert);
            valid &= ReadTextFile(ShaderDir + "Irradiance.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultIrradianceShader", std::move(prog));

        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "PreFilterEnvIbl.vert", vert);
            valid &= ReadTextFile(ShaderDir + "PreFilterEnvIbl.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultPrefilterEnvIblShader", std::move(prog));

        }

        //WriteObjectIds.frag
        {
            {
                std::string vert, frag;
                valid &= ReadTextFile(ShaderDir + "WriteObjectIds.vert", vert);
                valid &= ReadTextFile(ShaderDir + "WriteObjectIds.frag", frag);
                assert(valid);

                auto prog = std::make_unique<ShaderProgram>(
                    std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                    std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
                valid &= GetResourceManager().AddShader("__DefaultWriteObjectIdsShader", std::move(prog));

            }
        }
        {
            {
                std::string vert, frag;
                valid &= ReadTextFile(ShaderDir + "IconShader.vert", vert);
                valid &= ReadTextFile(ShaderDir + "IconShader.frag", frag);
                assert(valid);

                auto prog = std::make_unique<ShaderProgram>(
                    std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                    std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
                valid &= GetResourceManager().AddShader("__DefaultIconShader", std::move(prog));

            }

            {
                std::string vert, frag;
                valid &= ReadTextFile(ShaderDir + "SelectionShader.vert", vert);
                valid &= ReadTextFile(ShaderDir + "SelectionShader.frag", frag);
                assert(valid);

                auto prog = std::make_unique<ShaderProgram>(
                    std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                    std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
                valid &= GetResourceManager().AddShader("__DefaultSelectionShader", std::move(prog));

            }
        }


        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "Brdf.vert", vert);
            valid &= ReadTextFile(ShaderDir + "Brdf.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, { GLSL_VERSION,  GLSL_BRDF_LUT, GLSL_SHARED, GLSL_FUNCTIONS }), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultBrdfLut", std::move(prog));

        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "LineShader.vert", vert);
            valid &= ReadTextFile(ShaderDir + "LineShader.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultLineShader", std::move(prog));
        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "GridShader.vert", vert);
            valid &= ReadTextFile(ShaderDir + "GridShader.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultGridShader", std::move(prog));

        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "DepthOnly.vert", vert);
            valid &= ReadTextFile(ShaderDir + "DepthOnly.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultDepthOnlyShader", std::move(prog));

            auto depthOnlyMat = new HWMaterial();
            valid &= GetResourceManager().AddMaterial("__DefaultDepthOnlyMaterial", std::unique_ptr<HWMaterial>(depthOnlyMat));
        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "DepthOnlyWithTexMask.vert", vert);
            valid &= ReadTextFile(ShaderDir + "DepthOnlyWithTexMask.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultDepthOnlyWithTexMaskShader", std::move(prog));

            auto depthOnlyMat = new HWMaterial();
            valid &= GetResourceManager().AddMaterial("__DefaultDepthOnlyWithTexMaskMaterial", std::unique_ptr<HWMaterial>(depthOnlyMat));
        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "Equirectangular.vert", vert);
            valid &= ReadTextFile(ShaderDir + "Equirectangular.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultEquirectangularShader", std::move(prog));

        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "ConvoleCubemap.vert", vert);
            valid &= ReadTextFile(ShaderDir + "ConvoleCubemap.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__ConvoleCubemapShader", std::move(prog));

        }


        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "WriteDepth.vert", vert);
            valid &= ReadTextFile(ShaderDir + "WriteDepth.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultWriteDepthShader", std::move(prog));

            auto writeDepthMat = new HWMaterial();
            valid &= GetResourceManager().AddMaterial("__DefaultWriteDepthMaterial", std::unique_ptr<HWMaterial>(writeDepthMat));
        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "Overlay.vert", vert);
            valid &= ReadTextFile(ShaderDir + "Overlay.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultOverlayShader", std::move(prog));

            auto ovelayMat = new HWMaterial();
            valid &= GetResourceManager().AddMaterial("__DefaultOverlayMaterial", std::unique_ptr<HWMaterial>(ovelayMat));
        }

        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "DebugGeometry.vert", vert);
            valid &= ReadTextFile(ShaderDir + "DebugGeometry.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultDebugGeometryShader", std::move(prog));

        }


        {

            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "DefaultShader.vert", vert);
            valid &= ReadTextFile(ShaderDir + "DefaultShader.frag", frag);
            assert(valid);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultShader", std::move(prog));
        }
        {
            std::string vert, frag;
            valid &= ReadTextFile(ShaderDir + "SkyBox.vert", vert);
            valid &= ReadTextFile(ShaderDir + "SkyBox.frag", frag);
            assert(valid);


            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(vert, prepend), Shader::SHADER_TYPE_VERT),
                std::make_unique<Shader>(AssembleShaderCode(frag, prepend), Shader::SHADER_TYPE_FRAG));
            valid &= GetResourceManager().AddShader("__DefaultSkyBoxShader", std::move(prog));
        }
        {
            std::string comp;
            valid &= ReadTextFile(ShaderDir + "LightCulling.comp", comp);

            auto prog = std::make_unique<ShaderProgram>(
                std::make_unique<Shader>(AssembleShaderCode(comp, std::vector<std::string> { GLSL_VERSION }), Shader::SHADER_TYPE_COMPUTE));
            valid &= GetResourceManager().AddShader("__LightCullingShader", std::move(prog));
        }
        valid &= GetRenderer().RendererValid();
        return valid;
    }

    
    
    bool Context::CreateRendererMaterials()
    {
       
        bool valid = true;    
        

        { //Default textures & material
            using texData = std::vector<Vector4<uint8_t>>;

            texData  albedoData(4, Vector4<uint8_t>(128, 128, 255, 255));
            texData  normalData(4, Vector4<uint8_t>(128, 128, 255, 255));
            texData  roughData(4, Vector4<uint8_t>(128, 128, 128, 255));
            texData  metalData(4, Vector4<uint8_t>(128, 128, 128, 255));
            texData  aoData(4, Vector4<uint8_t>(255, 255, 255, 255));
            texData  emitData(4, Vector4<uint8_t>(0, 0, 0, 255));
            texData  unknownData(4, Vector4<uint8_t>(255, 128, 0, 255)); //ao rough metal

            texData* pixels[7] = { &albedoData, &normalData, &roughData, &metalData, &aoData, &emitData, &unknownData };
            std::string names[7] = { "__DefaultAlbedo", "__DefaultNormal", "__DefaultRough",
                "__DefaultMetal", "__DefaultAO", "__DefaultEmit", "__DefaultUnknown"};
            for (int i = 0; i < 7; ++i)
            {
                HWTexInfo ti = TexInfoFromInternalFormt(eInternalFormat::INTERNAL_RGBA8);
                ti.m_width = 2;
                ti.m_height = 2;
                ti.m_mips = false;
                ti.m_internal = true;
                ti.m_pData[0] = (const char*)pixels[i]->data();
                auto tex = CreateTexture(this, ti);
                assert(tex);

                valid &= GetResourceManager().AddTexture(names[i], std::unique_ptr<HWTexture>(tex));
            }

            {
                HWMaterial* newMat = new HWMaterial();
                // newMat->m_shader = GetResourceManager().GetShader("__DefaultShader");
                newMat->m_textures[eTextureTypes::ALBEDO] = GetResourceManager().GetTexture("__DefaultAlbedo");
                newMat->m_textures[eTextureTypes::NORMAL] = GetResourceManager().GetTexture("__DefaultNormal");
                newMat->m_textures[eTextureTypes::AO_ROUGH_METAL] = GetResourceManager().GetTexture("__DefaultUnknown");
                newMat->m_textures[eTextureTypes::EMISSION] = GetResourceManager().GetTexture("__DefaultEmit");
                valid &= GetResourceManager().AddMaterial("__DefaultMaterial", std::unique_ptr<HWMaterial>(newMat));     
            }
            {
                HWMaterial* newMat = new HWMaterial(); //null material
                valid &= GetResourceManager().AddMaterial("__DefaultNullMaterial", std::unique_ptr<HWMaterial>(newMat));
            }
        }
       
        //SkyBox
        {
            std::array<std::string, 6> faces =
            {
                ConvertPath( GetTextureDirectory() +  "skybox/sea/right.jpg"),
                ConvertPath( GetTextureDirectory() +  "skybox/sea/left.jpg"),
                ConvertPath( GetTextureDirectory() +  "skybox/sea/top.jpg"),
                ConvertPath( GetTextureDirectory() +  "skybox/sea/bottom.jpg"),
                ConvertPath( GetTextureDirectory() +  "skybox/sea/front.jpg"),
                ConvertPath( GetTextureDirectory() +  "skybox/sea/back.jpg")
            };
            HWTexInfo info;
     
            info.m_internal  = true;
            info.m_mips      = false;          
            info.m_target    = eTarget::TARGET_TEXTURE_CUBE_MAP;
            info.m_depth     = 0;
            info.m_minFilter = eMinFilter::MIN_FILTER_LINEAR;
            info.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
            info.m_wrapR = info.m_wrapS = info.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
            auto texPtr = CreateTextureCube(this, faces, info);
            valid &= GetResourceManager().AddTexture("__DefaultSeaSkybox", std::unique_ptr<HWTexture>(texPtr));
          //  valid  &= GetResourceManager().AddTexture("__DefaultSeaSkybox", std::unique_ptr<HWTexture>( bufPtr->GetTextures()[0].get() ) );
           // valid &= CreateEnvironmentDataFromResource(this, "__DefaultSeaSkybox", 32, 256);
           // if (glGetError() != GL_NO_ERROR)
           //     throw std::exception("Texture Creation Failure");  

        }
        {
            auto skyBoxMaterial = new HWMaterial();
            skyBoxMaterial->m_textures[eTextureTypes::ALBEDO] = GetResourceManager().GetTexture("__DefaultSeaSkybox");
            valid &= GetResourceManager().AddMaterial("__DefaultSkyBoxMaterial", std::unique_ptr<HWMaterial>(skyBoxMaterial));
            auto skybox = CreateSkyBox(this, GetResourceManager().GetShader("__DefaultSkyBoxShader"),
                                             GetResourceManager().GetMaterial("__DefaultSkyBoxMaterial"));
            
            GetSceneManager().AddObject( skybox->GetUUID(), WrappedEntityUPtr( skybox ) );

           
            
        }
       

        {
            
            auto AddIconTexture = [&]( const std::string& _name,  std::string _fileName) {
                HWTexInfo info;

                info.m_internal = true;
                info.m_mips = true;
                info.m_target = eTarget::TARGET_TEXTURE_2D;
                info.m_depth = 1;
                info.m_flipYOnRead = false;
                info.m_minFilter = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;
                info.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
                const std::string texDir = ConvertPath(GetTextureDirectory() + "Editor/");
                auto icon = CreateTexture(this, texDir + _fileName, info);
                if (!icon)
                    return false;
                return GetResourceManager().AddTexture(_name, HWTextureUPtr(icon));                
            };
            
            valid &= AddIconTexture("LightIcon", "LightIcon.png");   
            valid &= AddIconTexture("Folder", "folder.png");
            valid &= AddIconTexture("ParentFolder", "folder_parent.png");
        }

        return valid;
    }


    bool Context::CreateRendererPasses()
    {

        bool valid = true;
        {//passes & collectors
            auto depthOnlyState = GetRenderer().GetInitialState().Clone();
            depthOnlyState->mask.SetColorMask(0, false, false, false, false);
            depthOnlyState->mask.depth = true;
            depthOnlyState->depth.func = GL_LEQUAL;
            depthOnlyState->enable.field.cull_face = true;
            depthOnlyState->enable.field.depth_test = true;

            auto meshState = GetRenderer().GetInitialState().Clone();
            meshState->depth.func = GL_LEQUAL;
            meshState->mask.depth = true;
            meshState->enable.field.cull_face = true;
            meshState->enable.field.depth_test = true;

            auto skyState = GetRenderer().GetInitialState().Clone();
            skyState->depth.func = GL_LEQUAL;
            skyState->enable.field.cull_face = false;
            skyState->enable.field.depth_test = true;

            auto editorState = GetRenderer().GetInitialState().Clone();
            editorState->depth.func = GL_LEQUAL;
            editorState->mask.depth = true;
            editorState->enable.field.cull_face = true;
            editorState->enable.field.depth_test = true;

            auto gridState = GetRenderer().GetInitialState().Clone();
            gridState->depth.func = GL_LEQUAL;
            gridState->mask.depth = true;
            gridState->enable.field.cull_face = true;
            gridState->enable.field.depth_test = true;


            auto blendState = GetRenderer().GetInitialState().Clone();
            blendState->depth.func          = GL_LEQUAL;
            blendState->mask.depth          = false;
            blendState->enable.field.blend  = true;
            blendState->enable.field.cull_face = true;
            blendState->enable.field.depth_test = true;
            blendState->blend.blends[0].alpha.srcw = GL_SRC_ALPHA;
            blendState->blend.blends[0].alpha.dstw = GL_ONE_MINUS_SRC_ALPHA;
            blendState->blend.blends[0].rgb.srcw   = GL_SRC_ALPHA;
            blendState->blend.blends[0].rgb.dstw   = GL_ONE_MINUS_SRC_ALPHA;         

            valid &= GetResourceManager().AddRenderState("__BlendState", std::unique_ptr<RenderState>(blendState));
            valid &= GetResourceManager().AddRenderState("__MeshRenderState", std::unique_ptr<RenderState>(meshState));
            valid &= GetResourceManager().AddRenderState("__DepthOnlyRenderState", std::unique_ptr<RenderState>(depthOnlyState));
            valid &= GetResourceManager().AddRenderState("__SkyRenderState", std::unique_ptr<RenderState>(skyState));
            valid &= GetResourceManager().AddRenderState("__EditorRenderState", std::unique_ptr<RenderState>(editorState));
            valid &= GetResourceManager().AddRenderState("__EditorGridState", std::unique_ptr<RenderState>(gridState));


            EntityFilter IsMeshFilter = [](WrappedEntity* _pEnt)
            {
                return _pEnt->HasComponent<MeshComponent>() ||
                       _pEnt->HasComponent<IndexedMeshComponent>() && _pEnt->GetObjectFlags().m_field.m_visible;
            };


            EntityFilter IsSkyBoxFilter = [](WrappedEntity* _pEnt)
            {
                return _pEnt->HasComponent<SkyBoxComponent>() && _pEnt->GetObjectFlags().m_field.m_visible;
            };

            EntityFilter IsLightFilter = [](WrappedEntity* _pEnt)
            {
                return _pEnt->HasComponent<LightComponent>() && _pEnt->GetObjectFlags().m_field.m_visible;
            };

            EntityFilter EditorPassFilter = [IsMeshFilter](WrappedEntity* _pEnt) {
                return  (IsMeshFilter(_pEnt) || 
                    ( _pEnt->HasComponent<LightComponent>()) ||
                    ((_pEnt->HasComponent<EditorSpriteComponent>()) && _pEnt->HasComponent<Position3fComponent>()) 
                    
                    ) ;
            };


            auto depthOnlyPass = new DepthPrePass(this, 
                "Depth Only Pass", 
                std::make_unique<FilteredSceneItemCollector>(IsMeshFilter),
                depthOnlyState);
            auto meshPass = new HWPass(this, 
                "World Pass", 
                std::make_unique<FilteredSceneItemCollector>(IsMeshFilter),
                meshState);
            auto skyPass = new HWPass(this,
                    "Sky Pass", 
                std::make_unique<FilteredSceneItemCollector>(IsSkyBoxFilter),
                skyState);
            auto lightPass = new HWPass(this, 
                "Light Pass", 
                 std::make_unique<FilteredSceneItemCollector>(IsLightFilter)
            );
            auto editorPass = new EditorPass(this,
                "Editor Pass", 
                std::make_unique<FilteredSceneItemCollector>(EditorPassFilter),
                editorState);

            lightPass->SetShader(GetResourceManager().GetShader("__LightCullingShader"));

            valid &= GetResourceManager().AddPass("__DepthOnlyPass", std::unique_ptr<HWPass>(depthOnlyPass));
            valid &= GetResourceManager().AddPass("__LightPass", std::unique_ptr<HWPass>(lightPass));
            valid &= GetResourceManager().AddPass("__MeshPass", std::unique_ptr<HWPass>(meshPass));
            valid &= GetResourceManager().AddPass("__SkyPass", std::unique_ptr<HWPass>(skyPass));
            valid &= GetResourceManager().AddPass("__EditorPass", std::unique_ptr<HWPass>(editorPass));

            //assign passes to layers
            GetLayerManager().GetLayer(LAYER_WORLD_SOLID)->AddPass(depthOnlyPass);
            GetLayerManager().GetLayer(LAYER_WORLD_SOLID)->AddPass(lightPass);
            GetLayerManager().GetLayer(LAYER_WORLD_SOLID)->AddPass(meshPass);
            GetLayerManager().GetLayer(LAYER_WORLD_SOLID)->AddPass(skyPass);

            GetLayerManager().GetLayer(LAYER_EDITOR)->AddPass(editorPass);   

            return true;
        }
    }
}