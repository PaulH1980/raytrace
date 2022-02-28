#pragma once

#include "LayerManager.h"
#include "Timer.h"

namespace RayTrace
{
    static const auto InvalidObjUnderMouse = ObjectUnderMouse(INVALID_ID, Vector3f(InfinityF32));
    
    
    //////////////////////////////////////////////////////////////////////////
    //SkyBoxLayer Declarations
    //////////////////////////////////////////////////////////////////////////     
    class EditorLayer : public LayerBase
    {
        RT_OBJECT(EditorLayer, LayerBase)
    public:

        EditorLayer(Context* _pContext);

        virtual void        PreDraw(const HWViewport& _viewport, const HWCamera* _pCamera);
        virtual void        Draw();
        virtual void        PostDraw();
        void                DrawEditorOverlay(const SceneView* _pView);

        uint32_t            GetObjectId(const Vector2i& _xy ) const;
        float               GetDepth(const Vector2i& _xy) const;
        Vector3f            GetWorldPosition(const Vector2i& _xy, float _depth ) const;

        ObjectUnderMouse    GetObjectUnderCursor() const;
        ObjectUnderMouse    GetObjectUnderCursor(const Vector2i& _xy) const;

        void                AddPass(HWPass* _pPass) override;

        SceneSelection*     GetSelectionHelper() const;

		UiAction*           GetAction(const std::string _name) const;
		UiAction*           GetActionForShortCut(ImGuiKeyModFlags _keyMod, ImGuiKey _key) const;
		bool                RegisterAction(std::shared_ptr<UiAction> _pAction);
        bool                RegisterAction(const std::string& _lookupName, std::shared_ptr<UiAction> _pAction);
		const ActionMap&    GetActions() const;
        
                                
    private:      
        void         DrawCrossHair(const SceneView* _pView);
        
        int          GetPixelIndex(const Vector2i& _xy) const;

        bool         KeyDown(const KeyInfo& _info)override;
        bool         KeyUp(const KeyInfo& _info)override;
        bool         MouseMove(const MouseInfo& _info)override;
        bool         MouseDown(const MouseInfo& _info)override;
        bool         MouseUp(const MouseInfo& _info)override;
        bool         MouseWheel(const MouseInfo& _info)override;
        bool         Resize(const Vector2i& _size)override;
        bool         MouseDoubleClicked(const MouseInfo& _info) override;

        Vector2i     m_viewportCenter;
        bool         m_selectionAllowed = false;
        ActionMap    m_actionMap;

        mutable Timer  m_keyRepeatTime;
        float        m_keyRepeatTreshold = 1.0f;
        float        m_lastKeyRepeatTime = 0;
       
        EditorPass*  m_pEditorPass = nullptr;
        ObjectUnderMouse m_objUnderMouse = InvalidObjUnderMouse;

        std::unique_ptr<SceneSelection> m_pSelectionHelper;
    };
}