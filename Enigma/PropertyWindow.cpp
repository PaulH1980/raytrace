#include "UiIncludes.h"
#include "PropertyVisitor.h"

namespace RayTrace
{




    //////////////////////////////////////////////////////////////////////////
    //PropertyGrid
    //////////////////////////////////////////////////////////////////////////

    void PropertyGrid::Layout()
    {

    }

    void PropertyGrid::Resize(int _width, int _height)
    {

    }

    void PropertyGrid::PreDraw()
    {

    }

    void PropertyGrid::Draw()
    {
        bool visible = IsVisible();
        if (!visible)
            return;

        auto SelectedFilter = [](ObjectBase* _obj) {
            return _obj->GetObjectFlags().m_field.m_selected != 0;
        };

        auto selection = GetContext().GetSceneManager().GetObjects(SelectedFilter);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin(m_info.m_name.c_str(), &visible);
        ImGui::PopStyleVar();      

        auto numItems = selection.size();
        if (numItems == 1) //draw object properties
        {
           
            auto pObject = selection[0];
            auto objProps = pObject->GetProperties();
            for (auto& curPropGroup : objProps)
            {
                ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
                if (ImGui::CollapsingHeader(curPropGroup->m_name.c_str()))
                {
                    //Draw all member properties
                    ImGui::Separator();
                    ImGui::Indent(20.0f);
                    for ( auto& memberProp : curPropGroup->GetProperties() )
                    {
                        UiPropVisitor propVisitor(this, pObject->GetUUID(), curPropGroup->m_name, memberProp);
                        std::visit(propVisitor, memberProp.m_prop);
                        ImGui::Separator();
                    }
                    ImGui::Unindent(20.0f);
                }
            }

        }
        else if (numItems > 1)
        {
            if (!selection.empty()) {
                for (auto obj : selection)
                    ImGui::Text(obj->GetObjectName().c_str());
            }
        }

        else {
            ImGui::Text("Nothing Selected");
        }

        ImGui::End();

        SetVisible(visible);


    }

    void PropertyGrid::PostDraw()
    {

    }






}

