#pragma once

#include "LayerManager.h"
#include "UIAction.h"

namespace RayTrace
{
    /////////////////////////////////////////////////////////////////////////
   //UILayer Declarations
   //////////////////////////////////////////////////////////////////////////
    class UILayer : public LayerBase
    {
        RT_OBJECT(UILayer, LayerBase)

    public:
        
        UILayer(Context* _pContext, void* _window, void* _glContext );
        ~UILayer();
        void     PreDraw(const HWViewport& _viewport, const HWCamera* _pCamera)override;
        void     Draw() override;
        void     PostDraw()override;      

        WindowManager& GetWindowManager() const;

	
        

    private:
     
      
        std::unique_ptr<WindowManager>  m_manager;
      
    };
}
