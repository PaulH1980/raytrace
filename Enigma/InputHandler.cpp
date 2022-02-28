#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "Context.h"
#include "Logger.h"
#include "EventHandler.h"
#include "InputHandler.h"

namespace RayTrace
{

	uint32_t MouseMask(eMouseButtons _button)
	{
		return 1 << (uint32_t)_button;
	}

	MouseInfo GetMouseInfo(EventBase& _evt) {
		//EventBase evt(evtId, time);
		MouseInfo ret;
		ret.m_evtTime = _evt.m_eventTime;

		ret.m_x					= GetInt(_evt.m_data["x"]			);
		ret.m_y					= GetInt(_evt.m_data["y"]			);
		ret.m_deltaX			= GetInt(_evt.m_data["deltaX"]		);
		ret.m_deltaY			= GetInt(_evt.m_data["deltaY"]		);
	    ret.m_mouseButtons      = GetInt(_evt.m_data["buttonsMask"] );
		ret.m_keyModFlags		= GetInt(_evt.m_data["keyMod"]		);
		ret.m_mouseButton		= GetInt(_evt.m_data["button"]	    );
		ret.m_mouseDownTime		= GetFloat(_evt.m_data["downTime"]	);
		ret.m_windowId			= GetInt(_evt.m_data["windowId"]	);
        ret.m_mouseWheelDelta   = GetFloat(_evt.m_data["wheelDelta"]);

		return ret;
		
	}

	KeyInfo   GetKeyInfo( EventBase& _evt)
	{
		KeyInfo ret;
		ret.m_evtTime = _evt.m_eventTime;
		ret.m_windowId		= GetInt(_evt.m_data["windowId"]);
		ret.m_keyModFlags   = GetInt(_evt.m_data["keyMod"]  );
		ret.m_scanCode      = GetInt(_evt.m_data["scanCode"]);
		ret.m_keyDownTime   = GetFloat(_evt.m_data["downTime"]);
		return ret;
	}


	InputHandler::InputHandler(Context* _pContext)
		: SystemBase(_pContext)
	{
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());	
	}

    InputHandler::~InputHandler()
    {
		Clear();
    }

    bool InputHandler::PostConstructor()
    {
        SubcriberFunc KeyDown = [=](EventBase& _evt) {
            auto keyInfo = GetKeyInfo(_evt);
            keyInfo.m_keyDown = true;
            m_keyBoardModifiers = keyInfo.m_keyModFlags;
            m_inputKeys[keyInfo.m_scanCode] = keyInfo;

            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->KeyDown(keyInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };
        SubcriberFunc KeyUp = [=](EventBase& _evt) {

            auto keyInfo = GetKeyInfo(_evt);

            m_keyBoardModifiers = keyInfo.m_keyModFlags;
            m_inputKeys[keyInfo.m_scanCode] = {};

            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->KeyUp(keyInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }

        };
        SubcriberFunc MouseDown = [=](EventBase& _evt) {
            auto mouseInfo = GetMouseInfo(_evt);
            mouseInfo.m_mouseDown = true;
            m_inputMouse[mouseInfo.m_mouseButton] = mouseInfo;
            m_keyBoardModifiers = mouseInfo.m_keyModFlags;
            m_mouseDownFlags |= MouseMask((eMouseButtons)mouseInfo.m_mouseButton);

            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->MouseDown(mouseInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };

        SubcriberFunc MouseDoubleClicked = [=](EventBase& _evt) {
            auto mouseInfo = GetMouseInfo(_evt);
            mouseInfo.m_mouseDown = true;
            m_inputMouse[mouseInfo.m_mouseButton] = mouseInfo;
            m_keyBoardModifiers = mouseInfo.m_keyModFlags;
           // m_mouseDownFlags |= MouseMask((eMouseButtons)mouseInfo.m_mouseButton);

            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->MouseDoubleClicked(mouseInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };

        SubcriberFunc MouseUp = [=](EventBase& _evt) {
            auto mouseInfo = GetMouseInfo(_evt);
            m_inputMouse[mouseInfo.m_mouseButton] = {};
            m_keyBoardModifiers = mouseInfo.m_keyModFlags;
            m_mouseDownFlags &= ~MouseMask((eMouseButtons)mouseInfo.m_mouseButton);
            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->MouseUp(mouseInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };
        SubcriberFunc MouseMove = [=](EventBase& _evt) {
            auto mouseInfo = GetMouseInfo(_evt);
            m_keyBoardModifiers = mouseInfo.m_keyModFlags;
            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->MouseMove(mouseInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };
        SubcriberFunc MouseWheel = [=](EventBase& _evt) {
            auto mouseInfo = GetMouseInfo(_evt);
            m_keyBoardModifiers = mouseInfo.m_keyModFlags;
            for (int i = m_receivers.size() - 1; i >= 0; --i)
            {
                auto it = m_receivers[i];
                if (it->MouseWheel(mouseInfo)) {
                    _evt.m_handled = true;
                    break;
                }
            }
        };
        SubcriberFunc Resize = [=](EventBase& _evt) {
            //auto mouseInfo = GetMouseInfo(_evt);		
            int width = GetInt(_evt.m_data["width"]);
            int height = GetInt(_evt.m_data["height"]);
            for (auto it : m_receivers) //always propagate resize events
                it->Resize(Vector2i(width, height));
        };


        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_KEY_DOWN, { this, KeyDown });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_KEY_UP, { this, KeyUp });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_MOUSE_DOWN, { this, MouseDown });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_MOUSE_DOUBLE_CLICKED, { this, MouseDoubleClicked });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_MOUSE_UP, { this, MouseUp });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_MOUSE_MOTION, { this, MouseMove });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_MOUSE_WHEEL, { this, MouseWheel });
        m_pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_VIEWPORT_RESIZE, { this, Resize });

        return true;
    }

    bool InputHandler::Clear()
    {
		m_receivers.clear();
		m_keyBoardModifiers = 0;
		m_mouseDownFlags = 0;
		m_inputKeys.fill({});
		m_inputMouse.fill({});
        return true;
    }

    void InputHandler::ClearReceivers()
    {
        m_receivers.clear();
    }

    KeyInfo InputHandler::GetKey(int _key) const
	{
		assert(_key >= 0 && _key < 512);		
		return m_inputKeys[_key];
	}

	bool InputHandler::KeyIsDown(int _key) const
	{
		return GetKey(_key).m_keyDown;
	}


	bool InputHandler::MouseIsDown(eMouseButtons _button) const
	{
		return m_inputMouse[_button].m_mouseDown;
	}

	bool InputHandler::ShiftDown() const
	{
		return (m_keyBoardModifiers & eKeyboardModifiers::KEY_MOD_SHIFT) != 0;
	}

	bool InputHandler::CtrlDown() const
	{
		return (m_keyBoardModifiers & eKeyboardModifiers::KEY_MOD_CTRL) != 0;
	}

	bool InputHandler::AltDown() const
	{
		return (m_keyBoardModifiers & eKeyboardModifiers::KEY_MOD_ALT) != 0;
	}

	bool InputHandler::AddReceiver(InputReceiver* _pReceiver)
	{
		if (ContainsReceiver(_pReceiver)) {
			assert(false && "Duplicate Receiver");
			return false;
		}
		m_receivers.push_back(_pReceiver);
		return true;
	}

	bool InputHandler::RemoveReceiver(InputReceiver* _pReceiver)
	{
		for (int i = 0; i < m_receivers.size(); i++)
		{
			if (m_receivers[i] == _pReceiver) {
				m_receivers.erase(m_receivers.begin() + i);
				return true;
			}
		}
		assert(false && "Receiver Not Found");
		return false;
	}

	bool InputHandler::ContainsReceiver(InputReceiver* _pReceiver)
	{
		for (auto it : m_receivers)
		{
			if (_pReceiver == it)
				return true;
		}
		return false;
	}


    InputReceiver::InputReceiver()
    {

    }

    InputReceiver::~InputReceiver()
    {

    }

   

}