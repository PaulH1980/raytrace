#include "Context.h"
#include "SceneManager.h"
#include "UiIncludes.h"
#include "HistoryStack.h"
#include "Selection.h"
#include "SceneView.h"
#include "Controller.h"
#include "HistoryItems.h"
#include "UIAction.h"


namespace RayTrace
{
	namespace Action {

		auto SelectedFilter = [](ObjectBase* _obj) {
			return _obj->GetObjectFlags().m_field.m_selected != 0;
		};
	};

	UiAction::UiAction(Context* _pContext, const std::string& _name, ImGuiKeyModFlags _mod , ImGuiKey _key)
		: m_pContext(_pContext)
		, m_name( _name )
		, m_mod( _mod )
		, m_key(_key)
	{
	
		std::string sShortCut;
		if (_mod != 0)
		{
			if (_mod & ImGuiKeyModFlags_Ctrl) {
				sShortCut += "Ctrl";
			}
			if (_mod & ImGuiKeyModFlags_Shift) {
				if (sShortCut.length() != 0)
					sShortCut += "+";
				sShortCut += "Shift";
			}
			if (_mod & ImGuiKeyModFlags_Alt) {
				if (sShortCut.length() != 0)
					sShortCut += "+";
				sShortCut += "Alt";
			}
		}
		if (_key != 0)
		{
			if (sShortCut.length() != 0)
				sShortCut += "+";		
			sShortCut += ImGui::GetKeyName(_key);
			m_shortCut = sShortCut;
		}
	}

	bool UiAction::IsKeyModPressed(ImGuiKeyModFlags _flags /*= 0*/, ImGuiKey _key /*= 0*/)
	{
		return (_flags & m_mod) && (_key == m_key);
	}

	const std::string& UiAction::GetName() const
	{
		return m_name;
	}

	const std::string& UiAction::GetShortCut() const
	{
		return m_shortCut;
	}

	Context& UiAction::GetContext() const
	{
		return *m_pContext;
	}

	CopyAction::CopyAction(Context* _pContext)
		: UiAction(_pContext, "Copy",ImGuiKeyModFlags_Ctrl , ImGuiKey_C)
	{

	}

	bool CopyAction::DoIt()
	{
		if (!IsEnabled())
			return false;

		
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);
		GetContext().GetCopyPasteBuffer().SnapShot(selection);
		return true;
	}

	bool CopyAction::IsEnabled() const
	{
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);
		return !selection.empty();

	}

	UndoAction::UndoAction(Context* _pContext)
		: UiAction(_pContext, "Undo", ImGuiKeyModFlags_Ctrl, ImGuiKey_Z)
	{

	}

	bool UndoAction::DoIt()
	{
		if (!IsEnabled())
			return false;

		
		GetContext().GetHistory().Undo();
		return true;
	}

	bool UndoAction::IsEnabled() const
	{
		return GetContext().GetHistory().CanUndo();
	}


	RedoAction::RedoAction(Context* _pContext)
		: UiAction(_pContext, "Redo", ImGuiKeyModFlags_Ctrl, ImGuiKey_Y)
	{

	}

	bool RedoAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		
		GetContext().GetHistory().Redo();
		return true;
	}

	bool RedoAction::IsEnabled() const
	{
		return GetContext().GetHistory().CanRedo();
	}

	DeleteAction::DeleteAction(Context* _pContext)
		: UiAction(_pContext, "Delete", ImGuiKeyModFlags_Ctrl, ImGuiKey_Delete)
	{

	}

	bool DeleteAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		
		
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);	
		GetContext().GetHistory().Execute(std::make_unique<DeleteObjectsHistory>(&GetContext(), selection));
		return true;
	}

	bool DeleteAction::IsEnabled() const
	{
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);
		return !selection.empty();
	}

	PasteAction::PasteAction(Context* _pContext)
		: UiAction(_pContext, "Paste", ImGuiKeyModFlags_Ctrl, ImGuiKey_V)
	{

	}

	bool PasteAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		auto entities = GetContext().GetCopyPasteBuffer().GetEntities();
		GetContext().GetHistory().Execute(std::make_unique<AddObjectsHistory>(&GetContext(), entities));
		return true;
	}

	bool PasteAction::IsEnabled() const
	{
		return GetContext().GetCopyPasteBuffer().GetCount() != 0;
	}

	NewAction::NewAction(Context* _pContext)
		: UiAction(_pContext, "New", ImGuiKeyModFlags_Ctrl, ImGuiKey_N)
	{

	}

	bool NewAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		GetContext().RequestScene("");
		return true;
	}

	bool NewAction::IsEnabled() const
	{
		return true;
	}

	SaveAction::SaveAction(Context* _pContext)
		: UiAction(_pContext, "Save", ImGuiKeyModFlags_Ctrl, ImGuiKey_S)
	{

	}

	bool SaveAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		//GetContext().RequestScene("");
		return true;
	}

	bool SaveAction::IsEnabled() const
	{
		return true;
	}

	SaveAsAction::SaveAsAction(Context* _pContext)
		: UiAction(_pContext, "Save As", ImGuiKeyModFlags_Ctrl, ImGuiKey_A)
	{

	}

	bool SaveAsAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		//GetContext().RequestScene("");
		return true;
	}

	bool SaveAsAction::IsEnabled() const
	{
		return true;
	}

	ImportAction::ImportAction(Context* _pContext)
		: UiAction(_pContext, "Import", ImGuiKeyModFlags_Ctrl, ImGuiKey_I)
	{

	}

	bool ImportAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		//GetContext().RequestScene("");
		return true;
	}

	bool ImportAction::IsEnabled() const
	{
		return true;
	}

	ExitAction::ExitAction(Context* _pContext)
		: UiAction(_pContext, "Exit", ImGuiKeyModFlags_Ctrl|ImGuiKeyModFlags_Shift, ImGuiKey_F4)
	{

	}

	bool ExitAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		GetContext().RequestExit();
		return true;
	}

	bool ExitAction::IsEnabled() const
	{
		return true;
	}

	CutAction::CutAction(Context* _pContext)
		: UiAction(_pContext, "Cut", ImGuiKeyModFlags_Ctrl, ImGuiKey_X)
	{

	}

	bool CutAction::DoIt()
	{
		if (!IsEnabled())
			return false;
		
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);
		GetContext().GetCopyPasteBuffer().SnapShot(selection);
		GetContext().GetHistory().Execute(std::make_unique<DeleteObjectsHistory>(&GetContext(), selection));
		return true;
	}

	bool CutAction::IsEnabled() const
	{
		auto selection = GetContext().GetSceneManager().GetObjects(Action::SelectedFilter);
		return !selection.empty();
	}

	OpenAction::OpenAction(Context* _pContext)
		: UiAction(_pContext, "Open", ImGuiKeyModFlags_Ctrl, ImGuiKey_O)
	{

	}

	bool OpenAction::DoIt()
	{
		if (IsEnabled())
			return true;
		return false;
	}

	bool OpenAction::IsEnabled() const
	{
		return true;
	}

	ViewAction::ViewAction(Context* _pContext, const std::string& _name, 
		const Matrix4x4& _orientation, ImGuiKeyModFlags _mod, ImGuiKey _key, bool _focusOnSelection /*= false */, float _time)
		: UiAction( _pContext, _name, _mod, _key )
		, m_orientation( _orientation )
		, m_selection(_focusOnSelection )
		, m_intepolationTime( _time )
	{

	}

	bool ViewAction::DoIt()
	{
		if (!IsEnabled())
			return false;

		const auto activeView = GetContext().GetSceneManager().GetActiveSceneView();
		if (!activeView)
			return false;
		const auto objects = m_selection
			? GetContext().GetSceneManager().GetObjects(Action::SelectedFilter)
			: GetContext().GetSceneManager().GetObjects();
		const auto bbox = GetBoundingBox(&GetContext(), objects);
		const auto center = bbox.getCenter();
		auto length = Length( bbox.diagonal());
		if (length <= 0.001f)
			length = 1.0f;
		length *= 1.25f;

		const auto offsetMatrix  = Translate4x4( Vector3f( 0.f, 0.f, length ) );
		const auto curView		 = Transpose4x4( activeView->m_pCamera->GetView() );
		const auto requestedView =/* Transpose4x4*/( Translate4x4( center )  * m_orientation * offsetMatrix );

		activeView->m_pController->LookAt(curView, requestedView, length, m_intepolationTime );
		return true;
	}

	bool ViewAction::IsEnabled() const
	{
		const auto objects = m_selection
			? GetContext().GetSceneManager().GetObjects(Action::SelectedFilter)
			: GetContext().GetSceneManager().GetObjects();
		return !objects.empty();
	}

}

