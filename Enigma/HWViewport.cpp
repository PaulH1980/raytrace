#include "HWFrameBuffer.h"
#include "HWTexture.h"
#include "HWViewport.h"

namespace RayTrace {

    void HWViewport::Resize(const Vector2i& _size)
    {
        m_size = _size;
        if (m_frameBuffer)
            m_frameBuffer->Resize(_size.x, _size.y);
    }

    const Vector2i& HWViewport::GetFrameBufferSize() const
    {
        return m_size;
    }

    const Vector2i& HWViewport::GetFrameBufferPos() const
    {
        return m_pos;
    }

    WindowId HWViewport::GetWindowId() const
    {
        return m_windowId;
    }

    const Vector2i& HWViewport::GetWindowPos() const
    {
        return m_windowPos;
    }

    const Vector2i& HWViewport::GetSize() const
    {
        return m_size;
    }

    HWFrameBuffer* HWViewport::GetFrameBuffer() const
    {
        return m_frameBuffer;
    }

    void HWViewport::SetFrameBuffer(HWFrameBuffer* _pFrameBuffer)
    {
        m_frameBuffer = _pFrameBuffer;      
    }

}
