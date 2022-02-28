#pragma once
#include "FrontEndDef.h"
#include "Color.h"
#include "ObjectBase.h"



namespace RayTrace
{
    template<typename T>
    struct ComponentBase
    {

    };
      
    struct  CameraComponent
    {
        RT_COMPONENT(CameraComponent)
        int   m_width  = 2;
        int   m_height = 2;
        bool  m_ortho  = false;
        float m_fov    = 45.0f;       
        float m_near   = 0.1f;
        float m_far    = 1000.0f;
    };

    struct HierarchicalComponent
    {
        RT_COMPONENT(HierarchicalComponent)        
        EntityId m_parent = INVALID_ID;
    };


    struct Color3fComponent
    {
        RT_COMPONENT(Color3fComponent)
        Color3f m_color;
    };


    struct Color4fComponent
    {
        RT_COMPONENT(Color4fComponent)
        Color4f m_color;
    };

	
    struct BBox2fComponent
    {
        RT_COMPONENT(BBox2fComponent)
        BBox2f m_bounds;
    };	


    struct BBox2iComponent
    {
        RT_COMPONENT(BBox2iComponent)
        BBox2i m_bounds;
    };


    struct BBox3fComponent
    {
        RT_COMPONENT(BBox3fComponent)
        BBox3f m_bounds;
    };


    struct BBox3iComponent
    {
        RT_COMPONENT(BBox3iComponent)
        BBox3i m_bounds;
    };
	

    struct TransformComponent
    {
        RT_COMPONENT(TransformComponent)
        Matrix4x4 m_transform;
    };

    struct LocalTransformComponent
    {
        RT_COMPONENT(LocalTransformComponent)
        Matrix4x4 m_local;
    };


    struct Position2fComponent
    {
        RT_COMPONENT(Position2fComponent)
        Vector2f m_pos;
    };


    struct Position2iComponent
    {
        RT_COMPONENT(Position2iComponent)
        Vector2i m_pos;
    };


    struct Position3fComponent
    {
        RT_COMPONENT(Position3fComponent)
        Vector3f m_pos;
    };


    struct Position3iComponent
    {
        RT_COMPONENT(Position3iComponent)
        Vector3i m_pos;
    };
	

    struct Position4fComponent
    {
        RT_COMPONENT(Position4fComponent)
        Vector4f m_pos;
    };
	

    struct Position4iComponent
    {
        RT_COMPONENT(Position4iComponent)
        Vector4i m_pos;
    };



    struct LightComponent
    {
        RT_COMPONENT(LightComponent)
        Vector4f    m_color     = { 0.5f, 0.5f, 0.f, 2.f };
        Vector4f    m_angles    = { 25.f, 45.f, 0.0f, 0.0f };
        Vector2f    m_area      = { 1.f, 1.f };
        uint32_t    m_flags     = { 0 };
        uint32_t    m_id        = INVALID_ID;
        eLightType  m_type      = eLightType::POINT_LIGHT;
    };
	

    struct EditorComponent
    {
        RT_COMPONENT( EditorComponent)
        std::string m_objectName;
        std::string m_groupName;
        std::string m_uuid;
        ObjectFlags m_objectFlags = { 0 };
    };

    struct EditorGeometricComponent
    {
        RT_COMPONENT(EditorGeometricComponent)

        EditorGeometricComponent(){}
        EditorGeometricComponent(const EditorGeometricComponent& _rhs);
        EditorGeometricComponent(EditorGeometricComponent&& _rhs);
        ~EditorGeometricComponent();

        EditorGeometricComponent& operator =(const EditorGeometricComponent& _rhs);
        EditorGeometricComponent& operator =(EditorGeometricComponent&& _rhs);

        void PostConstructor(Context* _pContext);

        std::unique_ptr<ModifierStack> m_pModStack;
        Context* m_pContext = nullptr;

    };

    struct EditorSpriteComponent
    {
        RT_COMPONENT(EditorSpriteComponent)
        Vector3f        m_coords[4];
        Vector2f        m_uvs[4];
        Vector2f        m_scale = { 1.0f, 1.0f };
        uint32_t        m_spriteFlags = { 0 }; //alignment etc
        Color3f         m_spriteColor = { 1.f, 1.f, 1.f };
        TextureSlot     m_texId = {nullptr};
    };


     struct SpriteComponent
     {
        RT_COMPONENT(SpriteComponent)
        Vector3f    m_coords[4];
        Vector2f    m_uvs[4];
        Vector2f    m_scale = { 1.0f, 1.0f };
        uint32_t    m_spriteFlags = { 0 }; //alignment etc
     };	

        
     struct IndexedMeshComponent
     {
         RT_COMPONENT(IndexedMeshComponent)
         uint32_t m_vaoId = { INVALID_ID };
         uint32_t m_indexBufferId = { INVALID_ID };
         uint32_t m_instanceIdd = { INVALID_ID };
         uint32_t m_numIndices = { 0 };
         ProcessObjectFun m_pDrawMeshFun = { nullptr };
     };
   

    struct MeshComponent
    {
        RT_COMPONENT(MeshComponent)        
        uint32_t m_vaoId = { INVALID_ID };
        uint32_t m_instanceIdd = { INVALID_ID };
        uint32_t m_numVertices= { 0 };
        ProcessObjectFun m_pDrawMeshFun = { nullptr };
    };


    struct SkyBoxComponent
    {
        RT_COMPONENT(SkyBoxComponent)        
        uint32_t    m_vaoId = { INVALID_ID };
        uint32_t    m_vertexCount = { INVALID_ID };
        ProcessObjectFun m_pDrawSkyBoxFun = { nullptr };    
    };


    struct RenderStateComponent
    {
        RT_COMPONENT(RenderStateComponent)
        RenderState* m_pRenderState = { nullptr };
    };


    struct TextureComponent
    {
        RT_COMPONENT(TextureComponent)
        TextureSlot m_texture;        
    };


    struct MaterialComponent
    {
        RT_COMPONENT(MaterialComponent)             
        MaterialSlot m_material;
    };

   
    struct ShaderComponent
    {
        RT_COMPONENT(ShaderComponent)
        ShaderSlot m_shader;
    };

	
    struct AudioComponent
    {
        RT_COMPONENT(AudioComponent)
        FileNameSlot m_fileName;
        int         m_repeatCount = { -1 };
        float       m_attenuation = { 1.0f };    
    };  


    PropertyData* GetProperties(WrappedEntity* _pObj, AudioComponent* _pComp);    
	PropertyData* GetProperties(WrappedEntity* _pObj, CameraComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, HierarchicalComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Color3fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Color4fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, BBox2fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, BBox2iComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, BBox3fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, BBox3iComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, TransformComponent* _pComp);
    PropertyData* GetProperties(WrappedEntity* _pObj, LocalTransformComponent* _pComp);
    PropertyData* GetProperties(WrappedEntity* _pObj, Position2fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Position2iComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Position3fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Position3iComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Position4fComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, Position4iComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, LightComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, SpriteComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, MaterialComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, ShaderComponent* _pComp);
	PropertyData* GetProperties(WrappedEntity* _pObj, TextureComponent* _pComp);
}


