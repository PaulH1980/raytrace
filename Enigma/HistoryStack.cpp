#include "Context.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Selection.h"
#include "WrappedEntity.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "HistoryStack.h"

namespace RayTrace
{
    
    
    //////////////////////////////////////////////////////////////////////////
    //HistoryStack
    //////////////////////////////////////////////////////////////////////////  
    HistoryStack::HistoryStack(Context* _pContext) : SystemBase(_pContext)
    {
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    }

    bool HistoryStack::Execute(HistoryItemPtr  _pItem)
    {
        ClearRedoStack();       
        const auto retVal = _pItem->Execute();
        m_undoStack.push(std::move(_pItem));
        return retVal;
    }

    bool HistoryStack::Execute( HistoryFunc _pExec, HistoryFunc _pUndo, const std::string& _name)
    {
        return Execute( std::make_unique<HistoryItem>( &GetContext(), _pExec, _pUndo, nullptr, _name ) );
    }

    bool HistoryStack::Clear()
    {
        ClearUndoStack();
        ClearRedoStack();
        return true;
    }
   

    bool HistoryStack::CanUndo() const
    {
        return !m_undoStack.empty();
    }

    bool HistoryStack::CanRedo() const
    {
        return !m_redoStack.empty();
    }

    void HistoryStack::ClearUndoStack()
    {
        m_undoStack = std::stack<HistoryItemPtr>();
    }

    void HistoryStack::ClearRedoStack()
    {
        m_redoStack = std::stack<HistoryItemPtr>();
    }

    void HistoryStack::Undo()
    {
        assert(CanUndo());
         
        m_undoStack.top()->Undo();
        m_redoStack.push( std::move(m_undoStack.top()));
        m_undoStack.pop();
    }

    void HistoryStack::Redo()
    {
        assert(CanRedo());

        m_redoStack.top()->Redo();
        m_undoStack.push(std::move(m_redoStack.top()));
        m_redoStack.pop();
    }
}

