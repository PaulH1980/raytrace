
#include "UiHelperFuncs.h"
#include "ImGuizmo.h" 
#include <set>
#include <backends\imgui_impl_sdl.h>
#include <backends\imgui_impl_opengl3.h>
#include "SDL_keyboard.h"
#include "EventHandler.h"
#include "HWCamera.h"


#include "Context.h"
#include "InputHandler.h"
#include "SceneManager.h"
#include "EditorLayer.h"
#include "SceneView.h"
#include "WrappedEntity.h"
#include "HistoryStack.h"
#include "HistoryItems.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "Logger.h"

#include "Selection.h"

namespace RayTrace
{
    class SelectionModeHistoryItem : public HistoryItem
    {
    public:
        SelectionModeHistoryItem(Context* _pContext, SceneSelection::eSelectionMode _exec, SceneSelection::eSelectionMode _unExec)
            : HistoryItem(_pContext, "Selection Mode")
            , m_exec(_exec)
            , m_unExec(_unExec)
        {

        }
        bool Undo() override
        {
            return Apply(m_unExec);
        }

        bool Redo() override
        {
            return Execute();
        }

        bool Execute() override
        {
            return Apply(m_exec);
        }

    private:

        bool Apply(SceneSelection::eSelectionMode _mode)
        {
            return false;
        }

        SceneSelection::eSelectionMode m_exec,
                       m_unExec;
    };
    
    
    
    
    void SelectionModified(Context* _pContext, const UUIDVector& _entities)
    {
        //notify listeners
        EventBase evt(eEvents::EVENT_OBJECT_SELECTION, _pContext->GetElapsedTime());
        evt.m_data["selectedObjects"] = _entities;
        _pContext->AddEvent(evt);
    }

   

	BBox3f GetBoundingBox(Context* _pContext, const EntityVector& _entities)
	{
        BBox3f retVal;
        for (auto pEnt : _entities)
        {
            const auto& transform = pEnt->GetWorldTransform();
            const auto  bbox      = pEnt->GetBoundingBox();
            auto  val = Vector3f(transform * Vector4f(bbox.m_min, 1.0f));
            retVal.addPoint(Vector3f(transform * Vector4f(bbox.m_min, 1.0f)));
            retVal.addPoint(Vector3f(transform * Vector4f(bbox.m_max, 1.0f)));
        }
        return retVal;
	}

	//////////////////////////////////////////////////////////////////////////
    //SelectionTool
    //////////////////////////////////////////////////////////////////////////
    SceneSelection::SceneSelection(Context* _pContext, EditorLayer* _pLayer)
        : ObjectBase(_pContext)      
        , m_pEditLayer(_pLayer)
    {
      
    }

    void SceneSelection::DrawSelection(const SceneView* _pView)
    {
        //selection #todo move to global vars?
        const static uint32_t SelectionRectColor = ImGui::GetColorU32(ImVec4(0.15f, 0.5, 1.0f, 0.33f));
        const static uint32_t SelectionLineColor = ImGui::GetColorU32(ImVec4(0.15f, 0.5, 1.0f, 1.0f));
        const static uint32_t SelectionLineWidth = 3;

        auto* window = ImGui::GetCurrentWindow();
        auto* drawList = window->DrawList;
        const auto topLeft = ImVec2(_pView->GetWindowPos().x, _pView->GetWindowPos().y);
        const auto botRight = ImVec2(topLeft.x + m_width, topLeft.y + m_height);
        if (!m_leftMouseDown)
            return;

        const auto numPoints = m_clickPoints.size();
        if (m_selectionType == eSelectionType::SELECTION_TYPE_RECT)
        {
            assert(numPoints == 1);
            const auto& lastPoint = m_clickPoints[0];
            BBox2i bounds(lastPoint, m_currentPoint);
            if (bounds.area() > 0) {
                const auto a = topLeft + ToImVec2(bounds.m_min);
                const auto b = topLeft + ToImVec2(bounds.m_max);
                drawList->AddRectFilled(a, b, SelectionRectColor, 0.0f, ImDrawCornerFlags_None);
                drawList->AddRect(a, b, SelectionLineColor, 0.0f, ImDrawCornerFlags_None, SelectionLineWidth);
            }
        }
    }

 
    void SceneSelection::Clear()
    {
        m_clickPoints.clear();        
        m_leftMouseDown = false;
    }


    bool SceneSelection::IsActive() const
    {
        return m_leftMouseDown;
    }

    void SceneSelection::SetSelectionMode(eSelectionMode _mode)
    {
        if (m_selectionMode != _mode)
        {
            auto selMode = std::make_unique<SelectionModeHistoryItem>(&GetContext(), _mode, m_selectionMode);
            GetContext().GetHistory().Execute(std::move(selMode));
        }             
    }

    SceneSelection::eSelectionMode SceneSelection::GetSelectionMode() const
    {
        return m_selectionMode;
    }

    bool SceneSelection::HandleRectangleSelect(Vector2i _p1, Vector2i _p2)
    {
        //Single pick ? Make sure we bbox2 is valid!
        if (_p1 == _p2) {
            _p2.x += 1;
            _p2.y += 1;
        }

        BBox2i bounds(_p1, _p2);
        if (bounds.area() <= 0)
            return false;

        std::set<uint32_t> selectedObjectsSet;

        auto SelectObjectByIdFilter = [&](ObjectBase* _pObject) {
            for (auto id : selectedObjectsSet) {
                if (id == _pObject->GetObjectId())
                    return true;
            }
            return false;
        };


        for (int y = bounds.m_min.y; y < bounds.m_max.y; ++y)
        for (int x = bounds.m_min.x; x < bounds.m_max.x; ++x)
        {
            const auto [ID, WORLDPOS] = m_pEditLayer->GetObjectUnderCursor(Vector2i(x, y));
            if (ID != INVALID_ID) {
                selectedObjectsSet.insert(ID);
            }
        }

        auto selectedObject = GetContext().GetSceneManager().GetObjects(SelectObjectByIdFilter);
        return HandleSelectionGeneric(selectedObject);

        return false;
    }


    bool SceneSelection::HandleSelectionGeneric(const EntityVector& _selectedObjects)
    {
        const bool ctrlHeld = GetContext().GetInputHandler().CtrlDown();
        const bool shiftHeld = GetContext().GetInputHandler().ShiftDown();
        const auto objVector = GetContext().GetSceneManager().GetObjects();
        const auto selectionBefore = GetUuids( GetContext().GetSceneManager().GetObjects(SelectObjectFilter));

        if (ctrlHeld) //xor selected items
        {
            for (auto pObj : _selectedObjects)
            {
                pObj->GetObjectFlags().m_field.m_selected = !pObj->GetObjectFlags().m_field.m_selected;
            }
        }
        else if (shiftHeld) //add to selection
        {
            SelectObjects(_selectedObjects, true);
        }
        else //regular operation clear current selection then add 
        {
            SelectObjects(objVector, false);
            SelectObjects(_selectedObjects, true);
        }
        const auto selectionAfter = GetUuids( GetContext().GetSceneManager().GetObjects(SelectObjectFilter));
               
        auto pHistory = std::make_unique<SelectionHistory>(&GetContext(), selectionAfter, selectionBefore);
        return GetContext().GetHistory().Execute(std::move(pHistory));
    }

    bool SceneSelection::CanPerformSelection(const MouseInfo& _info) const
    {
        if (GetContext().GetInputHandler().AltDown())
            return false;
        //HACK scene selection shouldn't be aware of any imguizmo...stuff
        if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
            return false;

        if (m_selectionType == eSelectionType::SELECTION_TYPE_NONE)
            return false;

        if (_info.m_mouseButton != eMouseButtons::LEFT_MOUSE_BUTTON)
            return false;

        return true;
    }

    bool SceneSelection::KeyDown(const KeyInfo& _info)
    {
        return false;
    }

    bool SceneSelection::KeyUp(const KeyInfo& _info)
    {
        return false;
    }

    bool SceneSelection::MouseMove(const MouseInfo& _info)
    {
        if (GetContext().GetInputHandler().AltDown())
            return false;
        //HACK scene selection shouldn't be aware of any imguizmo...tuff
        if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
            return false;

        if (m_leftMouseDown)
        {
            m_currentPoint = Vector2i(_info.m_x, _info.m_y);
            return true;
        }
        return false;
    }

    bool SceneSelection::MouseDown(const MouseInfo& _info)
    {
        if (!CanPerformSelection(_info))
            return false;

        if (m_selectionType == eSelectionType::SELECTION_TYPE_RECT)
        {
            assert(m_leftMouseDown == false);
            const auto point = Vector2i(_info.m_x, _info.m_y);
            m_clickPoints.push_back(point);
            m_currentPoint = point;
            m_leftMouseDown = true;
            return true;
        }
        return false;
    }

    bool SceneSelection::MouseUp(const MouseInfo& _info)
    {
        if (GetContext().GetInputHandler().AltDown())
            return false;

        if (!m_leftMouseDown)
            return false;

        if (m_selectionType == eSelectionType::SELECTION_TYPE_RECT)
        {
            assert(m_clickPoints.size() == 1);
            const auto prevPoint = m_clickPoints[0];
            const auto curPoint = Vector2i(_info.m_x, _info.m_y);
            m_leftMouseDown = false;
            m_clickPoints.clear();
            return  HandleRectangleSelect(prevPoint, curPoint);
        }
        return false;
    }

    bool SceneSelection::MouseWheel(const MouseInfo& _info)
    {
        return false;
    }

    bool SceneSelection::Resize(const Vector2i& _size)
    {
        m_width = _size.x;
        m_height = _size.y;
        return true;
    } 

}

