#pragma once
#include "Window.h"


namespace RayTrace
{
    class PropertyGrid : public Window, public PropertyVisitor
    {
    public:
        PropertyGrid(const WindowInfo& _hint) : Window(_hint) {
        }

        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;

    private:

    };

}