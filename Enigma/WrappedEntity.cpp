#include <iostream>
#include "Context.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "ResourceManager.h"
#include "WrappedEntity.h"

namespace RayTrace
{
    WrappedEntity::WrappedEntity(Context* _pContext)
        : ObjectBase(_pContext)
        , m_entity(Entity(_pContext->GetNextEntityId()))
    {
        m_name = std::string("Entity_0") + std::to_string(m_entity.GetId());    
    }

    WrappedEntity::WrappedEntity(const WrappedEntity& _rhs)
        : ObjectBase( _rhs )
        , m_entity(Entity(_rhs.GetContext().GetNextEntityId()))
    {
        m_name = std::string("Entity_0") + std::to_string( m_entity.GetId() );
        GetContext().GetComponentSystem().Clone(_rhs.GetEntity(), m_entity );    
        Initialize();
    }

    WrappedEntity::~WrappedEntity()
    {
        AddLogMessage(&GetContext(), "destructor");
        ClearProperties();
    }

    ObjPropVector WrappedEntity::GetProperties()
    {
        auto parentProps = ObjectBase::GetProperties();
        
       // if (m_modified) 
        {

           auto entities =  GetContext().GetComponentSystem().GetComponents<  
                LightComponent,
                SpriteComponent,
                AudioComponent,
                TextureComponent,
                ShaderComponent,
                MaterialComponent,
                CameraComponent,
                Color3fComponent,
                Color4fComponent,
                TransformComponent,
                LocalTransformComponent,
                Position2fComponent,
                Position2iComponent,
                Position3fComponent,
                Position3iComponent,
                Position4fComponent,
                Position4iComponent               
                
           >( m_entity );

           auto AddToCompVector = [&](auto&& _pComp)
           {
               if (_pComp) {
                   auto childProps =  RayTrace::GetProperties(this, _pComp);
                   parentProps.push_back(std::move(childProps));
               }
           };
           std::apply([&](auto&&... args) {((AddToCompVector(args)), ...); }, entities);
        }      
        
        return parentProps;
    }

    WrappedEntity* WrappedEntity::Clone() const
    {
        WrappedEntity* pEntity = new WrappedEntity(*this);    
        if (auto pComp = pEntity->GetComponent<EditorGeometricComponent>())
            pComp->PostConstructor(&GetContext());       
        return pEntity;
    }

    const Entity& WrappedEntity::GetEntity() const
    {
        return m_entity;
    }

    BBox3f WrappedEntity::GetBoundingBox() const
    {
        if (auto bbComp = GetComponent<BBox3fComponent>())
            return bbComp->m_bounds;
        if (auto pos3Comp = GetComponent<Position3fComponent>())
            return BBox3f(pos3Comp->m_pos, pos3Comp->m_pos);
        if (auto bbComp = GetComponent<BBox2fComponent>())
            return BBox3f(ToVector3(bbComp->m_bounds.m_min), 
                          ToVector3(bbComp->m_bounds.m_max));
        if (auto lightComp = GetComponent<LightComponent>()) {
            auto lightRadius = lightComp->m_color.w;
            const Vector3f minmax(lightRadius * 0.5f);
            return BBox3f(minmax * -1.0f, minmax);
        }
        return BBox3f( Vector3f(0.0f), Vector3f(0.0f));        
    }

    Vector3f WrappedEntity::GetPosition() const
    {
        if (auto transComp = GetComponent<TransformComponent>())
            return ToVector3(transComp->m_transform * Vector4f(0.f, 0.f, 0.f, 1.0f));
        else if (auto posComp = GetComponent<Position3fComponent>())
            return posComp->m_pos;
        else if (auto posComp = GetComponent<Position3iComponent>())
            return posComp->m_pos;
        else if (auto posComp = GetComponent<Position2fComponent>())
            return ToVector3(posComp->m_pos);
        else if (auto posComp = GetComponent<Position2iComponent>())
            return ToVector3(posComp->m_pos);
        return Vector3f(0.f);
    }

    Vector3f WrappedEntity::GetIconColor() const
    {
        if (auto lightComp = GetComponent<LightComponent>())
            return ToVector3(lightComp->m_color);
        else if (auto spriteComp = GetComponent<EditorSpriteComponent>())
            return spriteComp->m_spriteColor.m_rgb;
        else return Vector3f(1.0f);
    }

    ProcessObjectFun WrappedEntity::GetProcessFunction() const
    {
        if (auto comp = GetComponent<MeshComponent>()) //TODO combine into one component?
            return comp->m_pDrawMeshFun;
        if (auto comp = GetComponent<IndexedMeshComponent>())
            return comp->m_pDrawMeshFun;
        if (auto comp = GetComponent<SkyBoxComponent>())
            return comp->m_pDrawSkyBoxFun;
        return nullptr;
    }

    HWMaterial* WrappedEntity::GetMaterial() const
    {
        if (auto comp = GetComponent<MaterialComponent>())
            return comp->m_material.m_pMaterial;
        assert(false && "Material Not Found");
        return GetContext().GetResourceManager().GetMaterial("__DefaultNullMaterial");
    }

    ShaderProgram* WrappedEntity::GetShader() const
    {
        if (auto comp = GetComponent<ShaderComponent>())
            return comp->m_shader.m_pShader;
        assert(false && "Shader Not Found");
        return GetContext().GetResourceManager().GetShader("__DefaultShader");
    }

    bool WrappedEntity::SetParent(EntityId _parent)
    {
        auto locComp = GetOrCreateComponent<LocalTransformComponent>();
        auto comp = GetOrCreateComponent<HierarchicalComponent>();
        assert(comp);

        if (comp->m_parent != _parent) {
            comp->m_parent = _parent;
        }
        return locComp != nullptr && comp != nullptr;
    }

    bool WrappedEntity::UnParent()
    {
        bool valid = true;
        if (HasComponent<HierarchicalComponent>())
            valid &= RemoveComponent<HierarchicalComponent>();
        if (HasComponent<LocalTransformComponent>())
            valid &= RemoveComponent<LocalTransformComponent>();
        return valid;
    }

    const Matrix4x4& WrappedEntity::GetWorldTransform() const
    {
        if (auto trans = GetComponent<TransformComponent>())
            return trans->m_transform;
        return Identity4x4;
    }

    const Matrix4x4& WrappedEntity::GetLocalTransform() const
    {
        if (auto trans = GetComponent<LocalTransformComponent>())
            return trans->m_local;
        return Identity4x4;
    }

    WrappedEntity* CreateGeometricPlane(Context* _pContext)
    {
        auto ent = CreateEntity<EditorGeometricComponent, MeshComponent, MaterialComponent, ShaderComponent, TransformComponent, BBox3fComponent>(_pContext);
        auto pComp = ent->GetComponent<EditorGeometricComponent>();
        pComp->PostConstructor(_pContext);
        pComp->m_pModStack->AddModifier(std::make_unique<PlaneModifier>(_pContext));        
        return ent;
    }

    WrappedEntity* CreateLightEntity(Context* _pContext)
    {
        auto ent = CreateEntity<LightComponent, TransformComponent, EditorSpriteComponent>(_pContext);
        auto sprite = ent->GetComponent<EditorSpriteComponent>();
        sprite->m_scale = { 64.0f, 64.0f };
        sprite->m_texId.m_pTexture = _pContext->GetResourceManager().GetTexture("LightIcon");
        return ent;
    }

    WrappedEntity* CreateAudioEntity(Context* _pContext)
    {
        return CreateEntity<AudioComponent, Position3fComponent, BBox3fComponent, EditorSpriteComponent>(_pContext);
    }

    WrappedEntity* CreateCameraEntity(Context* _pContext)
    {
        return CreateEntity<CameraComponent, TransformComponent>(_pContext);
    }

    WrappedEntity* CreateMeshEntity(Context* _pContext)
    {
        return CreateEntity<MeshComponent, MaterialComponent, ShaderComponent, TransformComponent, BBox3fComponent>(_pContext);
    }

    WrappedEntity* CreateIndexedMeshEntity(Context* _pContext)
    {
        return CreateEntity<IndexedMeshComponent, MaterialComponent, ShaderComponent, TransformComponent, BBox3fComponent>(_pContext);
    }

}

