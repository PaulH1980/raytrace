#pragma once
#include "EventBase.h"
#include "Context.h"
#include "FrontEndDef.h"

namespace RayTrace
{
    using HistoryFunc = std::function<bool(Context*)>;

    class HistoryItem
    {
    public:

        HistoryItem(Context* _pContext, HistoryFunc _exec, HistoryFunc _pUndo, HistoryFunc _pRedo = nullptr, const std::string& _name = "");
        HistoryItem(Context* _pContext, const std::string _name = "");

        virtual ~HistoryItem();

        virtual bool Undo() ;
        virtual bool Redo();
        virtual bool Execute();

        const std::string& GetItemName() const;

        Context& GetContext() const;

    protected:
        Context* m_pContext = nullptr;
        HistoryFunc m_pExecFun = nullptr;
        HistoryFunc m_pUndoFun = nullptr;
        HistoryFunc m_pRedoFun = nullptr;

        std::string m_name;
    };




    using HistoryItemPtr = std::unique_ptr<HistoryItem>;
}