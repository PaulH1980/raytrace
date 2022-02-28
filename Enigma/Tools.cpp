
#include "UiHelperFuncs.h"
#include "ImGuizmo.h" 
#include "UIAction.h"
#include <set>
#include <backends\imgui_impl_sdl.h>
#include <backends\imgui_impl_opengl3.h>
#include "SDL_keyboard.h"
#include "EventHandler.h"
#include "HWCamera.h"
#include "HistoryStack.h"
#include "Context.h"
#include "HistoryItems.h"
#include "InputHandler.h"
#include "SceneManager.h"
#include "EditorLayer.h"
#include "SceneView.h"
#include "Selection.h"
#include "WrappedEntity.h"
#include "Logger.h"
#include "HistoryItems.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "Tools.h"

namespace RayTrace
{
    bool ChangeTool(Context* _pContext, ToolBase* _pRequestedTool, const std::string& _subTool)
    {
		auto& toolMan = _pContext->GetToolManager();
		auto curTool = toolMan.GetActiveTool();
		//previous tool
		ToolSubTool curToolName = {
			curTool != nullptr ? curTool->GetToolName() : "",
			curTool != nullptr ? curTool->GetActiveSubTool() : ""
		};

		ToolSubTool newToolName =
		{
            _pRequestedTool->GetToolName(),
            _subTool
		};

		if (std::tie(curToolName.first, curToolName.second) != std::tie(newToolName.first, newToolName.second))
		{
			auto toolHistory = std::make_unique<ActivateToolHistory>(_pContext, newToolName, curToolName);
            return _pContext->GetHistory().Execute(std::move(toolHistory));
		}
        return false;
    }
    
    
    
    
    class TransformAction : public UiAction
    {
    public:
        TransformAction( GizmoTool* _pTool, Context* _pContext, const std::string& _name, ImGuiKeyModFlags _mod = 0, ImGuiKey _key = 0)
            : UiAction(_pContext, _name, _mod, _key)
            , m_pTool( _pTool )
        {

        }

        bool DoIt() override
        {
            if (!IsEnabled())
                return false;
            return ChangeTool(&GetContext(), m_pTool, GetName());	 
        }

        bool IsEnabled() const override
        {
	         return m_pTool->CanBeActivated();
        }

        bool IsSelected() const override
        {
            return m_pTool->GetActiveSubTool() == GetName();
        }

    private:

        GizmoTool* m_pTool = nullptr;
    };




    //////////////////////////////////////////////////////////////////////////
    //ToolManager
    //////////////////////////////////////////////////////////////////////////
    ToolManager::ToolManager(Context* _pContext)
        : SystemBase(_pContext)
    {
        GetContext().GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    }

    bool ToolManager::Clear()
    {
        m_pActiveTool = nullptr;
        m_toolMap.clear();
        return true;
    }

    ToolBase* ToolManager::GetActiveTool() const
    {
        return m_pActiveTool;
    }

    bool ToolManager::ActivateTool(ToolBase* _pActiveTool, const std::string& _subTool)
    {
        bool valid = true;    
        valid &= SetActiveTool(_pActiveTool, _subTool);
        return valid;
    }

    bool ToolManager::ActivateTool(const std::string& _toolName, const std::string& _subTool)
    {
        auto toolPtr = GetTool(_toolName);
        if (!toolPtr)
            return false;
        return ActivateTool(toolPtr, _subTool);
    }

    bool ToolManager::SetActiveTool(ToolBase* _pTool, const std::string& _subTool)
    {
        bool valid = true;
        if (_pTool != m_pActiveTool) {
            if (m_pActiveTool) {
                //valid &= m_pContext->GetInputHandler().RemoveReceiver(m_pActiveTool);
                valid &= m_pActiveTool->Deactivate();
            }
            if (_pTool) {
                 //valid &= m_pContext->GetInputHandler().AddReceiver(_pTool);
                 auto view = GetContext().GetSceneManager().GetActiveSceneView();
                 assert(view);
                 _pTool->SetSize(view->m_viewport.GetFrameBufferSize().x, view->m_viewport.GetFrameBufferSize().y);
                 _pTool->SetWindowId(view->m_viewport.GetWindowId());
                 valid &= _pTool->Activate(_subTool);
            }
        }  
        else if (m_pActiveTool) //change subtool
        {

            
            valid &= _pTool->Activate(_subTool);
        }

        m_pActiveTool = _pTool;
        return valid;
    }



    bool ToolManager::AddTool(const std::string& _name, ToolBaseUPtr _pTool,  bool _registerActions )
    {
        if (m_toolMap.find(_name) != std::end(m_toolMap)) {
            assert(false && "Duplicate Toolname");
            return false;
        }
        auto pLayer = dynamic_cast<EditorLayer*>( GetContext().GetLayerManager().GetLayer(LAYER_EDITOR));
        _pTool->SetLayer(pLayer);
        _pTool->SetToolName(_name);

		if (_registerActions) {
			for (auto action : _pTool->GetToolActions())
				pLayer->RegisterAction(action);
		}

        m_toolMap[_name] = std::move(_pTool);
       
        return true;
    }

    bool ToolManager::DeactivateTool()
    {
        if (!m_pActiveTool)
            return false;
        m_pActiveTool->Deactivate();
        m_pActiveTool = nullptr;        
        return true;
    }

    ToolBase* ToolManager::GetTool(const std::string& _toolName) const
    {
        if (m_toolMap.find(_toolName) == std::end(m_toolMap))
            return nullptr;
        return m_toolMap.at(_toolName).get();
    }


    std::vector<ToolBase*> ToolManager::GetRegisteredTools() const
    {
        std::vector<ToolBase*> ret;
        for (auto& [key, tool] : m_toolMap)
            ret.push_back(tool.get());
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////
    //ToolBase
    //////////////////////////////////////////////////////////////////////////
    ToolBase::ToolBase(Context* _pContext)
        : ObjectBase( _pContext )      
    {

    }

    const std::string& ToolBase::GetToolName() const
    {
        return m_toolName;
    }    

	const std::string& ToolBase::GetGroupName() const
	{
		return m_groupName;
	}

	const std::string& ToolBase::GetStatusBarText() const
    {
        return m_statusBarText;
    }

    const std::string& ToolBase::GetActiveSubTool() const
    {
        return m_activeSubTool;
    }


	void ToolBase::SetGroupName(const std::string& _name)
	{
		m_groupName = _name;
	}

	const ActionVector& ToolBase::GetToolActions() const
	{
        return m_toolActions;
	}

	bool ToolBase::Activate(const std::string& _subTool)
    {
        
        if (!_subTool.empty() && !HasSubTool(_subTool))
            return false;        
        bool valid = true;        
        GetObjectFlags().m_field.m_activated = true;

        m_activeSubTool = _subTool;        
        m_isActive = true;
        //notify listeners
        EventBase evt(eEvents::EVENT_TOOL_ACTIVATED, GetContext().GetElapsedTime());
        evt.m_data["toolName"] = GetToolName();
        evt.m_data["subTool"] = _subTool;
        GetContext().AddEvent(evt);

        return valid;
    }

    bool ToolBase::Deactivate()
    {
        bool valid = true;
        GetObjectFlags().m_field.m_activated = false;
        m_activeSubTool = "";
        if (valid)
            m_isActive = false;

        //notify listeners
        EventBase evt(eEvents::EVENT_TOOL_DEACTIVATED, GetContext().GetElapsedTime());
        evt.m_data["toolName"] = GetToolName();
        GetContext().AddEvent(evt);

        return valid;
    }

    bool ToolBase::CanBeActivated() const
    {
        return true;
    }

    bool ToolBase::IsActive() const
    {
        return m_isActive;
    }

    void ToolBase::DrawTool(SceneView* const _pView)
    {
      

    }


    EditorLayer* ToolBase::GetLayer() const
    {
        return static_cast<EditorLayer*>(m_pLayer);
    }

    bool ToolBase::HasSubTool(const std::string& _subTool) const
    {
        for (const auto& action : m_toolActions)
            if (action->GetName() == _subTool)
                return true;
        return false;
    }

    void ToolBase::SetToolName(const std::string& _name)
    {
        m_toolName = _name;      
    }

    void ToolBase::SetSize(int _width, int _height)
    {
        m_width = _width;
        m_height = _height;
    }

    void ToolBase::SetWindowId(WindowId   _windowId)
    {
        m_windowId = _windowId;
    }

    void ToolBase::SetLayer(LayerBase* _pActiveLayer)
    {
        assert(dynamic_cast<EditorLayer*>(_pActiveLayer) != nullptr);
        m_pLayer = _pActiveLayer;
    }

    //////////////////////////////////////////////////////////////////////////
    //GizmoTool
    //////////////////////////////////////////////////////////////////////////
    GizmoTool::GizmoTool(Context* _pContext) 
        : ToolBase(_pContext)
    {
        
        SetToolName("Transform");
        SetGroupName("Transform");
        auto OnSelectionChanged = [&](EventBase& _evt) 
        {
            const auto selection = GetStringVector(_evt.m_data["selectedObjects"]);
            PopulateTransformables(selection);            
        };       

        auto OnTransformChanged = [&](EventBase& _evt)
        {
            const auto selection = GetStringVector(_evt.m_data["changedObjects"]);
            PopulateTransformables(selection);
        };
        
        auto transAction = std::make_shared<TransformAction>(this, _pContext,  "Translate", ImGuiKeyModFlags_Ctrl, ImGuiKey_Q);
        auto rotateAction = std::make_shared<TransformAction>(this, _pContext, "Rotate", ImGuiKeyModFlags_Ctrl, ImGuiKey_W);
        auto scaleAction = std::make_shared<TransformAction>(this, _pContext, "Scale", ImGuiKeyModFlags_Ctrl, ImGuiKey_E);
        m_toolActions.push_back(transAction);
        m_toolActions.push_back(rotateAction);
        m_toolActions.push_back(scaleAction);
        
       // m_toolName = "Transform Tool";
        m_transformType = ImGuizmo::MODE::WORLD;      

        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_OBJECT_SELECTION, { this, OnSelectionChanged });
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_OBJECT_TRANSFORM_CHANGED, { this, OnTransformChanged });

    }

    void GizmoTool::DrawTool(SceneView* const _pView)
    {
        if (m_transformables.empty())
            return;

        const auto topLeft = ImVec2(_pView->GetWindowPos().x, _pView->GetWindowPos().y);
        const auto botRight = ImVec2(topLeft.x + m_width, topLeft.y + m_height);
        const auto pCamera = _pView->m_pCamera.get();
        const auto& view = pCamera->GetWorldToView();
        const auto& proj = pCamera->GetProj();

        ImGuizmo::BeginFrame();
        ImGuizmo::Enable(true);
        ImGuizmo::SetOrthographic(pCamera->IsOrtho());            
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(topLeft.x, topLeft.y, m_width, m_height);
     
        const auto invSelection = glm::inverse(m_selectionTransform);

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), (ImGuizmo::OPERATION)m_transformOp,
            (ImGuizmo::MODE)m_transformType, glm::value_ptr(m_selectionTransform), glm::value_ptr( m_deltaTransform ), GetSnapValue(), nullptr, nullptr))
        {
            m_transformActive = true;            
            for (auto& trans : m_transformables) {
                
                auto pTransComp         = trans.m_pEntity->GetComponent<TransformComponent>();  
                const auto& lastTrans   = pTransComp->m_transform;
                pTransComp->m_transform = m_deltaTransform * lastTrans;                
            }           
        }     
    }

    void GizmoTool::SetTransformType(eTransformType _type)
    {
        if (_type == eTransformType::TRANSFORM_TYPE_WORLD)
            m_transformType = ImGuizmo::MODE::WORLD;
        else
            m_transformType = ImGuizmo::MODE::LOCAL;        
    }

    void GizmoTool::Clear()
    {
        m_transformables.clear();
        m_selectedEntities.clear();
        m_selectionTransform = Identity4x4;    
        m_deltaTransform = Identity4x4;
        m_leftMouseDown = false;
    }

    void GizmoTool::SetSnapValue(eTransformOp _op, float _value)
    {
        m_snapValues[(int)_op] = _value;
    }

    bool GizmoTool::Activate(const std::string& _subTool)
    {
        if (!ToolBase::Activate(_subTool))
            return false;
        
        if (_subTool == "Translate")
            m_transformOp = ImGuizmo::OPERATION::TRANSLATE;
        else if( _subTool == "Rotate")
            m_transformOp = ImGuizmo::OPERATION::ROTATE;
        else if( _subTool == "Scale" )
            m_transformOp = ImGuizmo::OPERATION::SCALE;
        else		
            throw std::exception("Invalid Transform Operation");
        return true;
    }

    bool GizmoTool::Deactivate()
    {
        if (!ToolBase::Deactivate())
            return false;
        m_transformOp = 0;
        Clear();
        return true;
    }

    bool GizmoTool::CanBeActivated() const
    {
        return !m_transformables.empty();       
    }


    const float* GizmoTool::GetSnapValue() const
    {
        const float* pRetVal = nullptr;
        
        if (m_transformOp == ImGuizmo::OPERATION::TRANSLATE) {
            if( m_snapValues[0] > 0.0f )
                pRetVal = &m_snapValues[0];
        }
        else if (m_transformOp == ImGuizmo::OPERATION::ROTATE) {
            if (m_snapValues[1] > 0.0f)
                pRetVal = &m_snapValues[1];
        }
        else if (m_transformOp == ImGuizmo::OPERATION::SCALE)  {
            if (m_snapValues[2] > 0.0f)
                pRetVal = &m_snapValues[2];
        }
        return pRetVal;
    }

    bool GizmoTool::KeyDown(const KeyInfo& _info)
    {
        return false;
    }

    bool GizmoTool::KeyUp(const KeyInfo& _info)
    {
        return false;
    }

    bool GizmoTool::MouseMove(const MouseInfo& _info)
    {
        
        if (GetContext().GetInputHandler().AltDown())
            return false;
        return false;
    }

    bool GizmoTool::MouseDown(const MouseInfo& _info)
    {
        if (GetContext().GetInputHandler().AltDown())
            return false;

        const auto shiftDown    = GetContext().GetInputHandler().ShiftDown();
        const auto lmbDown      = _info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON;

        if (shiftDown && lmbDown && !m_leftMouseDown )
        {
            //copy objects
            auto cloned = CloneEntities( GetEntityVetor( m_transformables ));
            for (auto pEnt : cloned)
                pEnt->GetObjectFlags().m_field.m_selected = false;                       
            
            auto addObjects = std::make_unique<AddObjectsHistory>( &GetContext(), cloned, true );
            GetContext().GetHistory().Execute(std::move(addObjects));

            m_leftMouseDown = true;
        }


        return false;
    }

    bool GizmoTool::MouseUp(const MouseInfo& _info)
    {
     
        if (GetContext().GetInputHandler().AltDown())
            return false;


        if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON)
            m_leftMouseDown = false;
        
        const auto currentTransforms = GetTransformable(m_selectedEntities);
        assert(currentTransforms.size() == m_transformables.size());
        auto selChanged = false;
        
        for (int i = 0; i < (int)currentTransforms.size(); ++i)
        {
            const auto& a = m_transformables[i];
            const auto& b = currentTransforms[i];
            assert(a.m_entityUuid == b.m_entityUuid );
            if (a.m_startTransform != b.m_startTransform)
            {
                selChanged = true;
                break;
            }

        }
        if (selChanged) 
        {
            auto transHistory = std::make_unique<TransformHistory>(&GetContext(), currentTransforms, m_transformables);
            GetContext().GetHistory().Execute(std::move(transHistory));           
        }
        return false;
    }

    bool GizmoTool::MouseWheel(const MouseInfo& _info)
    {
        //throw std::logic_error("The method or operation is not implemented.");
        return false;
    }

    bool GizmoTool::Resize(const Vector2i& _size)
    {
        m_width = _size.x;
        m_height = _size.y;
        return true;
    }

    std::vector<Transformable> GizmoTool::GetTransformable(const UUIDVector& _entities) const
    {
        auto IsSelectedFilter = [&](WrappedEntity* _pEnt)
        {
            if (!_pEnt->GetObjectFlags().m_field.m_selected)
                return false;
            if (!_pEnt->HasComponent<TransformComponent>())
                return false;
            return true;
        };

        std::vector<Transformable> retVal;
        const auto objects = FilterObjects(  GetContext().GetSceneManager().GetEntities( _entities ), IsSelectedFilter);
     
        for (auto pObject : objects)
        {
            const auto transComp = pObject->GetComponent<TransformComponent>();
            const auto& trans = transComp->m_transform;
            const auto worldPos = trans * Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
            retVal.push_back(Transformable{ pObject->GetUUID(), trans , pObject });
        }
        return retVal;
    }

	EntityVector GizmoTool::GetEntityVetor(const std::vector<Transformable>& _transformables) const
	{
        EntityVector retVal;
		for (auto& trans : _transformables) {
            assert(trans.m_pEntity == GetContext().GetSceneManager().GetObject(trans.m_entityUuid));
			retVal.push_back(trans.m_pEntity);
		}
		return retVal;
	}

	bool GizmoTool::PopulateTransformables(const UUIDVector& _entities)
    {
        Clear();  
       
        Vector3f selectionCenter(0.f, 0.f, 0.f);
        m_transformables      = GetTransformable(_entities);
        m_selectedEntities = _entities;
        if (m_transformables.empty())
            return false;
        const auto objCount = m_transformables.size();
        
        for (const auto& transformable : m_transformables )
        {
           // const auto transComp = pObject->GetComponent<TransformComponent>();
            const auto& trans = transformable.m_startTransform;
            const auto worldPos = trans * Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
            selectionCenter += ToVector3(worldPos);            
        }
        if (objCount > 1) {
            selectionCenter /= (float)objCount;
            m_selectionTransform = Translate4x4(selectionCenter);
        }
        else
        {
            m_selectionTransform = m_transformables[0].m_startTransform;
        }
        
        return true;
    }

    
    //////////////////////////////////////////////////////////////////////////
    //PlaceSceneObjectTool
    //////////////////////////////////////////////////////////////////////////
    PlaceSceneObjectTool::PlaceSceneObjectTool(Context* _pContext)
        : ToolBase( _pContext)
    {

    }

    void PlaceSceneObjectTool::DrawTool(SceneView* const _pView)
    {

    }

    bool PlaceSceneObjectTool::Activate(const std::string& _subTool)
    {
        if (!ToolBase::Activate(_subTool))
            return false;
        return true;
    }

    bool PlaceSceneObjectTool::Deactivate()
    {
        if (!ToolBase::Deactivate())
            return false;
        return true;
    }

}

