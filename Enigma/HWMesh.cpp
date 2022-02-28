
#include "Common.h"
#include "HWMaterial.h"
#include "HWTexture.h"
#include "HWShader.h"
#include "HWVertexBuffer.h"
#include "TriangleMesh.h"
#include "HWRenderer.h"
#include "HWTexture.h"
#include "ResourceManager.h"
#include "HWFrameBuffer.h"
#include "Context.h"
#include "WrappedEntity.h"
#include "Geometry.h"

#include "HWMesh.h"




///*        
//        {
//           */ ////   GeometryBase baseGeo(m_pContext);
//            ModifierStack modStack(&_pMesh->GetContext());
//            auto* newMode = new CubeModifier(&modStack);
//            modStack.AddModifier(std::unique_ptr<ModifierBase>(newMode));
//            modStack.Apply();
//
//           
//            auto geom = modStack.GetGeometry();
//
//            auto texPtr = _pMesh->GetContext().GetResourceManager().GetTexture("BRDF_Lut");
//            auto shader = _pMesh->GetContext().GetResourceManager().GetShader("__DefaultDebugGeometryShader");
//            int texUnit = 0;
//            texPtr->Bind(texUnit);
//            shader->Bind();
//            shader->SetInt("albedo", &texUnit);
//
//           _pMesh->GetContext().GetRenderer().DrawTriangles(geom->m_verts.data(), geom->m_verts.size(), shader,
//                geom->m_normals.data(), nullptr, geom->m_uvs.data());
//
//            shader->UnBind();
//            texPtr->UnBind();
//           // renderer.PopState();
// 
//        }



namespace RayTrace
{
    void DrawSkyBox(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader)
    {
        auto pSkyComp = _pEntity->GetComponent<SkyBoxComponent>();
        if (!pSkyComp)
            return;        
        bool localMat = false;
        bool localShader = false;

        if (!_pShader)
        {
            _pShader = _pEntity->GetComponent<ShaderComponent>()->m_shader.m_pShader;
            _pShader->Bind();
            localShader = true;
        }


        if (!_pMat) {
            _pMat = _pEntity->GetComponent<MaterialComponent>()->m_material.m_pMaterial;
            _pMat->Bind(_pShader);
            localMat = true;
        }

        _pRenderer->BindVertexArrayObject(pSkyComp->m_vaoId);
        _pRenderer->DrawArrays(eDrawMode::DRAWMODE_TRIANGLES, 0, pSkyComp->m_vertexCount);
        _pRenderer->BindVertexArrayObject(0);

        if (localMat)
            _pMat->UnBind();
        if(localShader)
            _pShader->UnBind();
    }

    void DrawMesh(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader)
    {
        auto pMesh = _pEntity->GetComponent<MeshComponent>();
        if (!pMesh)
            return;

        bool localMat = false;
        bool localShader = false;

        if (!_pShader) {
            _pShader = _pEntity->GetComponent<ShaderComponent>()->m_shader.m_pShader;
            _pShader->Bind();
            localShader = true;
        }
        if (!_pMat) {
            _pMat = _pEntity->GetComponent<MaterialComponent>()->m_material.m_pMaterial;
            _pMat->Bind(_pShader);
            localMat = true;
        }


        const auto& transform = _pEntity->GetWorldTransform();
        _pRenderer->BindVertexArrayObject(pMesh->m_vaoId);
        _pShader->SetMatrix4x4("model", &transform);
        _pRenderer->DrawArrays(eDrawMode::DRAWMODE_TRIANGLES, 0, pMesh->m_numVertices);
        _pRenderer->BindIndexBuffer(0);
        _pRenderer->BindVertexArrayObject(0);

        if (localMat)
            _pMat->UnBind();
        if (localShader)
            _pShader->UnBind();
    }


    void DrawIndexedMesh(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader)
    {
        auto pIndexMesh = _pEntity->GetComponent<IndexedMeshComponent>();
        if (!pIndexMesh)
            return;


        bool localMat = false;
        bool localShader = false;

        if (!_pShader)
        {
            _pShader = _pEntity->GetShader();
            _pShader->Bind();
            localShader = true;
        }
        if (!_pMat) {
            _pMat = _pEntity->GetMaterial();
            _pMat->Bind(_pShader);
            localMat = true;
        }


        const auto& transform = _pEntity->GetWorldTransform();
       
        _pRenderer->BindVertexArrayObject(pIndexMesh->m_vaoId);
        _pRenderer->BindIndexBuffer(pIndexMesh->m_indexBufferId);
        _pShader->SetMatrix4x4("model", &transform);

   
        _pRenderer->DrawIndexed(eDrawMode::DRAWMODE_TRIANGLES, pIndexMesh->m_numIndices);

        _pRenderer->BindIndexBuffer(0);
        _pRenderer->BindVertexArrayObject(0);          

        if (localMat)
            _pMat->UnBind();
        if (localShader)
            _pShader->UnBind();
    }


    //////////////////////////////////////////////////////////////////////////
    //HWDefaultMesh declaration
    //////////////////////////////////////////////////////////////////////////
    class HWDefaultMesh : public MeshBase
    {

        RT_OBJECT(HWDefaultMesh, MeshBase)


    public:
        HWDefaultMesh(Context* _pContext);
        ~HWDefaultMesh();

        void        CreateVao() override;

   
        std::unique_ptr<IndexBuffer>    m_indices;
        std::unique_ptr<Vec3fBuffer>    m_verts,   //attId : 0
                                        m_normals, //attId : 1
                                        m_tangents;//attId : 2
        std::unique_ptr<Vec2fBuffer>    m_uvs;     //attId : 3    

        std::unique_ptr<VertexArrayObject> m_vao;
    };

    class SkyBoxMesh : public MeshBase
    {
    public:
        RT_OBJECT(SkyBoxMesh, MeshBase)

            SkyBoxMesh(Context* _pContext, HWMaterial* _pMaterial);
        virtual ~SkyBoxMesh();

        void CreateVao()                   override;
        std::unique_ptr<Vec3fBuffer>       m_verts;   //attId : 0   
        std::unique_ptr<VertexArrayObject> m_vao;
    };

    WrappedEntity* CreateIndexedMesh(
        Context* _pContext, Vector3f* const _verts, const Vector3f* _normals,
        const Vector3f* _tangents, const  Vector2f* uvs, const uint32_t* _indices, uint32_t _numIndices,
        uint32_t _numVerts, HWMaterial* _pMaterial, ShaderProgram* _shader, bool _transformToOrigin )
    {
        HWDefaultMesh* mesh = new HWDefaultMesh(_pContext);
        auto bounds = GetBounds( _verts, _numVerts);
        auto transform = Identity4x4;
        if (_transformToOrigin) {
            const auto center = bounds.getCenter();
            for (int i = 0; i <(int) _numVerts; ++i ) 
                _verts[i] = _verts[i] - center;
            //0651042619 chnl-20123518
            
            bounds.m_min = bounds.m_min - center;
            bounds.m_max = bounds.m_max - center;
            transform = Translate4x4(center);
        }
        


        mesh->SetMaterial(_pMaterial);
        mesh->SetShader(_shader);
        mesh->m_indices.reset(CreateIndexBuffer(_indices, _numIndices));
        mesh->m_verts.reset(CreateVec3fBuffer(_verts, _numVerts, 0));
        mesh->m_normals.reset(CreateVec3fBuffer(_normals, _numVerts, 1));
        mesh->m_tangents.reset(CreateVec3fBuffer(_tangents, _numVerts, 2));
        mesh->m_uvs.reset(CreateVec2fBuffer(uvs, _numVerts, 3));
        mesh->CreateVao();
        //add resource
        _pContext->GetResourceManager().AddDrawable(mesh->GetUUID(), std::unique_ptr<HWDefaultMesh>(mesh));        
     
        auto pEnt = CreateIndexedMeshEntity(_pContext);
        pEnt->GetComponent<BBox3fComponent>()->m_bounds = bounds;
        pEnt->GetComponent<TransformComponent>()->m_transform = transform;
        pEnt->GetComponent<MaterialComponent>()->m_material.m_pMaterial = _pMaterial;
        pEnt->GetComponent<ShaderComponent>()->m_shader.m_pShader = _shader;    
     

        auto pMeshComp = pEnt->GetComponent<IndexedMeshComponent>();
        pMeshComp->m_indexBufferId = mesh->GetIndexBufferId();
        pMeshComp->m_numIndices    = mesh->GetIndicesCount();
        pMeshComp->m_vaoId         = mesh->GetVaoId();
        pMeshComp->m_pDrawMeshFun  = DrawIndexedMesh;

        return pEnt;
    }



    WrappedEntity* CreateSkyBox(Context* _pContext, ShaderProgram* _pShader, HWMaterial* _pMat )
    {
        WrappedEntity* pEnt = new WrappedEntity(_pContext);
        SkyBoxMesh* pSkyBox = new SkyBoxMesh(_pContext, nullptr);

        _pContext->GetResourceManager().AddDrawable(pSkyBox->GetUUID(), DrawableBaseUPtr(pSkyBox));

        pEnt->CreateComponent<ShaderComponent>()->m_shader.m_pShader       = _pShader;
        pEnt->CreateComponent<MaterialComponent>()->m_material.m_pMaterial = _pMat;
        
        auto pComp                  = pEnt->CreateComponent<SkyBoxComponent>();
        pComp->m_vaoId              = pSkyBox->GetVaoId();
        pComp->m_vertexCount        = pSkyBox->GetVertexCount();
        pComp->m_pDrawSkyBoxFun     = DrawSkyBox;

        return pEnt;       
    }

    //////////////////////////////////////////////////////////////////////////
    //MeshBase Implementation
    //////////////////////////////////////////////////////////////////////////
  

    MeshBase::MeshBase(Context* _pContext) 
        : DrawableBase(_pContext)        
    {
        m_name = fmt::format("Mesh_Object_{}", m_objectId);
    }

    MeshBase::~MeshBase()
    {

    }

    void MeshBase::SetMaterial(HWMaterial* _pMaterial)
    {
        m_pMaterial = _pMaterial;
    }

    void MeshBase::SetShader(ShaderProgram* _pShader)
    {
        m_pShader = _pShader;
    }

    ShaderProgram* MeshBase::GetShader() const
    {
        return m_pShader;
    }

    uint32_t MeshBase::GetVaoId() const
    {
        return m_vaoId;
    }

    uint32_t MeshBase::GetIndexBufferId() const
    {
        return m_indexBufferId;
    }

    uint32_t MeshBase::GetVertexCount() const
    {
        return m_vertexCount;
    }

    uint32_t MeshBase::GetIndicesCount() const
    {
        return m_indicesCount;
    }

    HWMaterial* MeshBase::GetMaterial() const
    {
        return m_pMaterial;
    }

    //////////////////////////////////////////////////////////////////////////
    //SkyboxMesh Implementation
    //////////////////////////////////////////////////////////////////////////
    SkyBoxMesh::SkyBoxMesh( Context* _pContext, HWMaterial* _pMaterial )
        : MeshBase( _pContext)
        
    {
        SetMaterial(_pMaterial);
        m_name = fmt::format( "SkyBox_Object_{}", m_objectId );
        
        
        Vector3f skyBoxVerts[36] = {
            // positions          
            {-1.0f,  1.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f},

            {1.0f, -1.0f, -1.0f},
            {1.0f, -1.0f,  1.0f},
            {1.0f,  1.0f,  1.0f},
            {1.0f,  1.0f,  1.0f},
            {1.0f,  1.0f, -1.0f},
            {1.0f, -1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f},
            {-1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f},

            {-1.0f,  1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            {-1.0f,  1.0f,  1.0f},
            {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f},
        };

        m_verts.reset(CreateVec3fBuffer(skyBoxVerts, 36, 0));
        m_vertexCount = m_verts->GetCount();
        CreateVao();
    }

    SkyBoxMesh::~SkyBoxMesh()
    {

    }

  

    void SkyBoxMesh::CreateVao()
    {
        HWBufferBase* buffers[] = { m_verts.get() };
        m_vao = std::make_unique<VertexArrayObject>(buffers, std::size(buffers));
        m_vaoId       = m_vao->m_vaoId;
        m_vertexCount = m_verts->GetCount();
    }
    
    
    
    //////////////////////////////////////////////////////////////////////////
    //HWDefaultMesh Implementation
    //////////////////////////////////////////////////////////////////////////
    HWDefaultMesh::HWDefaultMesh( Context* _pContext)
        : MeshBase( _pContext )
    {

    }

    HWDefaultMesh::~HWDefaultMesh()
    {

    }
    

    void HWDefaultMesh::CreateVao()
    {
        HWBufferBase* buffers[] = { m_verts.get(), m_normals.get(), m_tangents.get(), m_uvs.get() };
        m_vao = std::make_unique<VertexArrayObject>(buffers, std::size(buffers));
        m_vaoId = m_vao->m_vaoId;
        m_vertexCount = m_verts->GetCount();
        m_indexBufferId = m_indices->GetBufferId();
        m_indicesCount = m_indices->GetCount();

    }

    

}

