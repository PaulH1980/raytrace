#include "Context.h"
#include "WrappedEntity.h"
#include "Logger.h"
#include "HWMesh.h"
#include "EventHandler.h"
#include "SceneView.h"
#include "ResourceManager.h"
#include "InputHandler.h"
#include "Controller.h"
#include "SceneManager.h"

namespace RayTrace
{

    std::vector<WrappedEntity*> FilterObjects(const  EntityVector& _objects, EntityFilter _filter /*= nullptr*/)
    {
        std::vector<WrappedEntity*> ret;
        for (auto& obj : _objects) {
            bool add = true;
            if (_filter)
                add = _filter(obj);
            if (add)
                ret.push_back(obj);
        }
        return  ret;
    }


    void SelectObjects(const EntityVector& _objects, bool _selected)
    {
        for (auto _obj : _objects)
            _obj->GetObjectFlags().m_field.m_selected = _selected;
    }


	RayTrace::UUIDVector GetUuids(const EntityVector& _objects)
	{
        UUIDVector retval;
        retval.reserve(_objects.size());
        for (const auto& obj : _objects)
        {
            retval.push_back(obj->GetUUID());
        }
        return retval;


	}

	SceneManager::SceneManager(Context* _pContext)
        : SystemBase( _pContext )
    {
        GetContext().GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
        m_objectVector.reserve(100);
    }

    bool SceneManager::PostConstructor()
    {
       
        return true;
    }

    bool SceneManager::Clear()
    {
        m_modBeginSize = m_modEndSize = 0;
        m_pActiveView = nullptr;
        m_objectMap.clear();
        m_objectVector.clear();
        return true;
    }

	EntityVector SceneManager::GetEntities(const UUIDVector& _names) const
	{
        EntityVector retval;
        for (const auto& name : _names)
            retval.push_back(GetObject(name));
        return retval;
	}

	WrappedEntity* SceneManager::GetObject(const std::string& _name) const
    {
        if (!ContainsObject(_name)) {
            assert(false && "Entity Not Found");
            return nullptr;
        }
        auto objIdx = m_objectMap.at(_name);
        return m_objectVector[objIdx].get();
    }

    bool SceneManager::AddObject(const std::string& _name, WrappedEntityUPtr _object)
    {
        if (ContainsObject(_name))
            return false; //duplicate name
        const auto objCount = GetObjectCount();        
        m_objectVector.push_back(std::move(_object));
        m_objectMap[_name] = objCount;
        return true;
    }

    bool SceneManager::ContainsObject(const std::string& _name) const
    {
        return m_objectMap.find(_name) != std::end(m_objectMap);
    }

    WrappedEntityUPtr SceneManager::RemoveObject(const std::string& _name)
    {
        if (!ContainsObject(_name))
            return nullptr;
        const auto objIdx  = m_objectMap[_name];
        const auto lastIdx = GetObjectCount() - 1;
    
        //swap objIdx with lastIdx...    
        std::swap(m_objectVector[objIdx], m_objectVector[lastIdx]);
        const auto& uuid = m_objectVector[objIdx]->GetUUID();
        m_objectMap[uuid] = objIdx;
        //move entity
        auto pEntity = std::move(m_objectVector.back());
        //erase
        m_objectVector.pop_back();
        m_objectMap.erase(_name);
        assert(m_objectVector.size() == m_objectMap.size());
        return pEntity;
    }

    //WrappedEntityUPtr SceneManager::MoveObject(const std::string& _name)
    //{
    //    if (!ContainsObject(_name))
    //        return nullptr;

    //    const auto objIdx = m_objectMap[_name];
    //    auto retVal = std::move(m_objectVector[objIdx]);
    //    if (!RemoveObject(_name))
    //        return nullptr;
    //    return retVal;
    //}

    uint32_t SceneManager::GetObjectCount() const
    {
        return m_objectVector.size();
    }

    std::vector<WrappedEntity*> SceneManager::GetObjects( EntityFilter _filter) const
    {
        std::vector<WrappedEntity*> ret;
        for (auto& obj : m_objectVector) {
            bool add = true;
            if (_filter)
                add = _filter(obj.get());
            if( add )
                ret.push_back(obj.get());
        }
        return  ret;
    }

    void SceneManager::BeginModification()
    {
        m_modBeginSize = m_objectVector.size();
    }

    void SceneManager::EndModification()
    {
        m_modEndSize = m_objectVector.size();
    }

    BBox3f SceneManager::GetSceneBounds() const
    {
        auto IsVisible = [](WrappedEntity* _pObj)
        {
            return _pObj->GetObjectFlags().m_field.m_visible != 0;
        };
        
        auto objects = GetObjects(IsVisible); 

        BBox3f ret;
        for (auto& obj : objects)
        {
            const auto bounds = obj->GetBoundingBox();
            if (bounds.volume() > 0.0f)
                ret = Union(ret, bounds);
        }
        return ret;

    }

    void SceneManager::SetActiveSceneView(SceneView* _pView)
    {
        if (m_pActiveView != _pView) {
            m_pActiveView = _pView;
            //GetContext().GetInputHandler().AddReceiver(_pView->m_pController.get());
        }
    }

    SceneView* SceneManager::GetActiveSceneView() const
    {
        return m_pActiveView;
    }

  
   

}

