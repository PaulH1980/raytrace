#include "Controller.h"

#include "LayerManager.h"
#include "Context.h"
#include "ResourceManager.h"
#include "HWFrameBuffer.h"
#include "HWTexture.h"
#include "HWRenderer.h"
#include "SceneView.h"

namespace RayTrace
{

    SceneView::SceneView(Context* _pContext, HWFrameBuffer* _pFrameBuffer )
        : ObjectBase( _pContext )
    {
        assert(_pFrameBuffer);
        
        m_pCamera.reset(new HWCamera(2,2));
        m_pController.reset(new FPSCameraController( _pContext, m_pCamera.get() ) );                
        m_viewport.m_frameBuffer = _pFrameBuffer;
        m_viewport.m_size = m_viewport.m_frameBuffer->GetSize();
    }  

    SceneView::SceneView(Context* _pContext, HWFrameBuffer* _pFrameBuffer, std::unique_ptr<CameraControllerBase> _pController)
        : ObjectBase(_pContext)
        , m_pController( std::move( _pController ) )
    {
        assert(_pFrameBuffer);
        m_pCamera.reset(new HWCamera(2, 2));
        m_pController->SetCamera(m_pCamera.get());
        m_viewport.m_frameBuffer = _pFrameBuffer;
        m_viewport.m_size = m_viewport.m_frameBuffer->GetSize();
    }

    void SceneView::SetSize(const Vector2i& _size)
    {
        m_viewport.Resize(_size);
        if (m_pController && _size.x > 0 && _size.y >= 0)
            m_pController->Resize( _size );
    }

    void SceneView::SetWindowId( int _windowId )
    {
        m_viewport.m_windowId = _windowId;
        if (m_pController)
            m_pController->SetViewportId(_windowId);
    }

    void SceneView::SetWindowPos(const Vector2i& _pos)
    {
         m_viewport.m_windowPos = _pos;
    }

    const Vector2i& SceneView::GetWindowPos() const
    {
        return m_viewport.m_windowPos;
    }

    HWFrameBuffer* SceneView::GetFrameBuffer() const
    {
        return m_viewport.m_frameBuffer;
    }

}

