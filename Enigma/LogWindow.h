#pragma once
#include "Window.h"

namespace RayTrace
{

    class LogWindow : public Window {

     

    public:
        LogWindow(const WindowInfo& _hint);


        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;
        void Clear();
        void AddLog(const char* fmt, ...) IM_FMTARGS(2);


        struct WindowRecipient;      

        std::shared_ptr<WindowRecipient> m_recipient;

    };
}