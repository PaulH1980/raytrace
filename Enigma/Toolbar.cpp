#include "UiIncludes.h"
#include "Toolbar.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
    //Toolbar
    //////////////////////////////////////////////////////////////////////////

    void Toolbar::Layout()
    {

    }

    void Toolbar::Resize(int _width, int _height)
    {

    }

    void Toolbar::PreDraw()
    {

    }

    void Toolbar::Draw()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + m_info.m_manager->m_menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_info.m_manager->m_toolBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = 0
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Begin(m_info.m_type.c_str(), NULL, window_flags);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::SetCursorPosY(5);
        ImGui::Button("Toolbar goes here", ImVec2(0, 30));

        ImGui::End();
    }

    void Toolbar::PostDraw()
    {

    }

}

