#include "UiIncludes.h"
#include "HistoryItems.h"
#include "EditorLayer.h"
#include "HistoryStack.h"
#include "MainMenubar.h"

namespace RayTrace
{

   
    inline void ActionMenuItem(UiAction* _pAction)
    {
        assert(_pAction);        
        bool enabled  = _pAction->IsEnabled();
        bool selected = _pAction->IsSelected();
        if( ImGui::MenuItem(_pAction->GetName().c_str(), _pAction->GetShortCut().c_str(), &selected, enabled) )
        {
            _pAction->DoIt();
        }
    }

    inline void ActionMenuItem( const ActionMap& _map, const std::string& _actionName) {
        ActionMenuItem( GetAction(_map, _actionName ) );
    }


    //////////////////////////////////////////////////////////////////////////
   //MainMenubar 
   //////////////////////////////////////////////////////////////////////////
    void MainMenubar::Layout()
    {

    }

    void MainMenubar::Resize(int _width, int _height)
    {
        UNUSED(_width)
        UNUSED(_height)
    }

    void MainMenubar::PreDraw()
    {

    }

    void MainMenubar::Draw()
    {
        // Menu
        auto& context = m_info.m_manager->GetContext();
        auto* editLayer  = dynamic_cast<EditorLayer*>( context.GetLayerManager().GetLayer(LAYER_EDITOR));
        assert(editLayer);
        const auto& actionMap = editLayer->GetActions();
       // const auto& actionMap = m_info.m_manager->m_actionMap;

        if (ImGui::BeginMainMenuBar())
        {
            m_info.m_manager->m_menuBarHeight = ImGui::GetCurrentWindow()->MenuBarHeight();

            if (ImGui::BeginMenu("File")) //file menu
            {
                ActionMenuItem(actionMap, "New");
                ActionMenuItem(actionMap, "Open");
                ImGui::Separator();
                ActionMenuItem(actionMap, "Import");
                ImGui::Separator();
                ActionMenuItem(actionMap, "Save");
                ActionMenuItem(actionMap, "Save As");
                ImGui::Separator();
                ActionMenuItem(actionMap, "Exit");    
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) //edit menu
            {
                ActionMenuItem(actionMap, "Copy");
                ActionMenuItem(actionMap, "Cut");
                ActionMenuItem(actionMap, "Paste");
                ImGui::Separator();
				ActionMenuItem(actionMap, "Undo");
				ActionMenuItem(actionMap, "Redo");
                ImGui::Separator();
                ActionMenuItem(actionMap, "Delete");     

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
              
                auto tools = GetContext().GetToolManager().GetRegisteredTools();
                for (auto curTool : tools)
                {
                    if (!curTool->GetGroupName().empty())
                    {
                        if (ImGui::BeginMenu(curTool->GetGroupName().c_str()))
                        {
                            for ( auto& action : curTool->GetToolActions() )
                            {
                                ActionMenuItem(action.get());
                            }

                            ImGui::EndMenu();
                        }
                    }
                   
                    else
                    {
						for (auto& action : curTool->GetToolActions())
						{
							ActionMenuItem(action.get());
						}
                    }                    
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {

                if (ImGui::BeginMenu("Windows"))
                {
                    for (auto it = m_info.m_manager->Begin(); it != m_info.m_manager->End(); ++it) {
                        auto& window = it->second;
                        const auto& info = window->GetInfo();
                        if (!info.m_fields.m_toggleable)
                            continue;
                        bool visible = info.m_fields.m_visible;
                        ImGui::MenuItem(info.m_name.c_str(), NULL, &visible);
                        window->SetVisible(visible);
                    }
                    ImGui::EndMenu();//Show
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Views"))
                {
                    if (ImGui::BeginMenu("Selection"))
                    {
							ActionMenuItem(actionMap, "Selection_Top");
							ActionMenuItem(actionMap, "Selection_Right");
							ActionMenuItem(actionMap, "Selection_Front");
							ActionMenuItem(actionMap, "Selection_Back");
							ActionMenuItem(actionMap, "Selection_Left");
							ActionMenuItem(actionMap, "Selection_Bottom");											
                        
                        ImGui::EndMenu(); //Standard
                    }
					if (ImGui::BeginMenu("Extents"))
					{
						ActionMenuItem(actionMap, "Extents_Top");
						ActionMenuItem(actionMap, "Extents_Right");
						ActionMenuItem(actionMap, "Extents_Front");
						ActionMenuItem(actionMap, "Extents_Back");
						ActionMenuItem(actionMap, "Extents_Left");
						ActionMenuItem(actionMap, "Extents_Bottom");

						ImGui::EndMenu(); //Standard
					}

                    ImGui::Separator();
                    if (ImGui::MenuItem("Focus", "F", nullptr, true ) )
                    {

                    }
                    ImGui::EndMenu(); //Views
                }
                ImGui::EndMenu(); //Windows
            }

            if (ImGui::BeginMenu("Help"))
            {
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void MainMenubar::PostDraw()
    {

    }
}

