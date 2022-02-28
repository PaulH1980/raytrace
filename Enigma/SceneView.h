#pragma once

#include "FrontEndDef.h"
#include "Transform.h"
#include "HWViewport.h"
#include "ObjectBase.h"

namespace RayTrace
{
    
    
    class SceneView : public ObjectBase
    {
        RT_OBJECT(SceneView, ObjectBase)
    public:
        
        SceneView(Context* _pContext, HWFrameBuffer* _pFrameBuffer, std::unique_ptr<CameraControllerBase> _pController );
        
        /*
            @brief: Create a sceneview
        */
        SceneView( Context* _pContext, HWFrameBuffer *_pFrameBuffer );
  
        void                SetSize(const Vector2i& _size);
        void                SetWindowId( int _windowID);
        void                SetWindowPos(const Vector2i& _pos);
        const Vector2i&     GetWindowPos() const;

        HWFrameBuffer*      GetFrameBuffer() const;

        std::unique_ptr<CameraControllerBase> m_pController;
        std::unique_ptr<HWCamera>             m_pCamera;
        HWViewport          m_viewport;
    
    };
}