#pragma once
#include "Component.h"
#include "Components.h"
#include "Context.h"

#include "Entity.h"

namespace RayTrace
{


    class WrappedEntity : public ObjectBase
    {
        RT_OBJECT(WrappedEntity, ObjectBase)
    public:
        WrappedEntity(Context* _pContext);
        virtual ~WrappedEntity();

        ObjPropVector           GetProperties() override;

        WrappedEntity*          Clone() const override;

        const Entity&           GetEntity() const;

        const Matrix4x4&        GetWorldTransform() const;

        const Matrix4x4&        GetLocalTransform() const;

        BBox3f                  GetBoundingBox() const;

        Vector3f                GetPosition() const;

        Vector3f                GetIconColor() const;

        ProcessObjectFun        GetProcessFunction() const;

        HWMaterial*             GetMaterial() const;

        ShaderProgram*          GetShader() const;

        bool                    SetParent(EntityId _parent);

        bool                    UnParent();

        template<typename T>
        T*                      GetOrCreateComponent();

        template<typename T>
        bool                    HasComponent() const;

        template<typename... Targs>
        auto                    GetComponents() const;

        template<typename T>
        bool                    RemoveComponent();
     
        template<typename T>
        T*                      GetComponent() const;

        template<typename T>
        T*                      CreateComponent();

        template<typename... Targs>
        auto                    CreateComponents();


    protected:

        WrappedEntity(const WrappedEntity& _rhs);
        Entity          m_entity;            
    };

    template<typename T>
    T* WrappedEntity::GetOrCreateComponent()
    {
        auto result = GetComponent<T>();
        if (!result) {
            result = CreateComponent<T>();
            Initialize();
        }
        return result;
    }

	template<typename T>
	bool WrappedEntity::HasComponent() const
	{
		return GetComponent<T>() != nullptr;
	}

	template<typename... Targs>
	auto WrappedEntity::GetComponents() const
	{
		return GetContext().GetComponentSystem().GetComponents<Targs...>(m_entity);
	}

	template<typename T>
	bool WrappedEntity::RemoveComponent()
	{
		auto retval = GetContext().GetComponentSystem().RemoveComponent<T>(m_entity);
		if (retval)
			Initialize();
		return retval;
	}

	template<typename T>
	T* WrappedEntity::GetComponent() const
	{
		return GetContext().GetComponentSystem().GetComponent<T>(m_entity);
	}

	template<typename T>
	T* WrappedEntity::CreateComponent()
	{
		auto retval = GetContext().GetComponentSystem().CreateComponent<T>(m_entity);
		Initialize();
		return retval;
	}

	template<typename... Targs>
	auto WrappedEntity::CreateComponents()
	{
		auto retval = GetContext().GetComponentSystem().CreateComponents<Targs...>(m_entity);
		Initialize();
		return retval;
	}

	template<typename... Targs>
    WrappedEntity* CreateEntity(Context* _pContext)
    {
        auto* newEntity = new WrappedEntity(_pContext);
        newEntity->CreateComponents<Targs...>();
        return newEntity;
    }

    WrappedEntity* CreateGeometricPlane(Context* _pContext);

    WrappedEntity* CreateLightEntity(Context* _pContext);

    WrappedEntity* CreateAudioEntity(Context* _pContext);

    WrappedEntity* CreateCameraEntity(Context* _pContext);

    WrappedEntity* CreateMeshEntity(Context* _pContext);

    WrappedEntity* CreateIndexedMeshEntity(Context* _pContext);
    
}