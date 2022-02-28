#pragma once
#include "HWFrameBufferClear.h"
namespace RayTrace
{

 

    class HWViewport
    {
    public:

        void                            Resize(const Vector2i& _size);
        const Vector2i&                 GetFrameBufferSize() const;
        const Vector2i&                 GetFrameBufferPos() const;       
        const Vector2i&                 GetWindowPos() const;
        const Vector2i&                 GetSize() const;
        WindowId                        GetWindowId() const;

        HWFrameBuffer*                  GetFrameBuffer() const;
        void                            SetFrameBuffer(HWFrameBuffer* _pFrameBuffer);

        Vector2i                        m_windowPos;
        Vector2i                        m_pos,
                                        m_size;
        FrameBufferClear                m_clearData;
        HWFrameBuffer*                  m_frameBuffer = nullptr;
        WindowId                        m_windowId = INVALID_ID;
    };

}