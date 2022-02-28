#pragma once

#include "SystemBase.h"
#include "InputHandler.h"

namespace RayTrace
{

    class ToolManager : public SystemBase
    {
        RT_OBJECT(ToolManager, SystemBase)
    public:


        ToolManager(Context* _pContext);
        bool                    Clear() override;

        bool                    ActivateTool(const std::string& _toolName, const std::string& _subTool);
        bool                    ActivateTool(ToolBase* _pActiveTool , const std::string& _subTool);
        bool                    AddTool(const std::string& _name, ToolBaseUPtr _pTool, bool _registerActions = true);
        bool                    DeactivateTool();

        ToolBase*               GetTool(const std::string& _toolName) const;

        std::vector<ToolBase*>  GetRegisteredTools() const;
        ToolBase*               GetActiveTool() const;

    private:        
                   
        bool                    SetActiveTool(ToolBase* _pTool, const std::string& _subTool);

        ToolBase* m_pActiveTool = { nullptr };
        std::map<std::string, ToolBaseUPtr> m_toolMap;

    };

    
    //////////////////////////////////////////////////////////////////////////
    //ToolBase
    //////////////////////////////////////////////////////////////////////////
    class ToolBase : public ObjectBase, public InputReceiver
    {
        RT_OBJECT(ToolBase, ObjectBase )
    public:
        ToolBase(Context* _pContext);

        const std::string&      GetToolName() const;
        const std::string&      GetGroupName() const;
        const std::string&      GetStatusBarText() const;
        const std::string&      GetActiveSubTool() const;
        void                    SetGroupName(const std::string& _name);

        const ActionVector&     GetToolActions() const;
 
        virtual bool            Activate( const std::string& _subTool );
        virtual bool            Deactivate();
        virtual bool            CanBeActivated() const;
        bool                    IsActive() const;
        bool                    HasSubTool(const std::string& _subTool) const;     
       
        /*
            @brief: Tools will be drawn in the editor layer
        */
        virtual void            DrawTool(SceneView* const _pView);   

        EditorLayer*            GetLayer() const;

    protected:            
        std::string  m_toolName;   
        std::string  m_statusBarText;
        std::string  m_activeSubTool;
        std::string  m_groupName;
      
        WindowId     m_windowId = INVALID_ID;
        int          m_width    = 0,
                     m_height   = 0;      
        bool         m_isActive = false;
        LayerBase*   m_pLayer   = nullptr;

        ActionVector m_toolActions;

        void         SetToolName(const std::string& _name);
      
    private:
       
        void         SetSize(int _width, int _height);
        void         SetWindowId(WindowId   _windowId);
        void         SetLayer(LayerBase* _pActiveLayer);

        friend class ToolManager;
    };

    //////////////////////////////////////////////////////////////////////////
    //GizmoTool
    //////////////////////////////////////////////////////////////////////////
    class GizmoTool : public ToolBase
    {
        RT_OBJECT(GizmoTool, ToolBase)
    

    public:

        enum class eTransformType
        {
            TRANSFORM_TYPE_WORLD = 1,
            TRANSFORM_TYPE_LOCAL,
        };

        enum class eTransformOp
        {
            TRANSFORM_OP_TRANS = 0,
            TRANSFORM_OP_ROTATE,
            TRANSFORM_OP_SCALE
        };

        GizmoTool(Context* _pContext);

        void            DrawTool(SceneView* const _pView) override;    
        void            SetTransformType(eTransformType _type);

        void            Clear();

        void            SetSnapValue(eTransformOp _op, float _value = -1.0f);

        bool            Activate(const std::string& _subTool) override;
        bool            Deactivate() override;
        bool            CanBeActivated() const override;

     

    private:
        



        bool            KeyDown(const KeyInfo& _info) override;
        bool            KeyUp(const KeyInfo& _info) override;
        bool            MouseMove(const MouseInfo& _info) override;
        bool            MouseDown(const MouseInfo& _info) override;
        bool            MouseUp(const MouseInfo& _info) override;
        bool            MouseWheel(const MouseInfo& _info) override;
        bool            Resize(const Vector2i& _size) override;

        std::vector<Transformable> GetTransformable(const UUIDVector& _entities) const;

        //UUIDVector              GetEntities(const std::vector<Selectable>& _transformables) const;
        EntityVector    GetEntityVetor(const std::vector<Transformable>& _transformables) const;

        bool            PopulateTransformables( const UUIDVector& _entties );
        const float*    GetSnapValue() const;


        int            m_transformOp       = 0;
        int            m_transformType     = 0;
        bool           m_transformActive   = false;
       
        float          m_snapValues[3]     = { -1.0f }; //negative means disabled
        bool           m_leftMouseDown = false;
      
     
       

        Matrix4x4                   m_selectionTransform;
        Matrix4x4                   m_deltaTransform;
        std::vector<Transformable>  m_transformables;
        UUIDVector                  m_selectedEntities;
      
    };

    

    class PlaceSceneObjectTool : public ToolBase
    {
        RT_OBJECT(PlaceSceneObjectTool, ToolBase)
    public:

        PlaceSceneObjectTool(Context* _pContext);

        void            DrawTool(SceneView* const _pView)override;

        bool            Activate(const std::string& _subTool) override;

        bool            Deactivate() override;

    private:

    };
}