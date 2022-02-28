#pragma once

#include "HWFormats.h"
#include "Transform.h"
#include "SystemBase.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "HWTexInfo.h"
#include "HWDrawable.h"

namespace RayTrace
{
    WrappedEntity* CreateIndexedMesh(Context* _pContext,  Vector3f* const _verts, const Vector3f* _normals, const Vector3f* _tangents,
                                     const  Vector2f* uvs, const  uint32_t* _indices, uint32_t _numVerts, 
                                     uint32_t _numIndices, HWMaterial* _pMaterial, ShaderProgram* _shader,
                                     bool _transformToOrigin = true );


    WrappedEntity* CreateSkyBox(Context* _pContext, ShaderProgram* _pShader, HWMaterial* _pTexture);


    void    DrawSkyBox(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader);
    void    DrawMesh(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader);
    void    DrawIndexedMesh(HWRenderer* _pRenderer, WrappedEntity* _pEntity, HWMaterial* _pMat, ShaderProgram* _pShader);
  


    //////////////////////////////////////////////////////////////////////////
    //MeshBase declaration
    //////////////////////////////////////////////////////////////////////////
    class MeshBase : public DrawableBase
    {

        RT_OBJECT(MeshBase, DrawableBase)

    public:
        MeshBase( Context* _pContext );
        virtual ~MeshBase();
       
        virtual void        CreateVao() = 0;
        virtual bool        PostInitialize() { return true; }      

        void                SetMaterial(HWMaterial* _pMaterial);
        HWMaterial*         GetMaterial() const;

        void                SetShader(ShaderProgram* _pShader);
        ShaderProgram*      GetShader() const;                

        uint32_t            GetVaoId() const;
        uint32_t            GetIndexBufferId() const;
        uint32_t            GetVertexCount() const;
        uint32_t            GetIndicesCount() const;

    protected:        
      
        uint32_t            m_vaoId         = INVALID_ID;
        uint32_t            m_vertexCount   = INVALID_ID;
        uint32_t            m_indicesCount  = INVALID_ID;
        uint32_t            m_indexBufferId = INVALID_ID;
        HWMaterial*         m_pMaterial     = nullptr;
        ShaderProgram*      m_pShader       = nullptr;   
    };

}