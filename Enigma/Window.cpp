#define GLEW_STATIC
#include  <glew.h>
#include <backends\imgui_impl_sdl.h>
#include <backends\imgui_impl_opengl3.h>
#include "ImGuizmo.h"
#include <SDL.h>
#include <SDL_opengl.h>

#include <stdio.h>
#include "UiIncludes.h"
#include "UIAction.h"
#include "IO.h"

#include "ShaderSources.h"
#include "FlowWindow.h"
#include "PropertyWindow.h"
#include "HierarchyWindow.h"
#include "LogWindow.h"
#include "SceneWindow.h"
#include "LogWindow.h"
#include "Statusbar.h"
#include "Toolbar.h"
#include "ResourceExplorer.h"
#include "MainMenubar.h"
#include "FilePaths.h"
#include "Window.h"


namespace RayTrace
{


    bool Focus(WrappedEntity* _pObject)
    {
        auto& context = _pObject->GetContext();
        const auto& bb = _pObject->GetBoundingBox();
        if (bb.volume() > 0.0f) {

            EventBase evt(eEvents::EVENT_FOCUS_ON_SCENEOBJECT_REQUEST, context.GetElapsedTime());
            evt.m_data["min"] = bb.m_min;
            evt.m_data["max"] = bb.m_max;
            evt.m_data["uuid"] = _pObject->GetUUID();
            context.AddEvent(evt);


            //CenterOnBounds(camera, bb.m_min, bb.m_max);
            return true;
        }    
        return false;
    }

    class WindowCursor
    {
    public:

        WindowCursor() {

        }

        ~WindowCursor()
        {
            if (m_pCursor)
                SDL_FreeCursor(m_pCursor);
        }


        bool LoadCursor(const std::string& _fileName, int* _hotspotX = nullptr, int* _hotspotY = nullptr )
        {
            ImageExInfo result;
            result.m_readFlipped = true;
            if (!ReadImage(_fileName, result))
                return false;


            if (result.m_numChannels != 4)
                return false;

            U8Vector r, mask;
           
            SplitChannels(result, &r, nullptr, nullptr, &mask );
            int cX = _hotspotX != nullptr ? *_hotspotX : result.m_width / 2;
            int cY = _hotspotY != nullptr ? *_hotspotY : result.m_height / 2;

            m_pCursor = SDL_CreateCursor(r.data(), mask.data(), result.m_width, result.m_height, cX, cY);

            return m_pCursor != nullptr;            
        }

       


        SDL_Cursor* m_pCursor = nullptr;;
    };



    Window* CreateWindow(const WindowInfo& _hint)
    {
        const auto& _type = _hint.m_type;
        
        //simple 'factory' method
        Window* _pWindow = nullptr;
        if (_type == "LogWindow")
            _pWindow = new LogWindow(_hint);
        else if (_type == "SceneWindow")
            _pWindow = new SceneWindow(_hint);
        else if (_type == "Statusbar")
            _pWindow = new Statusbar(_hint);
        else if (_type == "Toolbar")
            _pWindow = new Toolbar(_hint);
        else if (_type == "ExplorerGrid")
            _pWindow = new ExplorerGrid(_hint);
        else if (_type == "PropertyGrid")
            _pWindow = new PropertyGrid(_hint);
        else if (_type == "ResourceExplorer")
            _pWindow = new ResourceExplorer(_hint);
        else if (_type == "MainMenubar")
            _pWindow = new MainMenubar(_hint);
        else if (_type == "FlowWindow")
            _pWindow = new FlowWindow(_hint);

        assert(_pWindow && "Invalid Type");

       
        return _pWindow;
    }


    Window::Window(const WindowInfo& _hint)
        : m_info(_hint)
    {

    }


    void Window::Layout()
    {

    }

    void Window::PreDraw()
    {

    }

    void Window::Draw()
    {

    }

    void Window::PostDraw()
    {

    }

    void Window::HandleEvents()
    {
        
    }

    bool Window::HasSceneView() const
    {
        return m_info.m_fields.m_hasSceneView;
    }

    void Window::SetVisible(bool _visible)
    {
        m_info.m_fields.m_visible = _visible;
    }

    bool Window::IsVisible() const
    {
        return m_info.m_fields.m_visible != 0;
    }

    bool Window::Dock(Window* _pParent)
    {
       UNUSED(_pParent)
        
        return false;
    }

    bool Window::Undock()
    {
        return false;
    }

    SceneView* Window::GetSceneView() const
    {
        return GetContext().GetResourceManager().GetSceneView(m_info.m_sceneViewName);
    }

    Context& Window::GetContext() const
    {
        return m_info.m_manager->GetContext();
    }

    const WindowInfo& Window::GetInfo() const
    {
        return m_info;
    }

    WindowInfo& Window::GetInfo()
    {
        return m_info;
    }

   

    void Window::Resize(int _width, int _height)
    {
        UNUSED(_width)
        UNUSED(_height)
    }

    WindowManager::WindowManager(Context* _pContext, void* _windowHandle, void* _hwContextHandle )
        : SystemBase( _pContext )
        , m_imageCache( 256 )
    {
        SubcriberFunc OnResize = [=](EventBase& _evt) {   
            this->Resize(GetInt(_evt.m_data["width"]), GetInt(_evt.m_data["height"]));
        };

        GetContext().GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_RESIZE, { this, OnResize });

        //load cursors
        {
            const auto path = GetTextureDirectory() + "Editor/";
            bool valid = true;
            {
                valid &= LoadCursor("CrossHair", path + "CrossHair.png");
                valid &= LoadCursor("HandGrab", path + "HandGrab.png");
            }
        }


        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
       //  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;         // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
       // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
       // ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        ImGuiStyle& style = ImGui::GetStyle();
        {

            auto fontPtr = io.Fonts->AddFontFromFileTTF("assets/fonts/Karla-Regular.ttf", 20.0f);
            AddFont("Karla-Regular", fontPtr);



            ImVec4* colors = style.Colors;

            /// 0 = FLAT APPEARENCE
            /// 1 = MORE "3D" LOOK
            int is3D = 0;

            colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
            colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
            colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
            colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

            style.PopupRounding = 3;

            style.WindowPadding = ImVec2(4, 4);
            style.FramePadding = ImVec2(6, 4);
            style.ItemSpacing = ImVec2(6, 2);

            style.ScrollbarSize = 18;

            style.WindowBorderSize = 1;
            style.ChildBorderSize = 1;
            style.PopupBorderSize = 1;
            style.FrameBorderSize = is3D;

            style.WindowRounding = 3;
            style.ChildRounding = 3;
            style.FrameRounding = 3;
            style.ScrollbarRounding = 2;
            style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK 
            style.TabBorderSize = is3D;
            style.TabRounding = 3;

            colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }
#endif
        }

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        //ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)_windowHandle, _hwContextHandle);
        ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    }

  

    WindowManager::~WindowManager()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    bool WindowManager::Clear()
    {
        return true;
    }


    void WindowManager::PreDraw()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
      

        Layout();
        
        
        auto windows = GetWindows([](Window* _win) { return _win->IsVisible(); });
        SortWindows(windows);
        for (auto win : windows)
            win->PreDraw();
    }

    void WindowManager::Draw()
    {
        auto windows = GetWindows([](Window* _win) { return _win->IsVisible(); });
        SortWindows(windows);
        for (auto win : windows)
            win->Draw();      





        ImGui::ShowDemoWindow();
    }

    void WindowManager::PostDraw()
    {
        auto windows = GetWindows([](Window* _win) { return _win->IsVisible(); });
        SortWindows(windows);
        for (auto win : windows)
            win->PostDraw();


        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // Rendering
        glUseProgram(0);
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
    }

    void WindowManager::Resize(int _width, int _height)
    {
        m_applicationWindowSize = { _width, _height };
    }

    void WindowManager::Layout()
    {
        ImGuiDockNodeFlags dockspace_flags = 0;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        const auto offset = ImVec2(0, m_toolBarHeight);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos + offset);
        ImGui::SetNextWindowSize(viewport->WorkSize - ImVec2(0, m_toolBarHeight + m_statusBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoSavedSettings ;


        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);


        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            const auto dockSize = ImGui::GetWindowSize();
            const auto dockPos = ImGui::GetWindowPos();
     
            if (m_firstTime)
            {

                m_firstTime = false;
                ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, dockSize);
                ImGui::DockBuilderSetNodePos(dockspace_id, dockPos);

                float sidePanelSize   = dockSize.x * 0.25f;
                float centerPanelSize = dockSize.x * 0.50f;
                float bottomPanelSize = dockSize.y * 0.25f;
                float topPanelSize    = dockSize.y * 0.75f;

                UNUSED(topPanelSize)
                UNUSED(bottomPanelSize)
                UNUSED(centerPanelSize)



                // auto 
                auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
                auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
                auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
                auto dock_id_up = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 1, nullptr, &dockspace_id);


                ImGui::DockBuilderSetNodeSize(dock_id_left, ImVec2(sidePanelSize, viewport->Size.y));
                ImGui::DockBuilderSetNodeSize(dock_id_right, ImVec2(sidePanelSize, viewport->Size.y));

                ImGui::DockBuilderDockWindow("ExplorerGrid", dock_id_left);
                ImGui::DockBuilderDockWindow("PropertyGrid", dock_id_right);
                ImGui::DockBuilderDockWindow("LogWindow", dock_id_down);
                ImGui::DockBuilderDockWindow("ResourceExplorer", dock_id_down);
                ImGui::DockBuilderDockWindow("FlowWindow", dock_id_down);
                ImGui::DockBuilderDockWindow("SceneWindow", dock_id_up);

                ImGui::DockBuilderFinish(dockspace_id);
                ImGui::SetNextWindowDockID(dockspace_id);

            }
        }
        ImGui::End();
    }   

    void WindowManager::Update(float _dt)
    {
        UNUSED(_dt)
    }

    bool WindowManager::LoadCursor(const std::string& _cursorName, const std::string& _fileName, int* _hotspotX, int* _hotspotY)
    {
        //cross hair
        auto pCursor = std::make_unique<WindowCursor>();
        if (pCursor->LoadCursor(_fileName, _hotspotX, _hotspotY ) )
        {
            m_cursors[_cursorName] = std::move(pCursor);
            return true;
        }
        return false;
    }

    bool WindowManager::SetCursor(const std::string& _name)
    {
        if (m_cursors.find(_name) == std::end(m_cursors))
            return false;
        SDL_SetCursor(m_cursors[_name]->m_pCursor);
        return true;
    }

    bool WindowManager::AddWindow(const std::string& _name, std::unique_ptr<Window> _window)
    {
        if (GetWindow(_name))
            return false;
        m_windows[_name] = std::move(_window);
        return true;
    }

    Window* WindowManager::GetWindow(const std::string& _name) const
    {
        const auto it = m_windows.find(_name);
        if (it == std::end(m_windows))
            return nullptr;
        return m_windows.at(_name).get();
    }


    bool WindowManager::AddFont(const std::string& _name, ImFont* _pFont)
    {
        auto it = m_installedFonts.find(_name);
        if (it != std::end(m_installedFonts))
            return false;
        m_installedFonts[_name] = _pFont;
        return true;
    }

    ImFont* WindowManager::GetFont(const std::string& _name) const
    {
        auto it = m_installedFonts.find(_name);
        if (it == std::end(m_installedFonts))
            return nullptr;
        return m_installedFonts.at(_name);
    }

    std::vector<Window*> WindowManager::GetWindows(WindowFilter _filter /*= nullptr*/) const
    {
        std::vector<Window*> ret;
        for (auto& it : m_windows) {

            bool add = true;
            if (_filter)
                add = _filter(it.second.get());
            if (add)
                ret.push_back(it.second.get());
        }
        return ret;
    } 

	bool WindowManager::AddImageToCache(const std::string& _fileName)
	{
		if (m_imageCache.Contains(_fileName))
			return true;
		ImageExInfo imgOrig;
		if (ReadImage(_fileName, imgOrig))
		{
			ImageExInfo resized;
			resized.m_width = 128;
			resized.m_height = 128;

			if (ResizeImage(imgOrig, resized))
			{
				HWTexInfo info;
				auto pTex = CreateTexture(&GetContext(), resized, info);
				if (pTex)
				{
					imgOrig.m_rawData.clear();
					imgOrig.m_rawData.shrink_to_fit();
					ImageInfo info = {
						imgOrig,
						std::shared_ptr<HWTexture>(pTex)
					};

					m_imageCache.Insert(_fileName, info);
					return true;
				}
			}
		}

		return false;
	}

	const ImageInfo& WindowManager::GetImageFromCache(const std::string& _imgName) 
	{
        return m_imageCache.GetValue(_imgName);
	}

	void WindowManager::SortWindows(std::vector<Window*>& _windows)
    {
        std::sort(_windows.begin(), _windows.end(), [](const Window* _a, const Window* _b)
            { return _a->GetInfo().m_drawOrderId > _b->GetInfo().m_drawOrderId; } );
    }

   

}

