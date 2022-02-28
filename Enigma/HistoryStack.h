#pragma once
#include <stack>
#include "SystemBase.h"
#include "HistoryItem.h"

namespace RayTrace
{
   
    
    class HistoryStack : public SystemBase
    {
        RT_OBJECT(HistoryStack, SystemBase)

    public:
        HistoryStack(Context* _pContext);

        bool Execute(HistoryFunc _pExec, HistoryFunc _pUndo, const std::string & _name);        
        
        bool Execute(HistoryItemPtr  _pItem);

        bool Clear() override;

        bool CanUndo() const;

        bool CanRedo() const;

        void ClearUndoStack();

        void ClearRedoStack();

        void Undo();

        void Redo();

    private:

        std::stack<HistoryItemPtr> m_undoStack;
        std::stack<HistoryItemPtr> m_redoStack;

    };
}