#pragma once
#include <variant>
#include <string>
#include <map>
#include <atomic>
#include <queue>

#include "SystemBase.h"
#include "EventBase.h"

namespace RayTrace
{
   
   
    using SubcriberFunc    = std::function<void(EventBase&)>;
    using SubcriberPair    = std::pair<const ObjectBase*, SubcriberFunc>; //use void?

    using SubscriberMap    = std::map<const ObjectBase*,SubcriberFunc>;
    using EventsMap        = std::map<eEvents, SubscriberMap>;
    

    //////////////////////////////////////////////////////////////////////////
    //EventHandler Declaration
    //////////////////////////////////////////////////////////////////////////
    class EventHandler : public SystemBase
    {

        RT_OBJECT(EventHandler, SystemBase)
    public:


        EventHandler(Context* _pContext);

        bool Clear() override;

        bool RegisterSubscriber(eEvents _evtId, SubcriberPair _sub);
        bool RemoveSubscriber(eEvents _evtId, SubcriberPair _sub);
        bool ContainsSubscriber(eEvents _evtId, const ObjectBase* _rec) const;
        bool RemoveSubscriber(ObjectBase* _sub);

        void AddEvent(const EventBase _evt);
        void AddEvents(const std::vector<EventBase>& _evts);
        void PostEvents();

        

    private:
        EventsMap               m_registeredEvents;
        std::queue<EventBase>   m_eventMap;
    };
}