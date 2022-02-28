#include "Context.h"
#include "Logger.h"
#include "Transform.h"
#include "Components.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "Component.h"

namespace RayTrace
{
    
    ComponentSystem::ComponentSystem(Context* _pContext) 
        : SystemBase(_pContext)
    {
        GetContext().GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());       
    }

    bool ComponentSystem::PostConstructor()
    {
        bool valid = true;
        valid &= Register<CameraComponent>();
        valid &= Register<HierarchicalComponent>();
        valid &= Register<Color3fComponent>();
        valid &= Register<Color4fComponent>();
        valid &= Register<BBox3fComponent>();
        valid &= Register<BBox3iComponent>();
        valid &= Register<BBox2fComponent>();
        valid &= Register<BBox2iComponent>();
        valid &= Register<TransformComponent>();
        valid &= Register<LocalTransformComponent>();      
        valid &= Register<Position3fComponent>();  
        valid &= Register<Position3iComponent>();       
        valid &= Register<Position2fComponent>();
        valid &= Register<Position2iComponent>();
        valid &= Register<Position4fComponent>();
        valid &= Register<Position4iComponent>();
        valid &= Register<LightComponent>();
        valid &= Register<SpriteComponent>();
        valid &= Register<EditorSpriteComponent>();
        valid &= Register<AudioComponent>();
        valid &= Register<EditorComponent>();
        valid &= Register<MeshComponent>();
        valid &= Register<IndexedMeshComponent>();
        valid &= Register<TextureComponent>();
        valid &= Register<ShaderComponent>();
        valid &= Register<MaterialComponent>();
        valid &= Register<EditorGeometricComponent>();
     
        assert(valid);
        return valid;
    }

    bool ComponentSystem::Clear()
    {
        m_components.clear();
        return true;
    }

    bool ComponentSystem::Clone(Entity _srcEnt, Entity _dstEnt)
    {
        bool valid = true;
        for (auto& [key, compMan] : m_components) {
            if (compMan->ContainsEntity(_srcEnt))
                valid &= compMan->Clone(_srcEnt, _dstEnt );            
        }
        return valid;
    }

    bool ComponentSystem::Remove(Entity _ent)
    {
        bool valid = true;
        for (auto& [key, compMan] : m_components) {
            if (compMan->ContainsEntity(_ent))
                valid &= compMan->Remove(_ent);
        }
        return valid;
    }

    ComponentManagerBase* ComponentSystem::GetManager(const std::string& _name) const
    {
        if (m_components.find(_name) == std::end(m_components))
            return nullptr;
        return m_components.at(_name).get();
    }

    bool ComponentSystem::Contains(const std::string& _name) const
    {
        return GetManager(_name) != nullptr;
    }

    

    ComponentManagerBase::ComponentManagerBase(Context* _pContext) : m_pContext(_pContext)
    {

    }

}

