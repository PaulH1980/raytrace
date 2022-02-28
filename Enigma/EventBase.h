#pragma once
#include "FrontEndDef.h"

namespace RayTrace
{
    enum class eEvents : int
    {
        EVENT_UNDEFINED = -1,

        EVENT_MOUSE_UP = 10,
        EVENT_MOUSE_DOWN,
        EVENT_MOUSE_PRESSED,
        EVENT_MOUSE_DOUBLE_CLICKED,
        EVENT_MOUSE_MOTION,
        EVENT_MOUSE_WHEEL,

        EVENT_KEY_UP,
        EVENT_KEY_DOWN,
        EVENT_KEY_PRESSED,
        EVENT_UPDATE,

        EVENT_VIEWPORT_RESIZE,
        EVENT_RESIZE,
        EVENT_FOCUS_ON_SCENEOBJECT_REQUEST, //focus camera on object
        EVENT_SCENEWINDOW_FOCUS_GAINED,
        EVENT_SCENEWINDOW_FOCUS_LOST,
        EVENT_SCENEWINDOW_MOUSE_ENTERED,
        EVENT_SCENEWINDOW_MOUSE_LEAVE,
        EVENT_TOOL_ACTIVATED,
        EVENT_TOOL_DEACTIVATED,
        EVENT_OBJECT_SELECTION,
        EVENT_OBJECT_TRANSFORM_CHANGED, //transform changed by user(UI) input
        EVENT_OBJECT_MEMBER_CHANGED,
        EVENT_OBJECTS_REMOVED,
        EVENT_OBJECTS_ADDED,

        EVENT_BEGIN_OBJECT_MODIFICATION,
        EVENT_END_OBJECT_MODIFICATION,
    };

    using EventPayload = std::variant<bool, float, int, void*,  StringVector,
        Vector2i, Vector3i, Vector4i,
        Vector2f, Vector3f, Vector4f,
        Matrix4x4, std::string>;
    using EventPayloadMap = std::map<std::string, EventPayload>;

    inline bool GetBool(const EventPayload& _v) {
        return std::get<bool>(_v);
    }

    inline  int GetInt(const EventPayload& _v) {
        return std::get<int>(_v);
    }

    inline float GetFloat(const EventPayload& _v) {
        return std::get<float>(_v);
    }

    inline Vector2i GetVector2i(const EventPayload& _v) {
        return std::get<Vector2i>(_v);
    }

    inline Vector3i GetVector3i(const EventPayload& _v) {
        return std::get<Vector3i>(_v);
    }

    inline Vector4i GetVector4i(const EventPayload& _v) {
        return std::get<Vector4i>(_v);
    }

    inline Vector2f GetVector2f(const EventPayload& _v) {
        return std::get<Vector2f>(_v);
    }

    inline Vector3f GetVector3f(const EventPayload& _v) {
        return std::get<Vector3f>(_v);
    }

    inline Vector4f GetVector4f(const EventPayload& _v) {
        return std::get<Vector4f>(_v);
    }

    inline Matrix4x4 GetMatrix4x4(const EventPayload& _v) {
        return std::get<Matrix4x4>(_v);
    }

    inline void* GetPtr(const EventPayload& _v) {
        return std::get<void*>(_v);
    }

    inline std::string GetString(const EventPayload& _v) {
        return std::get<std::string>(_v);
    }

    inline StringVector GetStringVector(const EventPayload& _v) {
        return std::get<StringVector>(_v);
    }

    struct EventBase
    {
        EventBase();
        EventBase(eEvents _type, float _time);

        eEvents         m_eventType = eEvents::EVENT_UNDEFINED;
        uint32_t        m_eventId = 0;
        float           m_eventTime = 0.0f;
        uint32_t        m_pad = 0xFFFFFFFF;
        bool            m_handled = false;
        EventPayloadMap m_data;

        static std::atomic<uint32_t> s_nextId;
    };
}