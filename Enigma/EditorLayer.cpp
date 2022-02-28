#include "UiHelperFuncs.h"
#include "Context.h"
#include "HWRenderer.h"
#include "HWMesh.h"
#include "WrappedEntity.h"
#include "HWCamera.h"
#include "InputHandler.h"
#include "HWPass.h"
#include "ResourceManager.h"
#include "HWVertexBuffer.h"
#include "HWFrameBuffer.h"
#include "SceneManager.h"
#include "HWTexture.h"
#include "HWPass.h"
#include "Tools.h"
#include "HistoryStack.h"
#include "HistoryItems.h"
#include "EventHandler.h"
#include "HWTexture.h"
#include "SceneView.h"
#include "Selection.h"
#include "UIAction.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "EditorLayer.h"

namespace RayTrace
{
    EditorLayer::EditorLayer(Context* _pContext)
        : LayerBase( _pContext )
        , m_pSelectionHelper( std::make_unique<SceneSelection>( _pContext, this ) )
    {
        m_layerName = "Editor";
        m_layer = eLayer::LAYER_EDITOR;     

        auto OnMouseLeave = [&](EventBase& _evt)
        {
            m_selectionAllowed = false;
        };

        auto OnMouseEnter = [&](EventBase& _evt)
        {
            m_selectionAllowed = true;
        };

        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_SCENEWINDOW_MOUSE_LEAVE, { this, OnMouseLeave });
        GetContext().GetEventHandler().RegisterSubscriber(eEvents::EVENT_SCENEWINDOW_MOUSE_ENTERED, { this,  OnMouseEnter });


		{
			RegisterAction(std::make_shared<NewAction>(m_pContext));
			RegisterAction(std::make_shared<SaveAction>(m_pContext));
			RegisterAction(std::make_shared<SaveAsAction>(m_pContext));
			RegisterAction(std::make_shared<ImportAction>(m_pContext));
			RegisterAction(std::make_shared<ExitAction>(m_pContext));
			RegisterAction(std::make_shared<CopyAction>(m_pContext));
			RegisterAction(std::make_shared<CutAction>(m_pContext));
			RegisterAction(std::make_shared<OpenAction>(m_pContext));
			RegisterAction(std::make_shared<PasteAction>(m_pContext));
			RegisterAction(std::make_shared<DeleteAction>(m_pContext));
			RegisterAction(std::make_shared<UndoAction>(m_pContext));
			RegisterAction(std::make_shared<RedoAction>(m_pContext));

            const auto top    = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, { 0.f, -1.0f, 0.f },  { 0.f, 0.f, -1.0f }));
            const auto bottom = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, { 0.f,  1.0f, 0.f },  { 0.f, 0.f,  1.0f }));
            const auto left   = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, { -1.f,  0.0f, 0.f }, { 0.f, 1.f,  0.0f }));
            const auto right  = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, {  1.f,  0.0f, 0.f }, { 0.f, 1.f,  0.0f }));
			const auto front  = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, { 0.f,  0.0f, -1.f }, { 0.f, 1.f,  0.0f }));
			const auto back   = Transpose4x4(LookAt4x4({ 0.0f, 0.0f, 0.0f }, { 0.f,  0.0f, 1.f },  { 0.f, 1.f,  0.0f }));
            //selection
			RegisterAction("Selection_Right",  std::make_shared<ViewAction>(m_pContext, "Right",  right,  ImGuiKeyModFlags_Ctrl, ImGuiKey_1, true));
			RegisterAction("Selection_Left",   std::make_shared<ViewAction>(m_pContext, "Left",   left,   ImGuiKeyModFlags_Ctrl, ImGuiKey_2, true));
			RegisterAction("Selection_Back",   std::make_shared<ViewAction>(m_pContext, "Back",   back,   ImGuiKeyModFlags_Ctrl, ImGuiKey_3, true));
			RegisterAction("Selection_Front",  std::make_shared<ViewAction>(m_pContext, "Front",  front,  ImGuiKeyModFlags_Ctrl, ImGuiKey_4, true));
			RegisterAction("Selection_Top",    std::make_shared<ViewAction>(m_pContext, "Top",    top,    ImGuiKeyModFlags_Ctrl, ImGuiKey_5, true));
			RegisterAction("Selection_Bottom", std::make_shared<ViewAction>(m_pContext, "Bottom", bottom, ImGuiKeyModFlags_Ctrl, ImGuiKey_6, true));
            //zoom extents
            RegisterAction("Extents_Right",  std::make_shared<ViewAction>(m_pContext, "Right",  right,  0, ImGuiKey_1, false ));
            RegisterAction("Extents_Left",   std::make_shared<ViewAction>(m_pContext, "Left",   left,   0, ImGuiKey_2, false ));
			RegisterAction("Extents_Back",   std::make_shared<ViewAction>(m_pContext, "Back",   back,   0, ImGuiKey_3, false ));
			RegisterAction("Extents_Front",  std::make_shared<ViewAction>(m_pContext, "Front",  front,  0, ImGuiKey_4, false ));
			RegisterAction("Extents_Top",    std::make_shared<ViewAction>(m_pContext, "Top",    top,    0, ImGuiKey_5, false ));
			RegisterAction("Extents_Bottom", std::make_shared<ViewAction>(m_pContext, "Bottom", bottom, 0, ImGuiKey_6, false ));

		}
        m_keyRepeatTime.start();
    }

   

    void EditorLayer::PreDraw(const HWViewport& _viewport, const HWCamera* _pCamera)
    { 
        m_viewportCenter = _viewport.GetWindowPos() + (_viewport.GetSize() / 2);
        
        LayerBase::PreDraw(_viewport, _pCamera);      
    }

    void EditorLayer::Draw()
    {
        LayerBase::Draw();
    }

    void EditorLayer::PostDraw()
    {
        LayerBase::PostDraw();
    }

    void EditorLayer::DrawEditorOverlay(const SceneView* _pView)
    {
        m_pSelectionHelper->DrawSelection(_pView);   
        DrawCrossHair(_pView);       
    }

    uint32_t EditorLayer::GetObjectId(const Vector2i& _xy) const
    {
        const auto pixIndex = GetPixelIndex(_xy);
        if (pixIndex == INVALID_ID)
            return INVALID_ID;
        
        return m_pEditorPass->GetObjectIdPixels()[pixIndex];
    }

    float EditorLayer::GetDepth(const Vector2i& _xy) const
    {
        const auto pixIndex = GetPixelIndex(_xy);
        if (pixIndex == INVALID_ID)
            return InfinityF32;
        
        return m_pEditorPass->GetDepthPixels()[pixIndex];
    }

    Vector3f EditorLayer::GetWorldPosition(const Vector2i& _xy, float _depth) const
    {
        assert(m_pActiveCam && "No Active Camera");
        const float depth = GetDepth(_xy);
        if (depth == InfinityF32)
            return Vector3f(InfinityF32);
        return m_pActiveCam->ScreenToWorld( _xy.x, _xy.y, depth );
    }

    std::pair<uint32_t,Vector3f> EditorLayer::GetObjectUnderCursor() const
    {
        return m_objUnderMouse;
    }

    ObjectUnderMouse EditorLayer::GetObjectUnderCursor(const Vector2i& _xy) const
    {
        const auto depth = GetDepth(_xy);
        if (depth == InfinityF32) {          
            return { INVALID_ID, Vector3f{ InfinityF32, InfinityF32, InfinityF32 } };
        }
        const auto objId = GetObjectId(_xy);
        const auto worldPos = GetWorldPosition(_xy, depth);
        return { objId, worldPos };
    }

    void EditorLayer::AddPass(HWPass* _pPass)
    {
        auto pass = dynamic_cast<EditorPass*>(_pPass);
        if (pass){
            m_pEditorPass = pass;
        }        
       
        LayerBase::AddPass(_pPass);
    }


    SceneSelection* EditorLayer::GetSelectionHelper() const
    {
        return m_pSelectionHelper.get();
    }

    void EditorLayer::DrawCrossHair(const SceneView* _pView)
    {
        //crosshair
        const static uint32_t CrossHairColor = ImGui::GetColorU32(ImVec4(1.0f, 0.5, 0.0f, 1.0));
        const static float CrossHairSize = 11;
        const static float CrossHairLineWidth = 1.5f;

        auto* drawList = ImGui::GetCurrentWindow()->DrawList;
        const auto& pos = _pView->m_viewport.GetWindowPos();
        const auto& size = _pView->m_viewport.GetSize();
        const auto center = pos + (size / 2);

        const auto offset = CrossHairSize / 2.0f;
        drawList->AddLine(ImVec2(center.x - offset, center.y),
            ImVec2(center.x + offset, center.y),
            CrossHairColor, CrossHairLineWidth);

        drawList->AddLine(ImVec2(center.x, center.y - offset),
            ImVec2(center.x, center.y + offset),
            CrossHairColor, CrossHairLineWidth);
    }

    int EditorLayer::GetPixelIndex(const Vector2i& _xy) const
    {
        const auto& vpSize   = m_pEditorPass->GetViewport().GetFrameBufferSize();
        const auto _xyCoord  = _xy / m_pEditorPass->GetViewportDownScale();
        const auto idx = vpSize.x *( vpSize.y - _xyCoord.y ) + _xyCoord.x; //flip y
        if (idx < 0 || idx >= vpSize.x * vpSize.y)
            return INVALID_ID;

        return idx;
    }

    bool EditorLayer::KeyDown(const KeyInfo& _info)
    {
		auto CtrlDown = GetContext().GetInputHandler().CtrlDown();
		auto ShiftDown = GetContext().GetInputHandler().ShiftDown();
		auto AltDown = GetContext().GetInputHandler().AltDown();

		uint32_t imgGuiMask = 0;
		if (ShiftDown)
			imgGuiMask |= ImGuiKeyModFlags_Shift;
		if (CtrlDown)
			imgGuiMask |= ImGuiKeyModFlags_Ctrl;
		if (AltDown)
			imgGuiMask |= ImGuiKeyModFlags_Alt;
		const auto imGuiKey = ImGui::FindImGuiKey( _info.m_scanCode );
		if (auto pAction = GetActionForShortCut(imgGuiMask, imGuiKey))
			return pAction->DoIt();            
		              
        return m_pSelectionHelper->KeyDown(_info);
    }

    bool EditorLayer::KeyUp(const KeyInfo& _info)
    {
        auto key = _info.m_scanCode;
        if (key == SDL_SCANCODE_ESCAPE) {
            auto selectedObjects = GetUuids(  GetContext().GetSceneManager().GetObjects(SelectObjectFilter) );
            if (!selectedObjects.empty()) {

                UUIDVector empty;
                auto pHistory = std::make_unique<SelectionHistory>(&GetContext(), empty, selectedObjects );
                return GetContext().GetHistory().Execute(std::move(pHistory));        
            }
            else if (auto pTool = GetContext().GetToolManager().GetActiveTool())
            {
                GetContext().GetToolManager().DeactivateTool();
            }
            return true;
        }               
        
        return m_pSelectionHelper->KeyUp(_info);     
    }

    bool EditorLayer::MouseMove(const MouseInfo& _info)
    {
        if (!IsActive())
            return false;
        
        assert(GetWindowId() == _info.m_windowId && "Window Id Mismatch"); //TODO support for multiple windows
        const auto xy = Vector2i(_info.m_x, _info.m_y);
        const auto depth = GetDepth(xy);
        if (depth == InfinityF32) {
            m_objUnderMouse = InvalidObjUnderMouse;
            return {};
        }
        const auto objId = GetObjectId(xy);
        const auto worldPos = GetWorldPosition(xy, depth);       
        m_objUnderMouse = ObjectUnderMouse(objId, worldPos);

        if (m_pSelectionHelper->IsActive())
            return m_pSelectionHelper->MouseMove(_info);

        return false; //handled
       
    }

    bool EditorLayer::MouseDown(const MouseInfo& _info)
    {
        if( m_selectionAllowed)
            return m_pSelectionHelper->MouseDown(_info);
        return false;
    }

    bool EditorLayer::MouseUp(const MouseInfo& _info)
    {
        if( m_pSelectionHelper->IsActive())
            return m_pSelectionHelper->MouseUp(_info);
        return false;
    }

    bool EditorLayer::MouseWheel(const MouseInfo& _info)
    {
        return false;
    }

    bool EditorLayer::Resize(const Vector2i& _size)
    {
        m_pSelectionHelper->Resize(_size);
        return false;
    }

    bool EditorLayer::MouseDoubleClicked(const MouseInfo& _info)
    {   
        return false;
    }

	UiAction* EditorLayer::GetAction(const std::string _name) const
	{
		return RayTrace::GetAction(m_actionMap, _name);
	}

	UiAction* EditorLayer::GetActionForShortCut(ImGuiKeyModFlags _keyMod, ImGuiKey _key) const
	{
		
        if (m_keyRepeatTime.elapsedSeconds() < 0.25f)
            return nullptr;
        m_keyRepeatTime.reset();

		for (const auto& [key, action] : m_actionMap)
		{
			if (action->IsEnabled() && action->IsKeyModPressed(_keyMod, _key))
				return action.get();
		}
      
		return nullptr;
	}

	bool EditorLayer::RegisterAction(std::shared_ptr<UiAction> _pAction)
	{
        return RegisterAction(_pAction->GetName(), _pAction);
	}

	bool EditorLayer::RegisterAction(const std::string& _lookupName, std::shared_ptr<UiAction> _pAction)
	{
		if (ContainsAction(m_actionMap, _lookupName )) {
			AddLogMessage(&GetContext(), fmt::format("Action Not Registered {}", _lookupName ), eLogLevel::LOG_LEVEL_WARNING);
			return false;
		}
		m_actionMap[_lookupName] = std::move(_pAction);
		return true;
	}

	const ActionMap& EditorLayer::GetActions() const
	{
		return m_actionMap;
	}


}


