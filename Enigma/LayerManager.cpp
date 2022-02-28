#include "Context.h"
#include "HWRenderer.h"
#include "ResourceManager.h"
#include "EventHandler.h"
#include "HWFrameBuffer.h"
#include "HWTexture.h"
#include "SceneView.h"
#include "Window.h"
#include "Tools.h"
#include "HWFormats.h"
#include "HWVertexBuffer.h"
#include "SceneManager.h"
#include "Controller.h"
#include "Logger.h"
#include "UILayer.h"
#include "LayerManager.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
    //LayerManager Implementation
   //////////////////////////////////////////////////////////////////////////
    LayerManager::LayerManager(Context* _pContext) : SystemBase(_pContext)
    {
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());     
    }

    bool LayerManager::PostConstructor()
    {
        //Create default buffers, TODO use reflection of glsl shaders for this
        auto perLayer = std::make_unique<VertexBuffer<PerLayer>>(
            nullptr,
            1,
            eVertexBufferTarget::VERTEX_BUFFER_UNIFORM,
            eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_DRAW
            );
        GetContext().GetResourceManager().AddBuffer("PerLayer", std::move(perLayer));

        SubcriberFunc SceneWindowFocusGained = [=](EventBase& _evt) {
            auto activeView = GetString(_evt.m_data["sceneView"]);
            auto windowId = GetInt(_evt.m_data["windowId"]);
            auto newView = GetContext().GetResourceManager().GetSceneView(activeView);
            assert(newView);
            //AddLogMessage(m_pContext, fmt::format("Scene Window Focus Gained {}", activeView), eLogLevel::LOG_LEVEL_DEBUG);
            GetContext().GetSceneManager().SetActiveSceneView(newView);
            ConstructReceivers( true );
        };

        SubcriberFunc SceneWindowFocusLost = [=](EventBase& _evt) {
            auto activeView = GetString(_evt.m_data["sceneView"]);
            auto windowId = GetInt(_evt.m_data["windowId"]);
            auto curActiveView = GetContext().GetSceneManager().GetActiveSceneView();
            //keep current active view, just dont handle any input
            if (curActiveView) {
                 ConstructReceivers(false);           
            }
        };

        SubcriberFunc ToolActivated = [=](EventBase& _evt) {           
            auto curActiveView = GetContext().GetSceneManager().GetActiveSceneView();
            if (curActiveView) {
                ConstructReceivers(true);
            }
        };

        SubcriberFunc ToolDeActivated = [=](EventBase& _evt) {
            auto curActiveView = GetContext().GetSceneManager().GetActiveSceneView();
            if (curActiveView) {
                ConstructReceivers(true);
            }
        };


        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_SCENEWINDOW_FOCUS_GAINED, { this,  SceneWindowFocusGained });
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_SCENEWINDOW_FOCUS_LOST, { this,  SceneWindowFocusLost });
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_TOOL_ACTIVATED, { this,  ToolActivated });
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_TOOL_DEACTIVATED, { this,   ToolDeActivated });



        return true;
    }

    bool LayerManager::ConstructReceivers( bool _activeView )
    { 
        //receiver will be propagated from back to front, so
        //least significant receiver should be placed first in the container
        
        auto& rInputHandler = GetContext().GetInputHandler();
        auto  pActiveView    = GetContext().GetSceneManager().GetActiveSceneView();
        rInputHandler.ClearReceivers();
        //add scene view first
        if (pActiveView && pActiveView->m_pController && _activeView ) {
            rInputHandler.AddReceiver(pActiveView->m_pController.get());
        }
        //add layers beginning from world
        const auto layerFlags = eLayer::LAYER_ALL & (~eLayer::LAYER_UI);
        for (auto idx : g_layers)
        {
            if (!(layerFlags & (uint32_t)idx))
                continue;

            auto pLayer = GetLayer((eLayer)idx);
            if (!pLayer || !pLayer->IsActive())
                continue;
            rInputHandler.AddReceiver(pLayer);
        }
        //add active tool last
        if (auto pTool = GetContext().GetToolManager().GetActiveTool()) {
            rInputHandler.AddReceiver(pTool);
        }

        return true;

    }

    bool LayerManager::Clear()
    {
        //clear all except ui layer
        auto uiLayer = std::move(m_layerMap[eLayer::LAYER_UI]);
        m_layerMap.clear();
        m_layerMap[eLayer::LAYER_UI] = std::move(uiLayer);
        return true;
    }

  

    void LayerManager::Update(float _dt)
    {
        UNUSED(_dt)
    }

    void LayerManager::Draw(uint32_t _layerFlags, const HWViewport& _view, const HWCamera* _pCamera)
    {
        using PerLayerBuffer = VertexBuffer<PerLayer>;
        auto layer = static_cast<PerLayerBuffer*>(m_pContext->GetResourceManager().GetBuffer("PerLayer"));

        PerLayer prevLayerData;
        for (auto idx : g_layers)
        {
            if( !(_layerFlags & (uint32_t)idx))
                continue;
            
            auto pLayer = GetLayer((eLayer)idx);
            if (!pLayer || !pLayer->IsActive())
                continue;

            //update ubo with per layer info
           
            if (_pCamera) {
                auto layerData = pLayer->GetPerLayerData(_pCamera);
                if (layerData != prevLayerData) {
                    layer->Bind();
                    layer->BindBufferBase(1);
                    layer->UpdateTyped(&layerData, 1);
                    layer->UnBind();
                    prevLayerData = layerData;
                }
            }
            //draw layer          
            pLayer->SetWindowId(_view.m_windowId);
            pLayer->PreDraw(_view, _pCamera);          
            pLayer->Draw();
            pLayer->PostDraw();
        }
    }
 

    void LayerManager::ActivateLayers(uint32_t _layerFlags, bool _activate)
    {
        for (auto& [id, curLayer] : m_layerMap) {
            if (id & _layerFlags)
                curLayer->SetActive(_activate);
        }

    }

   

    LayerBase* LayerManager::GetLayer(eLayer _layer) const
    {
        const auto it = m_layerMap.find(_layer);
        if (it == std::end(m_layerMap))
            return nullptr;
        return m_layerMap.at(_layer).get();
    }

    bool LayerManager::AddLayer(eLayer _layer, std::unique_ptr<LayerBase> _pLayer)
    {
        if (GetLayer(_layer))
            return false;
        m_layerMap[_layer] = std::move(_pLayer);
        m_layerMap[_layer]->Attach();
        return true;
    }
    
    
   
    std::vector<SceneView*> LayerManager::GetActiveViews() const
    {
        auto pred = [](const Window* _win) { return _win->IsVisible() && _win->HasSceneView(); };

        std::vector<SceneView*> ret;
        const auto* uiLayer = static_cast<UILayer*>( GetLayer(eLayer::LAYER_UI));
        if (!uiLayer)
            return ret;
    
        auto windows = uiLayer->GetWindowManager().GetWindows(pred);
        for (auto win : windows) {
            const auto& winInfo = win->GetInfo();

            if (auto sceneView = m_pContext->GetResourceManager().GetSceneView(winInfo.m_sceneViewName)) {
                const auto newSize = Max(winInfo.m_frameBufferSize, glm::ivec2(MIN_FBO_DIMS, MIN_FBO_DIMS));
                sceneView->SetSize(newSize);
                sceneView->SetWindowId(winInfo.m_id);
                ret.push_back(sceneView);
            }
        }
        return ret;
    }

    std::vector<LayerBase*> LayerManager::GetLayers(eLayer _flags) const
    {
        std::vector<LayerBase*> layers;
        for (auto& [id, curLayer] : m_layerMap) {
            if(( curLayer->GetLayerType() & _flags) != 0 )
                layers.push_back(curLayer.get());
        }
        return layers;
    }

   

   

}

