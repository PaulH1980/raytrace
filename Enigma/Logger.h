#pragma once

#include "SystemBase.h"


namespace RayTrace
{

    inline std::string LogLevelToString(eLogLevel _lvl)
    {
        switch (_lvl) {
        case eLogLevel::LOG_LEVEL_INFO:    return "Info";
        case eLogLevel::LOG_LEVEL_WARNING: return "Warning";
        case eLogLevel::LOG_LEVEL_ERROR:   return "Error";
        case eLogLevel::LOG_LEVEL_FATAL:   return "Fatal";
        case eLogLevel::LOG_LEVEL_DEBUG:   return "Debug";
        default: {}
        };
        return "Unknown";
    }


    
    struct LogMessage
    {
        eLogLevel   m_level   = eLogLevel::LOG_LEVEL_INFO;
        float       m_time = 0.0f;
        std::string m_msg;
    };
    
    class LogRecipient
    {
    public:
        ~LogRecipient() {}

        virtual void Receive(const LogMessage& _msg) = 0;     
    };
    
    
    static const float LOG_FLUSH_TIME = 0.25f; //seconds

    class Logger : public SystemBase
    {
        RT_OBJECT(Logger, SystemBase)

    public:

        Logger(Context* _pContext);

        bool Clear() override;

        bool AddRecipient(const std::string& _name,  const std::shared_ptr<LogRecipient>& _recipient);
        bool RemoveRecipient(const std::string& _name);
   
        void AddMessage(  const std::string& _msg, eLogLevel _level = eLogLevel::LOG_LEVEL_INFO );
        void AddMessage(const LogMessage& _msg);
        void Flush();

        void Update(float _dt) override;

    private:
        using RecipientMap = std::map<std::string, std::shared_ptr<LogRecipient>>;

        RecipientMap             m_recipients;
        std::vector<LogMessage>  m_messages; //messages added
        std::mutex               m_msgMutex, 
                                 m_recMutex;

        float                    m_lastFlushTime = 0;
        float                    m_totalTime     = 0;
    };


   
}