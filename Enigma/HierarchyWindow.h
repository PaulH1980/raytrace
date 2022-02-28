#pragma once

#include "Window.h"

namespace RayTrace
{



    class ExplorerGrid : public Window
    {
    public:




        ExplorerGrid(const WindowInfo& _hint) : Window(_hint) {

        }


        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;

    private:
        // void DrawLayerItems( )
        // void DrawGroupedItems(const ObjVector& _objects);

        void DrawLayers();
        void DrawLayer(LayerBase* _pLayer);
        void DrawGroups();

        void SortAndDrawObjects(const EntityVector& _objects);
        /*
        */
        void DrawObjectList(const EntityVector& _objects);

        void DrawContextMenu(WrappedEntity* _obj);


        LayerBase* m_curLayer = nullptr;
        std::string m_curGroup;

    };
}