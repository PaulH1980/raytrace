#include "Context.h"
#include "Logger.h"

namespace RayTrace
{

    Logger::Logger(Context* _pContext) : SystemBase(_pContext)
    { 
        AddMessage("Created " + GetTypeNameStatic() );
    }

    bool Logger::Clear()
    {
        {
            std::lock_guard<std::mutex> lock(m_recMutex);
            m_recipients.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_msgMutex);
            m_messages.clear();
        }
        return true;
       
    }

    bool Logger::AddRecipient(const std::string& _name, const std::shared_ptr<LogRecipient>& _recipient)
    {
        std::lock_guard<std::mutex> lock(m_recMutex);
        if (m_recipients.find(_name) != std::end(m_recipients))
            return false;
        m_recipients[_name] = _recipient;
        return true;

    }

    bool Logger::RemoveRecipient(const std::string& _name)
    {
        std::lock_guard<std::mutex> lock(m_recMutex);
        auto it = m_recipients.find(_name);
        if (it == std::end(m_recipients))
            return false;
        m_recipients.erase(it);
        return true;
    }

    void Logger::AddMessage(const std::string& _msg, eLogLevel _level /*= LOG_LEVEL_INFO */)
    {
        std::lock_guard<std::mutex> lock(m_msgMutex);
        float time = m_pContext->GetElapsedTime();
        m_messages.push_back({ _level, time, _msg,  });
    }

    void Logger::AddMessage(const LogMessage& _msg)
    {
        std::lock_guard<std::mutex> lock(m_msgMutex);
        m_messages.push_back(_msg);

    }

    void Logger::Flush()
    {
        std::lock_guard<std::mutex> lock(m_msgMutex);
        for (const auto& msg : m_messages) {
            std::lock_guard<std::mutex> lock(m_recMutex);
            for (const auto& [_name, rec] : m_recipients) {
                rec->Receive(msg);
            }
        }
        m_messages.clear();
        m_lastFlushTime = 0.0f;
    }


    void Logger::Update(float _dt)
    {
        m_totalTime += _dt;
        m_lastFlushTime += _dt;
        if( m_lastFlushTime >= LOG_FLUSH_TIME)
            Flush();
    }

}

