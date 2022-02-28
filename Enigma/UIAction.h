#pragma once
#include "FrontEndDef.h"
#include "imgui.h"
namespace RayTrace
{
	class UiAction
	{
	public:

		UiAction(Context* _pContext, const std::string& _name, ImGuiKeyModFlags _mod = 0, ImGuiKey _key = 0);

		virtual bool DoIt() = 0;
		virtual bool IsEnabled() const { return m_enabled; }
		virtual bool IsSelected() const { return m_selected; }
		virtual bool IsCheckbox() const { return m_isCheckBox; }
		virtual bool IsKeyModPressed(ImGuiKeyModFlags _flags = 0, ImGuiKey _key = 0);

		const std::string& GetName()const;
		const std::string& GetShortCut() const;

		Context&		 GetContext() const;
		

		std::string		 m_name;
		std::string		 m_shortCut;
		bool			 m_enabled = true;
		bool			 m_selected = false;
		bool			 m_isCheckBox = false;

		ImGuiKeyModFlags m_mod = 0;
		ImGuiKey		 m_key = 0;
		
		Context* m_pContext;
	};


	class NewAction : public UiAction
	{
	public:
		NewAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class SaveAction : public UiAction
	{
	public:
		SaveAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class SaveAsAction : public UiAction
	{
	public:
		SaveAsAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class ImportAction : public UiAction
	{
	public:
		ImportAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class OpenAction : public UiAction
	{
	public:
		OpenAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};


	class ExitAction : public UiAction
	{
	public:
		ExitAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};


	class CopyAction : public UiAction
	{
	public:
		CopyAction(Context* _pContext);
		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class CutAction : public UiAction
	{
	public:
		CutAction(Context* _pContext);

		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class UndoAction : public UiAction
	{
	public:
		UndoAction(Context* _pContext);

		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class RedoAction : public UiAction
	{
	public:
		RedoAction(Context* _pContext);

		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class PasteAction : public UiAction
	{
	public:
		PasteAction(Context* _pContext);

		bool DoIt() override;
		bool IsEnabled() const override;
	};

	class DeleteAction : public UiAction
	{
	public:
		DeleteAction(Context* _pContext);

		bool DoIt() override;
		bool IsEnabled() const override;
	};


	class ViewAction : public UiAction
	{
	public:
		ViewAction(Context* _pContext, const std::string& _name, const Matrix4x4& _orientation, ImGuiKeyModFlags _mod = 0, ImGuiKey _key = 0, bool _focusOnSelection = false, float _time = 0.5f);

		bool DoIt() override;
		bool IsEnabled() const override;

	private:
		Matrix4x4 m_orientation;
		bool      m_selection = false; // selection or 'all'
		float     m_intepolationTime = 0.5f;
	};




	inline bool	  ContainsAction(const ActionMap& _map, const std::string& _name)
	{
		return _map.find(_name) != std::end(_map);
	}

	inline UiAction* GetAction(const ActionMap& _map, const std::string& _name)
	{
		if (!ContainsAction(_map, _name))
			return nullptr;
		return _map.at(_name).get();
	}

	


}