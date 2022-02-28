#pragma once

#include "Window.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
    //Statusbar
    //////////////////////////////////////////////////////////////////////////
    class Statusbar : public Window
    {
    public:
        Statusbar(const WindowInfo& _hint) : Window(_hint) {}


        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;

    };
}