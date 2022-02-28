#include "Context.h"
#include "Logger.h"
#include "EventHandler.h"

namespace RayTrace
{

    EventBase::EventBase() 
        : EventBase(eEvents::EVENT_UNDEFINED, 0.0f)        
    {

    }

    EventBase::EventBase(eEvents _type, float _time) 
        : m_eventType(_type)
        , m_eventTime(_time)
        , m_eventId(s_nextId++)
    {

    }

    std::atomic<uint32_t> EventBase::s_nextId = 1;

    EventHandler::EventHandler(Context* _pContext)
        : SystemBase( _pContext )
    {
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    }

    bool EventHandler::Clear()
    {
       m_registeredEvents.clear();
       m_eventMap = std::queue<EventBase>();
       return true;
    }

    bool EventHandler::RegisterSubscriber(eEvents _evtId, SubcriberPair _sub)
    {
        if (ContainsSubscriber(_evtId, _sub.first)) {
            assert(false && "Duplicate");
            return false;
        }

        auto& subMap = m_registeredEvents[_evtId];
        subMap[_sub.first] = _sub.second;
        return true;
    }

    bool EventHandler::RemoveSubscriber(eEvents _evtId, SubcriberPair _sub)
    {
        if (!ContainsSubscriber(_evtId, _sub.first)) {            
            return false;
        }
        auto& subMap = m_registeredEvents[_evtId];
        auto it = subMap.find(_sub.first);
        subMap.erase(it);        
        return true;
    }

    bool EventHandler::RemoveSubscriber(ObjectBase* _sub)
    {
        bool found = false;
        
        for (auto& [_evt, _map] : m_registeredEvents) {
            auto it = _map.find(_sub);
            if (it != std::end(_map)) {
                _map.erase(it);
                found = true;
            }
        }
        return found;
    }

    bool EventHandler::ContainsSubscriber(eEvents _evtId, const ObjectBase* _rec) const
    {
        //lookup eventId
        const auto it = m_registeredEvents.find(_evtId);
        if (it == std::end(m_registeredEvents))
            return false;
        //lookup subscriber map
        const auto& recMap = it->second;
        const auto subIt = recMap.find(_rec);
        return subIt != std::end(recMap);
    }

    void EventHandler::AddEvent(const EventBase _evt)
    {
        m_eventMap.push(_evt);
    }

    void EventHandler::AddEvents(const std::vector<EventBase>& _evts)
    {
        for (const auto& evt : _evts)
            m_eventMap.push(evt);
    }

    void EventHandler::PostEvents()
    {
        auto eventMap = m_eventMap;   
        m_eventMap = {};

        while (!eventMap.empty())
        {
            //fetch oldest event
            auto evt = eventMap.front(); eventMap.pop();
            auto type = evt.m_eventType;
            //event registered & containts callbacks?
            if( m_registeredEvents.find( type ) == std::end( m_registeredEvents) )
                continue; //no listeners found -> continue with next event;
           
            //loop through callbacks
            for (auto [key, val] : m_registeredEvents[type]) {
                val(evt); //TODO use key->func(evt) instead??
                if (evt.m_handled) //event handled by object ?
                    break; //done with this event
            }           
        }
    }

}

