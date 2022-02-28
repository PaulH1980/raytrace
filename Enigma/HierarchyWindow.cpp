#include "UiIncludes.h"
#include "HierarchyWindow.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
    //ExplorerGrid
    //////////////////////////////////////////////////////////////////////////

    void ExplorerGrid::Layout()
    {

    }

    void ExplorerGrid::Resize(int _width, int _height)
    {

    }

    void ExplorerGrid::PreDraw()
    {
        m_curLayer = nullptr;
        m_curGroup = "";
    }

    void ExplorerGrid::Draw()
    {
        bool visible = IsVisible();
        if (!visible)
            return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin(m_info.m_name.c_str(), &visible);
        ImGui::PopStyleVar();
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Scene"))
        {
           // if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
           //     SelectObjects(GetContext().GetSceneManager().GetObjects(), false);



            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            DrawGroups();
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            DrawLayers();

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Views"))
            {
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
        ImGui::End();

        SetVisible(visible);
    }

    void ExplorerGrid::PostDraw()
    {

    }


    void ExplorerGrid::DrawLayers()
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        auto& layMan = GetContext().GetLayerManager();

        const uint32_t flags = eLayer::LAYER_ALL & (~eLayer::LAYER_UI);

        if (ImGui::TreeNode("Layers")) {
            for (auto& layer : layMan.GetLayers((eLayer)flags))
            {
                DrawLayer(layer);
            }
            ImGui::TreePop();
        }
    }

    void ExplorerGrid::DrawLayer(LayerBase* _pLayer)
    {

        if (ImGui::TreeNode(_pLayer->GetLayerName().c_str())) {

            auto layerId = _pLayer->GetLayerType();
            auto LayerFilter = [&](ObjectBase* _obj) {
                return (_obj->GetLayerFlags() & layerId) != 0;
            };

            SortAndDrawObjects(GetContext().GetSceneManager().GetObjects(LayerFilter));
            ImGui::TreePop();
        }
    }

    void ExplorerGrid::SortAndDrawObjects(const EntityVector& _objects)
    {
        auto IsLight                = [](WrappedEntity* _object) { 
            return _object->HasComponent<LightComponent>();
        };
        auto IsDrawable             = [](WrappedEntity* _object) { 
            return _object->HasComponent<MeshComponent>() ||
                   _object->HasComponent<IndexedMeshComponent>();
        };
        auto IsNotDrawableOrLight   = [&](WrappedEntity* _object) {
            return !(IsLight(_object) || IsDrawable(_object));
        };

        auto drawables  = FilterObjects(_objects, IsDrawable);
        auto lights     = FilterObjects(_objects, IsLight);
        auto remaining  = FilterObjects(_objects, IsNotDrawableOrLight);

        if (ImGui::TreeNode("Drawables"))
        {
            DrawObjectList(drawables);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Lights"))
        {
            DrawObjectList(lights);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Misc"))
        {
            DrawObjectList(remaining);
            ImGui::TreePop();
        }
    }

    void ExplorerGrid::DrawObjectList(const EntityVector& _objects)
    {
        ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth |
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        ObjectBase* clickedObj = nullptr;


        for (int i = 0; i < _objects.size(); ++i)
        {
            auto _obj = _objects[i];
            auto objFlags = _obj->GetObjectFlags();
            bool isSelected = _obj->GetObjectFlags().m_field.m_selected;

            ImGuiTreeNodeFlags nodeFlags = base_flags;

            if (isSelected)
                nodeFlags |= ImGuiTreeNodeFlags_Selected;
            bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, _obj->GetObjectName().c_str(), i);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                clickedObj = _obj;
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left) &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_None)) //zoom to object on double click
            {
                clickedObj = _obj;//zoom to
                Focus(_obj);
            }

            DrawContextMenu(_obj);
        }
        if (clickedObj)
        {
            // Update selection state
                // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
            auto& flags = clickedObj->GetObjectFlags();

            if (ImGui::GetIO().KeyCtrl)
                flags.m_field.m_selected = !flags.m_field.m_selected;
            else
            {
                SelectObjects(_objects, false); //unselect all
                flags.m_field.m_selected = true;
            }
            clickedObj->SetObjectFlags(flags);
        }
    }

    void ExplorerGrid::DrawContextMenu(WrappedEntity* _obj)
    {
        auto objFlags = _obj->GetObjectFlags();


        bool visible = objFlags.m_field.m_visible;
        bool selected = objFlags.m_field.m_selected;



        if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
        {

            if (ImGui::MenuItem("Focus"))
            {
                /*   const auto svName = m_info.m_sceneViewName;
                   auto sv = GetSceneView();
                   assert(sv->m_pController);
                   auto camera = sv->m_pController->GetCamera();
                   if (DrawableBase* db = dynamic_cast<DrawableBase*>(_obj)) {
                       const auto& bb = db->GetBounds();
                       if( bb.volume() > 0.0f )
                           CenterOnBounds(camera, bb.m_min, bb.m_max);
                   }*/
                Focus(_obj);

            }

            ImGui::Separator();
            if (ImGui::MenuItem("Copy")) //Copy to clipboard
            {

            }
            if (ImGui::MenuItem("Cut")) //copy to clipboard, remove from scene
            {

            }
            if (ImGui::MenuItem("Clone")) //copy and paste
            {

            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Layers"))
            {
                if (ImGui::MenuItem("Move To"))
                {

                }
                if (ImGui::MenuItem("Copy To"))
                {

                }
                ImGui::Separator();
                if (ImGui::MenuItem("Unassign"))
                {

                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Show", nullptr, &visible))
            {

            }
            if (ImGui::MenuItem("Select", nullptr, &selected))
            {

            }

            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                // if (ImGui::IsItemClicked())
                AddLogMessage(&GetContext(), "MenuItem Clicked ", eLogLevel::LOG_LEVEL_DEBUG);
                objFlags.m_field.m_markedForDelete = true;
            }
            objFlags.m_field.m_visible = visible;
            objFlags.m_field.m_selected = selected;

            _obj->SetObjectFlags(objFlags);

            ImGui::EndPopup();
        }
    }

    void ExplorerGrid::DrawGroups()
    {


        if (ImGui::TreeNode("Groups"))
        {
            auto IsGroup = [](WrappedEntity* _obj) {
                return _obj->GetGroupName().empty() == false;
            };
            auto objects = FilterObjects(GetContext().GetSceneManager().GetObjects(IsGroup));
            std::map<std::string, EntityVector> groupMap;

            for (auto& obj : objects)
                groupMap[obj->GetGroupName()].push_back(obj);
            //draw groups(sorted)
            for (auto& [_name, _objects] : groupMap) {
                if (ImGui::TreeNode(_name.c_str())) {
                    SortAndDrawObjects(_objects);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }


    }


}

