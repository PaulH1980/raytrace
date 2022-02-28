#include "UiIncludes.h"
#include "FlowWindow.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
    //FlowWindow
    //////////////////////////////////////////////////////////////////////////
    FlowWindow::FlowWindow(const WindowInfo& _hint) : Window(_hint)
    {

    }

    void FlowWindow::Layout()
    {

    }



    void FlowWindow::Resize(int _width, int _height)
    {

    }

    void FlowWindow::PreDraw()
    {

    }

    void FlowWindow::Draw()
    {
        bool visible = IsVisible();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin(m_info.m_name.c_str(), &visible);
        ImGui::PopStyleVar();
        ImGui::Text(m_info.m_type.c_str());
        ImGui::End();

        SetVisible(visible);
    }

    void FlowWindow::PostDraw()
    {

    }
}

