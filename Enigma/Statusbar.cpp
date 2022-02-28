#include "UiIncludes.h"
#include "EditorLayer.h"
#include "Statusbar.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
    //Statusbar
    //////////////////////////////////////////////////////////////////////////
    void Statusbar::Layout()
    {

    }

    void Statusbar::Resize(int _width, int _height)
    {

    }

    void Statusbar::PreDraw()
    {

    }

    void Statusbar::Draw()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto viewportSize = viewport->Size;
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewportSize.y - m_info.m_manager->m_statusBarHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_info.m_manager->m_statusBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = 0
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings
            ;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Begin(m_info.m_type.c_str(), NULL, window_flags);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(5);
        {
            auto activeScene = GetContext().GetActiveSceneView();
            if (activeScene) {
                const auto pos = activeScene->m_pCamera->GetPosition();
                ImGui::SameLine();
               // ImGui::Text("Position: [%1f %1f %f]", pos.x, pos.y, pos.z);
            }
           
            const auto pLayerEdit  = dynamic_cast<EditorLayer*>( GetContext().GetLayerManager().GetLayer(LAYER_EDITOR) );
            if (pLayerEdit && pLayerEdit->IsActive() )
            {
                auto objUnderMouse = pLayerEdit->GetObjectUnderCursor();
                const auto pos = objUnderMouse.second;
                if (!HasInfinities(pos)) {
                    ImGui::SameLine();
                  //  ImGui::Text("Mouse Cursor: [%1f %1f %1f]", pos.x, pos.y, pos.z);
                }
            }        
            const auto activeTool = GetContext().GetToolManager().GetActiveTool();
            if (activeTool) {
                
                ImGui::SameLine();
                ImGui::Text( fmt::format( "Active Tool: {} - {}", activeTool->GetToolName(), activeTool->GetActiveSubTool() ).c_str() );
            }
            ImGui::SameLine();
            ImGui::Button("Statusbar goes here", ImVec2(0, 30));
           
        }
       

        ImGui::End();
    }

    void Statusbar::PostDraw()
    {

    }

}

