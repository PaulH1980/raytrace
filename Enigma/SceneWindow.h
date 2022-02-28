#pragma once
#include "Window.h"

namespace RayTrace
{



    class SceneWindow : public Window
    {
    public:
        SceneWindow(const WindowInfo& _hint)
            : Window(_hint) {


        }

        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;
        void HandleEvents() override;

        Vector2i m_imagePos;
        Vector2i m_size;
        bool     m_leftMouseDown = false;
        bool     m_focused = false;
        bool     m_hovered = false;

    private:
        void HandleCursors(const Vector2i& _localPos);
        void HandleMouse(const Vector2i& _localPos);
        void HandleKeyboard();

    };
}