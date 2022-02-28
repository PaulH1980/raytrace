


#include "Window.h"
#include "Context.h"
#include "HWRenderer.h"
#include "Window.h"
#include "ResourceManager.h"
#include "HWVertexBuffer.h"
#include "HWFrameBuffer.h"
#include "HWTexture.h"
#include "UILayer.h"






namespace RayTrace
{



    //////////////////////////////////////////////////////////////////////////
   //UILayer implementation
   //////////////////////////////////////////////////////////////////////////
    UILayer::UILayer(Context* _pContext, void* _window, void* _glContext) 
        : LayerBase(_pContext)
        , m_manager( std::make_unique<WindowManager>(_pContext, _window, _glContext ))
    {
        m_layer = eLayer::LAYER_UI;


        {
            WindowInfo info;
            info.m_manager = m_manager.get();
            info.m_name = info.m_type = "MainMenubar";
            info.m_fields.m_visible    = true;
            info.m_fields.m_toggleable = 0;
            m_manager->AddWindow( info.m_name, std::unique_ptr<Window>(CreateWindow(info)));
            info.m_name = info.m_type = "Toolbar";        
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));
            info.m_name = info.m_type = "Statusbar"; 
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));

           
            info.m_name = info.m_type = "LogWindow"; //down
            info.m_drawOrderId = 99;
            info.m_fields.m_toggleable = 1;
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));

            info.m_name = info.m_type = "FlowWindow"; //down
            info.m_drawOrderId = 98;
            info.m_fields.m_toggleable = 1;
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));

            info.m_name = info.m_type = "ResourceExplorer"; //down
            info.m_drawOrderId = 97;
            info.m_fields.m_toggleable = 1;
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));
         
            info.m_name = info.m_type = "ExplorerGrid"; //left
            info.m_drawOrderId = 96;
            info.m_fields.m_toggleable = 1;
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));

            info.m_name = info.m_type = "PropertyGrid"; //right
            info.m_drawOrderId = 95;
            info.m_fields.m_toggleable = 1;
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));

            //toggleable visible items             
            info.m_name = info.m_type    = "SceneWindow"; //up
            info.m_drawOrderId           = 100;
            info.m_fields.m_toggleable   = 1;
            info.m_fields.m_hasSceneView = 1;
            info.m_sceneViewName         = "__DefaultSceneView"; //default named
           
            m_manager->AddWindow(info.m_name, std::unique_ptr<Window>(CreateWindow(info)));
        }        
    }

    UILayer::~UILayer()
    {
        
    }

    void UILayer::PreDraw(const HWViewport& _viewport, const HWCamera* _pCamera)
    {
        
        m_manager->PreDraw();
    }

    void UILayer::Draw()
    {
        m_manager->Draw();    
    }

    void UILayer::PostDraw()
    {
        m_manager->PostDraw();
    }       


    WindowManager& UILayer::GetWindowManager() const
    {
        return *m_manager;
    }
    


}


