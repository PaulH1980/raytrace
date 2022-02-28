#include "Context.h"
#include "HistoryItem.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
    //HistoryItem
    //////////////////////////////////////////////////////////////////////////
    HistoryItem::HistoryItem(Context* _pContext, HistoryFunc _pExec, HistoryFunc _pUndo, HistoryFunc _pRedo, const std::string& _name)
        : m_pContext(_pContext)
        , m_pExecFun(_pExec)
        , m_pUndoFun(_pUndo)
        , m_pRedoFun(_pRedo)
        , m_name(_name)
    {

    }

    HistoryItem::HistoryItem(Context* _pContext, const std::string _name /*= ""*/)
        : m_pContext(_pContext)
        , m_name(_name)
    {

    }

    HistoryItem::~HistoryItem()
    {

    }

    bool HistoryItem::Undo() 
    {
        return m_pUndoFun(m_pContext);
    }

    bool HistoryItem::Redo() 
    {
        if (m_pRedoFun)
            return m_pRedoFun(m_pContext);
        return m_pExecFun(m_pContext);
    }

    bool HistoryItem::Execute() 
    {
        return m_pExecFun(m_pContext);
    }

    const std::string& HistoryItem::GetItemName() const
    {
        return m_name;
    }

    Context& HistoryItem::GetContext() const
    {
        return *m_pContext;
    }

}

