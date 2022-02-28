#pragma once
#include "EventBase.h"
#include "Context.h"
#include "WrappedEntity.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "CopyPaste.h"
#include "HistoryItem.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
 // SelectionHistory
 //////////////////////////////////////////////////////////////////////////
    class SelectionHistory : public HistoryItem
    {
    public:
        SelectionHistory(Context* _pContext,
            const  UUIDVector& _exec, const  UUIDVector& _unExec);

        bool Undo() override;

        bool Redo() override;

        bool Execute() override;
    private:
        bool Apply(const   UUIDVector& _objects) ;
        UUIDVector m_execute;
        UUIDVector m_unExectute;
    };


    struct Transformable
    {
      
        UUID           m_entityUuid;
        Matrix4x4      m_startTransform;
        WrappedEntity* m_pEntity = nullptr;
    };


    class TransformHistory : public HistoryItem
    {
    public:
        TransformHistory(Context* _pContext,
            const std::vector<Transformable>& _exec, const std::vector<Transformable>& _unExec);

        bool Undo() override;

        bool Redo() override;

        bool Execute() override;

    private:
        bool Apply(const std::vector<Transformable>& _objects);

        std::vector<Transformable> m_execute;
        std::vector<Transformable> m_unExectute;
    };

    using ToolSubTool = std::pair<std::string, std::string>;

    

    class ActivateToolHistory : public HistoryItem
    {
    public:
        ActivateToolHistory(Context* _pContext,
           const ToolSubTool& _exec, const ToolSubTool& _unExec);

        bool Undo() override;

        bool Redo() override;

        bool Execute() override;

    private:
        bool Apply(const ToolSubTool& _tool) ;

        ToolSubTool m_exec;
        ToolSubTool m_unExec;
    };

    /*
        @brief: Copy entities and add to scenemanager
    */
    class AddObjectsHistory : public HistoryItem
    {
    public:

        AddObjectsHistory(Context* _pContext, const EntityVector& _entities, bool _isCloned = false );

        bool Undo() override;

        bool Redo() override;

        bool Execute() override;

    private:
        std::vector<WrappedEntityUPtr> m_entities;
        UUIDVector                     m_entityUuids;
    };
       

    class DeleteObjectsHistory : public HistoryItem
    {
    public:
        DeleteObjectsHistory(Context* _pContext, const  EntityVector& _entities);

        bool Undo() override;
        bool Redo() override;
        bool Execute() override;
    private:

        std::vector<WrappedEntityUPtr> m_deletedItems;
        UUIDVector                     m_entitieNames;        
    };

   


  
}