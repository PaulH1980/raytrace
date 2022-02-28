#pragma once
#include "FrontEndDef.h"

namespace RayTrace
{
    enum eClearMask
    {
        CLEAR_COLOR = 0x01,
        CLEAR_DEPTH = 0x02,
        CLEAR_STENCIL = 0x04,
        CLEAR_ALL = CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL
    };
    struct FrameBufferClear
    {
        Vector4f m_rgba = { 0.f, 0.f, 1.f, 1.f };
        float    m_depth = 1.0f;
        int      m_stencil = 0;
        uint32_t m_mask = CLEAR_ALL;
    };
}