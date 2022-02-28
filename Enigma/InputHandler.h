#pragma once
#include "FrontEndDef.h"
#include "SystemBase.h"
namespace RayTrace
{
    struct MouseInfo;
    struct KeyInfo;
    class  InputReceiver;

    enum eMouseButtons
    {
        LEFT_MOUSE_BUTTON = 0,
        RIGHT_MOUSE_BUTTON,
        MIDDLE_MOUSE_BUTTON,
        X1_MOUSE_BUTTON,
        X2_MOUSE_BUTTON,
        NUM_MOUSE_BUTTONS
    };



    uint32_t  MouseMask(eMouseButtons _button);
    MouseInfo GetMouseInfo(EventBase& _evt);
    KeyInfo   GetKeyInfo(EventBase& _evt);   
   


    enum eKeyboardModifiers : uint32_t
    {
        KEY_MOD_LSHIFT = 0x01,
        KEY_MOD_RSHIFT = 0x02,

        KEY_MOD_LALT  = 0x04,
        KEY_MOD_RALT  = 0x08,

        KEY_MOD_LCTRL = 0x10,
        KEY_MOD_RCTRL = 0x20,

        KEY_MOD_ALT = KEY_MOD_LALT | KEY_MOD_RALT,
        KEY_MOD_CTRL = KEY_MOD_LCTRL | KEY_MOD_RCTRL,
        KEY_MOD_SHIFT = KEY_MOD_LSHIFT | KEY_MOD_RSHIFT,
    };

    struct InputInfo {
        int         m_windowId    = 0;
        uint32_t    m_keyModFlags = 0x0;
        float       m_evtTime     = 0.0f;
    };


    struct KeyInfo : public InputInfo {
        int      m_scanCode     = { -1 };
        bool     m_keyDown      = { false };
        float    m_keyDownTime  = { 0.0f };
      
    };  

    struct MouseInfo : public InputInfo
    {
        int   m_mouseButton        = { -1 };
        int   m_x                = { -1 };
        int   m_y                = { -1 };
        int   m_deltaX                = { -1 };
        int   m_deltaY                = { -1 };
        bool  m_mouseDown        = { false };
        float m_mouseDownTime    = { 0.0f };      
        float m_mouseWheelDelta  = { 0.0f };
        uint32_t m_mouseButtons  = 0x0;       
    };

    class InputHandler : public SystemBase
    {
        RT_OBJECT(InputHandler, SystemBase)
    public:
        InputHandler(Context* _pContext);
   
        virtual ~InputHandler();

        bool        PostConstructor() override;

        bool        Clear() override;
        void        ClearReceivers();

        KeyInfo     GetKey(int _key) const;

        bool        KeyIsDown(int _key) const;
        bool        KeyIsMapped(int _key) const;
        bool        MouseIsDown(eMouseButtons _button) const;

        bool        ShiftDown() const;
        bool        CtrlDown() const;
        bool        AltDown() const;

        bool        AddReceiver(InputReceiver* _pReceiver);
        bool        RemoveReceiver(InputReceiver* _pReceiver);
        bool        ContainsReceiver(InputReceiver* _pReceiver);

    protected:

        std::array<KeyInfo   , 512>  m_inputKeys;
        std::array<MouseInfo , 5>    m_inputMouse;

        uint32_t    m_keyBoardModifiers = { 0 };
        uint32_t    m_mouseDownFlags = { 0 };

        std::vector<InputReceiver*> m_receivers; //non owning ptr
    };

    class InputReceiver 
    {

    public:

        InputReceiver();
        virtual ~InputReceiver();

      
        
        /*
            @brief handle input events, if one of the following function returns
            'true' it is assumed the event has been handled by it so it
            shouldn't propagate further
        */
    protected:
       

    private:
        friend class InputHandler;
        friend class SceneView;
        virtual bool KeyDown(const KeyInfo& ) { return false; }
        virtual bool KeyUp(const KeyInfo& ) { return false; }
        virtual bool MouseMove(const MouseInfo& ) { return false; }
        virtual bool MouseDown(const MouseInfo& ) { return false; }
        virtual bool MouseDoubleClicked(const MouseInfo& ) { return false; }
        virtual bool MouseUp(const MouseInfo& ) { return false; }
        virtual bool MouseWheel(const MouseInfo& ) { return false; }
        virtual bool Resize(const Vector2i& ) { return false; }
    
    };

   

}