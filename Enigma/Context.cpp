#include "ModelLoader.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "ContextInitDefault.h"


namespace RayTrace
{
    Context::Context( const AppInit& _init )       
        : m_windowHandle( _init._window )
        , m_hwContextHandle( _init._hwContext )
    {   

    }


    Context::~Context()
    {

    }

    bool Context::PostConstructor(const Vector2i& _dims)
    {
        bool valid = true;
        if(m_firstTime)
            valid &= CreateDefaultSystems();
       
        if (m_firstTime)
            valid &= CreateRenderer();

        valid &= CreateDefaultObjects();
        valid &= CreateRendererFrameBuffers();
        valid &= CreateRendererShaders();

        for (auto& [key, system] : m_systemMap)
            valid &= system->PostConstructor();
        
        valid &= CreateRendererMaterials();       
        valid &= CreateRendererPasses();

        GetSceneManager().SetActiveSceneView(GetResourceManager().GetSceneView("__DefaultSceneView"));
        GetToolManager().ActivateTool("Selection", "Point");

        assert(valid);
        if( m_firstTime )
            Resize(_dims);
        m_firstTime = false;

     

        return valid;
    }



	bool Context::ExitRequested() const
	{
		return m_exitRequested;
	}

	bool Context::BeginNewScene()
    {
        return true;
    }

    bool Context::ClearScene()
    {
        for (auto& [key, system] : m_systemMap)
            system->Clear();


        m_nextEntitytId = 1;
        m_nextObjectId = 1;
        m_totalTime = 0.0f;
        m_deltaTime = 0.0f;
        m_frameNumber = 0;
        m_clearRequested = false;
        return true;
    }

    void Context::Update(float _dt)
    {
        for (auto& [key, system] : m_systemMap)
            system->Update(_dt);
        m_deltaTime  = _dt;
        m_totalTime += _dt;

        EventBase evt(eEvents::EVENT_UPDATE, GetElapsedTime());
        evt.m_data["deltaTime"] = m_deltaTime;
        evt.m_data["totalTime"] = m_totalTime;
        AddEvent(evt);
    }

    void Context::Resize(const Vector2i& _dims) {

        int _width = _dims[0];
        int _height = _dims[1];
        if (_width == m_width && _height == m_height)
            return;     

        EventBase resizeEvt(eEvents::EVENT_RESIZE, GetElapsedTime());
        resizeEvt.m_data["width"]  = _width;
        resizeEvt.m_data["height"] = _height;      

        m_width = _width;
        m_height = _height;      

        AddEvent(resizeEvt);
    }


    Vector2i Context::GetSize() const
    {
        return Vector2i(m_width, m_height);
    }

	void Context::RequestExit()
	{
        m_exitRequested = true;
	}

	void Context::BeginFrame()
    {
        if (m_exitRequested)
        {
            //cleanup
        }
        
        
        if (m_clearRequested) {
            ClearScene();
            PostConstructor({ m_width, m_height });
            if ( !m_currentScene.empty() )
            {
                try {
                  // LoadModel( this, m_currentScene, )
                } catch (std::exception e) {                   
                    AddLogMessage(this, fmt::format("Failed To Load Scene:{}", m_currentScene), eLogLevel::LOG_LEVEL_ERROR);
                    (e);
                }
            }
        }
    }

    void Context::RequestScene(const std::string& _scenePath)
    {
        if (_scenePath != m_currentScene || _scenePath.empty() )
        {
            m_clearRequested = true;
            m_currentScene = _scenePath;
        }
    }


    const std::string& Context::GetCurrentScene() const
    {
        return m_currentScene;
    }

    void Context::AddEvent(const EventBase& _evt)
    {
        GetEventHandler().AddEvent(_evt);
    }

    void Context::PostEvents()
    {
        GetEventHandler().PostEvents();
    }
   
    bool Context::AddObject(WrappedEntity* _pObject, eLayer _layer )
    {
       _pObject->SetLayerFlags(_layer);      
       return GetSceneManager().AddObject( _pObject->GetUUID(), std::unique_ptr<WrappedEntity>( _pObject ) );
    }
   

    ResourceManager& Context::GetResourceManager() const
    {
        return *m_resMan;
    }

    EventHandler& Context::GetEventHandler() const
    {
        return *m_evtHandler;
    }

    HWRenderer& Context::GetRenderer() const
    {
        return *m_renderBackend;
    }

    LayerManager& Context::GetLayerManager() const
    {
        return *m_layerManager;
    }

    Logger& Context::GetLogger() const
    {
        return *m_logger;
    }

    ComponentSystem& Context::GetComponentSystem() const
    {
        return *m_componentSystem;
    }
    
    SceneManager& Context::GetSceneManager() const
    {
        return *m_sceneManager;
    }   

    HistoryStack& Context::GetHistory() const
    {
        return *m_history;
    }

    InputHandler& Context::GetInputHandler() const
    {
        return *m_inputHandler;
    }

    ToolManager& Context::GetToolManager() const
    {
        return *m_pToolManager;
    }

    CopyPasteBuffer& Context::GetCopyPasteBuffer() const
    {
        return *m_copyPasteBuffer.get();
    }

    SceneView* Context::GetActiveSceneView() const
    {
        return GetSceneManager().GetActiveSceneView();
    }

    uint32_t Context::GetFrameNumber() const
    {
        return m_frameNumber;
    }

    float Context::GetElapsedTime() const
    {
        return m_totalTime;
    }

    float Context::GetFrameTime() const
    {
        return m_deltaTime;
    }

    uint32_t Context::GetNextObjectId() 
    {
        return m_nextObjectId++;      
    }

    uint32_t Context::GetNextEntityId()
    {
        return m_nextEntitytId++;
    }



    ObjectUnderMouse Context::ObjectInfoFromScreen(const Vector2i& _xy, uint32_t _windowId, bool& _valid) const
    {
        UNUSED(_windowId)
        
        const auto pLayer =dynamic_cast<EditorLayer*>(  GetLayerManager().GetLayer(eLayer::LAYER_EDITOR));
        assert(pLayer);       
        const auto pos = pLayer->GetWorldPosition(_xy, pLayer->GetDepth(_xy));
        if (HasInfinities(pos)) {
            _valid = false;
            return InvalidObjUnderMouse;
        }
        const auto objId = pLayer->GetObjectId(_xy);
        _valid = true;
        return ObjectUnderMouse(objId, pos);

    }

    void Context::PreRender()
    {
        static const Vector2i MinFboSize(MIN_FBO_DIMS, MIN_FBO_DIMS);

        for (auto view : m_layerManager->GetActiveViews())
        {
            const auto size = view->m_viewport.m_size;
            view->m_viewport.m_size = Max(size, MinFboSize);
            view->m_viewport.m_frameBuffer->Resize(view->m_viewport.m_size.x, view->m_viewport.m_size.y);
        }
        
        GetRenderer().BeginRender();       
    }

    void Context::Render()
    {
        m_frameNumber++;   
    
        auto activeViews = m_layerManager->GetActiveViews();
        for (auto& curView : activeViews)
        {
            m_renderBackend->PushViewport(curView->m_viewport);    
            m_layerManager->Draw(eLayer::LAYER_ALL & (~LAYER_UI), curView->m_viewport, curView->m_pCamera.get() ); //draw all except layer ui
            m_renderBackend->PopViewport();
        }

        //Create fullscreen viewport and render layer ui
        HWViewport uiViewport;
        uiViewport.m_size = { m_width, m_height };
        uiViewport.m_clearData.m_mask = 0x0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_renderBackend->PushViewport(uiViewport);        
        m_layerManager->Draw(LAYER_UI, uiViewport, nullptr);
        m_renderBackend->PopViewport();   
    }

    void Context::PostRender()
    {
        GetRenderer().EndRender();
       
    }

    void Context::EndFrame()
    {

    }
   

}

