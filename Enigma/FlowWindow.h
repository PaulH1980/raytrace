#pragma once

#include "Window.h"

namespace RayTrace
{



    class FlowWindow : public Window {
    public:
        FlowWindow(const WindowInfo& _hint);


        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;
    };

}