#pragma once
#include <queue>
#include "FrontEndDef.h"

#include "AppInit.h"
#include "NonAssignable.h"


namespace RayTrace
{
    
    class Context : public NonAssignable
    {
    public:
        Context( const AppInit& _init );
        virtual ~Context();

        virtual bool        PostConstructor(const Vector2i& _dims);
        virtual bool        ExitRequested() const;
              
        
        //Input, Update Stuff
        void                Update(float _dt);
        void                Resize(const Vector2i& _dims);
        Vector2i            GetSize() const;
        void                RequestExit();

      
        void                RequestScene(const std::string& _scenePath);
        const std::string&  GetCurrentScene() const;
             
        //Event Handling Stuff
        void                AddEvent(const EventBase& _evt);
        void                PostEvents();

        //Add new object to scene
        virtual bool        AddObject(WrappedEntity* _pObject, eLayer _layer = eLayer::LAYER_ALL );                

        //Misc Stuff
        uint32_t            GetFrameNumber() const;
        float               GetElapsedTime() const;
        float               GetFrameTime() const;
        uint32_t            GetNextObjectId();
        uint32_t            GetNextEntityId();

        ObjectUnderMouse    ObjectInfoFromScreen(const Vector2i& _xy, uint32_t _windowId, bool& _valid) const;

        //request a new scene
        virtual void        BeginFrame();

        //Render Stuff
        virtual void        PreRender();
        virtual void        Render();
        virtual void        PostRender();

        virtual void        EndFrame();


        //ResourceManager
        ResourceManager&    GetResourceManager() const;
        EventHandler&       GetEventHandler() const;
        HWRenderer&         GetRenderer() const;
        LayerManager&       GetLayerManager() const;
        Logger&             GetLogger() const;
        ComponentSystem&    GetComponentSystem() const;
        SceneManager&       GetSceneManager() const;
        HistoryStack&       GetHistory() const;
        InputHandler&       GetInputHandler() const;
        ToolManager&        GetToolManager() const;
        CopyPasteBuffer&    GetCopyPasteBuffer() const;

        SceneView*          GetActiveSceneView() const;

        template<typename T> 
        SystemBase&         GetSystem( const std::string& _name ) const;

        template<typename T> 
        T&                  GetSystem();

        

    private:
        using SystemMap = std::map<std::string, std::unique_ptr<SystemBase>>;

        virtual bool        CreateDefaultSystems();
        virtual bool        CreateDefaultObjects();
      
  
        virtual bool        CreateRenderer();
        virtual bool        CreateRendererFrameBuffers();
        virtual bool        CreateRendererShaders();
        virtual bool        CreateRendererMaterials();
        virtual bool        CreateRendererPasses();
      
        virtual bool        BeginNewScene();
        virtual bool        ClearScene();
       
        HWRenderer*             m_renderBackend = nullptr;
        ResourceManager*        m_resMan        = nullptr;  
        EventHandler*           m_evtHandler    = nullptr;
        LayerManager*           m_layerManager  = nullptr;
        Logger*                 m_logger        = nullptr;
        ComponentSystem*        m_componentSystem = nullptr;
        SceneManager*           m_sceneManager  = nullptr;
        HistoryStack*           m_history       = nullptr;
        InputHandler*           m_inputHandler  = nullptr;
        ToolManager*            m_pToolManager  = nullptr;
        std::unique_ptr<CopyPasteBuffer> m_copyPasteBuffer;


        SystemMap               m_systemMap;
     
        std::atomic<uint32_t>   m_nextEntitytId = 1;
        std::atomic<uint32_t>   m_nextObjectId  = 1;

        void* m_windowHandle    = nullptr;
        void* m_hwContextHandle = nullptr;
      

        std::string m_currentScene;

        float    m_totalTime      = 0.0f;
        float    m_deltaTime      = 0.0f;
        int      m_width          = 0, 
                 m_height         = 0;
        uint32_t m_frameNumber    = 0;
        bool     m_clearRequested = false;
        bool     m_firstTime      = true;
        bool     m_exitRequested  = false;

        

    };




    template<typename T>
    SystemBase& Context::GetSystem(const std::string& _name) const
    {
        static_assert(std::is_base_of<SystemBase, T>::value);
        const auto it = m_systemMap.find(_name);
        if (it == std::end(m_systemMap))
            throw std::exception("System Not Found");
        auto system = dynamic_cast<T*>(m_systemMap.at(_name).get());
        if (!system)
            throw std::exception("Invalid Pointer Cast");
        return *system;
    }
    
    template<typename T>
    T& Context::GetSystem()
    {
        const auto name = T::GetTypeNameStatic();
        return static_cast<T&>(GetSystem<T>(name));
    }

}