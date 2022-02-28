#include "SceneManager.h"
#include "Selection.h"
#include "Tools.h"
#include "HistoryItems.h"

namespace RayTrace
{
    SelectionHistory::SelectionHistory(Context* _pContext, const UUIDVector& _exec, const  UUIDVector& _unExec) 
        : HistoryItem(_pContext, "Object Selection")
        , m_execute(_exec)
        , m_unExectute(_unExec)
    {

    }

    bool SelectionHistory::Undo() 
    {
        return Apply(m_unExectute);
    }

    bool SelectionHistory::Redo()
    {
        return Execute();
    }

    bool SelectionHistory::Execute()
    {
        return Apply(m_execute);
    }

    bool SelectionHistory::Apply(const UUIDVector& _objects)
    {
        SelectObjects(m_pContext->GetSceneManager().GetObjects(), false);

        for (auto& _obj : _objects) {
            auto pEnt = GetContext().GetSceneManager().GetObject(_obj);
            pEnt->GetObjectFlags().m_field.m_selected = true;
        }
        auto entUuids = GetUuids(m_pContext->GetSceneManager().GetObjects(SelectObjectFilter));
        SelectionModified(m_pContext, entUuids);
        return true;
    }



    TransformHistory::TransformHistory(Context* _pContext,
        const std::vector<Transformable>& _exec, const std::vector<Transformable>& _unExec)
        : HistoryItem(_pContext, "Object Transform")
        , m_execute(_exec)
        , m_unExectute(_unExec)
    {

    }

    bool TransformHistory::Undo()
    {
        return Apply(m_unExectute);
    }

    bool TransformHistory::Redo() 
    {
        return Execute();
    }

    bool TransformHistory::Execute()
    {
        return Apply(m_execute);
    }

    bool TransformHistory::Apply(const std::vector<Transformable>& _objects)
    {
        UUIDVector transformed;
        
        for (const auto& obj : _objects)
        {
            auto pEnt = GetContext().GetSceneManager().GetObject(obj.m_entityUuid);
            auto transComp = pEnt->GetComponent<TransformComponent>();
           
            transComp->m_transform = obj.m_startTransform;
            transformed.push_back(obj.m_entityUuid);
        }

        EventBase evt(eEvents::EVENT_OBJECT_TRANSFORM_CHANGED, m_pContext->GetElapsedTime());
        evt.m_data["changedObjects"] = transformed;
        m_pContext->AddEvent(evt);
        return true;
    }

    ActivateToolHistory::ActivateToolHistory(
        Context* _pContext, const ToolSubTool& _exec, const ToolSubTool& _unExec)
        : HistoryItem(_pContext, "Activate Tool")
        , m_exec(_exec)
        , m_unExec(_unExec)
    {

    }

    bool ActivateToolHistory::Undo()
    {
        return Apply(m_unExec);
    }

    bool ActivateToolHistory::Redo()
    {
        return Execute();
    }

    bool ActivateToolHistory::Execute()
    {
        return Apply(m_exec);
    }

    bool ActivateToolHistory::Apply( const ToolSubTool& _tool)
    {
        auto& toolMan = m_pContext->GetToolManager();
        const auto& [tool, subTool] = _tool;
        if (tool.empty())
            return toolMan.DeactivateTool();
        return toolMan.ActivateTool(tool, subTool);
    }

    DeleteObjectsHistory::DeleteObjectsHistory(Context* _pContext, const EntityVector& _entities)
        : HistoryItem( _pContext, "Delete Items")      
    {
        for (auto pEnt : _entities)
            m_entitieNames.push_back(pEnt->GetUUID());
    }

    bool DeleteObjectsHistory::Undo() 
    {
        bool valid = true;
        
        //EntityVector entitiesAdded;
        UUIDVector entUuids;
        for (auto& obj : m_deletedItems) {
            //auto pNaked = obj.get();
            //entitiesAdded.push_back(pNaked);
            entUuids.push_back(obj->GetUUID());
            valid &= m_pContext->GetSceneManager().AddObject(entUuids.back(), std::move(obj));
           
        }
        m_deletedItems.clear();
        assert(valid);

        EventBase evt(eEvents::EVENT_OBJECTS_ADDED, m_pContext->GetElapsedTime());
        evt.m_data["entities"] = entUuids;
        m_pContext->AddEvent(evt);

        SelectionModified(&GetContext(), GetUuids( m_pContext->GetSceneManager().GetObjects(SelectObjectFilter)));

        return true;
    }

    bool DeleteObjectsHistory::Redo()
    {
        return Execute();
    }

    bool DeleteObjectsHistory::Execute()
    {
        m_deletedItems.reserve(m_entitieNames.size());
        
        for (const auto& _name : m_entitieNames)
        {
            m_deletedItems.push_back(m_pContext->GetSceneManager().RemoveObject(_name));
            assert(m_deletedItems.back() != nullptr);
        }

        EventBase evt(eEvents::EVENT_OBJECTS_REMOVED, m_pContext->GetElapsedTime());
        evt.m_data["entityNames"] = m_entitieNames;
        m_pContext->AddEvent(evt);

        SelectionModified(&GetContext(), GetUuids( m_pContext->GetSceneManager().GetObjects(SelectObjectFilter)));
        return true;
    }

  

    AddObjectsHistory::AddObjectsHistory(Context* _pContext, const EntityVector& _entities, bool _isCloned )
        : HistoryItem(_pContext, "Copy Objects")
        //, m_entities( CopyEntitiesUPtr( _entities ) )
    {
        if (!_isCloned)
            m_entities = CopyEntitiesUPtr(_entities);
        else
        {
            for (auto pEnt : _entities)
                m_entities.push_back(WrappedEntityUPtr(pEnt));           
        }

    }

    bool AddObjectsHistory::Undo()
    {
        for (const auto& uuid : m_entityUuids)
        {
            auto pEnt = std::move(GetContext().GetSceneManager().RemoveObject(uuid));
            assert(pEnt);
            m_entities.push_back(std::move(pEnt));
        }

        EventBase evt(eEvents::EVENT_OBJECTS_REMOVED, m_pContext->GetElapsedTime());
        evt.m_data["entityNames"] = m_entityUuids;
        m_pContext->AddEvent(evt);

        SelectionModified(&GetContext(), GetUuids( m_pContext->GetSceneManager().GetObjects(SelectObjectFilter)));
        m_entityUuids.clear();
        return true;
    }

    bool AddObjectsHistory::Redo()
    {
        return Execute();
    }

    bool AddObjectsHistory::Execute()
    {
        bool valid = true;
        //EntityVector entitiesAdded;
        for (auto& pEnt : m_entities )
        {
            const auto& uuid = pEnt->GetUUID();
           // entitiesAdded.push_back(pEnt.get());
            m_entityUuids.push_back(uuid);
            valid &= GetContext().GetSceneManager().AddObject(uuid, std::move( pEnt ) );
            assert(valid);          
        }
        m_entities.clear();


        EventBase evt(eEvents::EVENT_OBJECTS_ADDED, m_pContext->GetElapsedTime());
        evt.m_data["entities"] = m_entityUuids;
        m_pContext->AddEvent(evt);

        SelectionModified(&GetContext(), GetUuids(m_pContext->GetSceneManager().GetObjects(SelectObjectFilter)));

        return true;
        
    }

}

