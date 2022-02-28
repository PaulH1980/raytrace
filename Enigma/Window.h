#pragma once
#include <array>
#include "Defines.h"
#include "FrontEndDef.h"
#include "SystemBase.h"
#include "UIAction.h"
#include "LruCache.h"
#include "IO.h"
#include "InputHandler.h"

struct ImFont;

namespace RayTrace
{
    class Window;
    class WindowCursor;

   
    bool        Focus(WrappedEntity* _pObject);
   
    struct WindowInfo
    {
        std::string m_name;
        std::string m_type;
        std::string m_sceneViewName;
        Vector2f    m_pos;
        Vector2f    m_size;   
        Vector2i    m_frameBufferSize;
        WindowId    m_id            = INVALID_ID;
        uint32_t    m_drawOrderId   = 0;
        Window*     m_parent        = nullptr;
        WindowManager* m_manager    = nullptr;       

        union {
            uint32_t        m_flags = 0;
            struct {
                bool    m_visible      : 1;
                bool    m_docked       : 1;
                bool    m_noMovable    : 1;
                bool    m_toggleable   : 1;
                bool    m_hasSceneView : 1; //backed by an fbo?
            } m_fields;
        };       
    }; 

	struct ImageInfo
	{
		ImageExInfo                 m_info;
		std::shared_ptr<HWTexture>  m_pTexture;
	};


    Window* CreateWindow( const WindowInfo& _hint ); 


    class Window 
    {
    public:
        Window( const WindowInfo& _hint );

        virtual ~Window() {}

     

        virtual void Layout();
        virtual void Resize( int _width, int _height);
        virtual void PreDraw();        
        virtual void Draw();
        virtual void PostDraw();

        virtual void HandleEvents();

        virtual bool HasSceneView() const;
        virtual void SetVisible(bool _visible);
        virtual bool IsVisible() const;
        virtual bool Dock(Window* _pParent);
        virtual bool Undock();

        Context& GetContext() const;
        SceneView* GetSceneView() const;

        WindowInfo& GetInfo();
        const WindowInfo& GetInfo() const;
       

    protected:
        WindowInfo  m_info;    
     
    };

   


    class WindowManager : public SystemBase
    {

        RT_OBJECT(WindowManager, SystemBase)


    public:

        using WindowFilter = std::function<bool(Window*)>;

        typedef std::map < std::string, std::unique_ptr<Window>>::iterator iterator;       

        WindowManager(Context* _pContext, void* _windowHandle, void* _hwContextHandle);
        virtual ~WindowManager();

        bool            Clear() override;

        virtual void    PreDraw();
        virtual void    Draw();
        virtual void    PostDraw();
        virtual void    Resize(int _width, int _height);
        virtual void    Layout();
        void            Update(float _dt) override;
        bool            LoadCursor(const std::string& _cursorName, const std::string& _fileName,
             int* _hotspotX = nullptr, int* _hotspotY = nullptr);
        bool            SetCursor(const std::string& _name);


        glm::ivec2        GetApplicationSize() const {
            return m_applicationWindowSize;
        }

        virtual bool    AddWindow(const std::string& _name, std::unique_ptr<Window> _window);
        virtual Window* GetWindow(const std::string& _name) const;

        virtual bool    AddFont(const std::string& _name, ImFont* _pFont);
        virtual ImFont* GetFont(const std::string& _name) const;

        iterator        Begin() { return m_windows.begin(); }
        iterator        End() { return m_windows.end(); }

        std::vector<Window*> GetWindows(WindowFilter _filter = nullptr) const;    
        //add image to lru cache for fast retrieval
        bool             AddImageToCache(const std::string& _fileName);
        const ImageInfo& GetImageFromCache( const std::string& _imgName) ;


    public :
        int         m_toolBarHeight   = 40;
        int         m_menuBarHeight   = 0;
        int         m_statusBarHeight = 40;
        bool        m_firstTime = true;
        glm::ivec2    m_applicationWindowSize;   

        


        std::map<std::string, std::unique_ptr<WindowCursor>> m_cursors;


    private:
        void SortWindows(std::vector<Window*>& _windows);           
        std::map < std::string, std::unique_ptr<Window>> m_windows;
        std::map<std::string, ImFont*> m_installedFonts;

		std::mutex                           m_cacheMutex;
		LRUCache<std::string, ImageInfo>     m_imageCache;
     
    };
}