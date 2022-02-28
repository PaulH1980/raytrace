
#include "SystemBase.h"

#include "InputHandler.h"

namespace RayTrace
{
    
    auto SelectObjectFilter = [&](ObjectBase* _pObject) {
        return _pObject->GetObjectFlags().m_field.m_selected == true;
    };
    
    
    void SelectionModified(Context* _pContext, const UUIDVector& _entities);
    BBox3f GetBoundingBox(Context* _pContext, const EntityVector& _entities);
    


   

    //class SelectionMode
    //{
    //public:
    //  

    //    SelectionMode(Context* _pContext);

    //    void SetSelectionMode(eSelectionMode _mode);
    //    eSelectionMode GetSelectionMode() const;

    //    
    //    Context* m_pContext = nullptr;
    //};

    
    //////////////////////////////////////////////////////////////////////////
    //SelectionTool .. Always Active
    //////////////////////////////////////////////////////////////////////////
    class SceneSelection : public ObjectBase
    {

        RT_OBJECT(SceneSelection, ObjectBase)
    public:

        enum class eSelectionType
        {
            SELECTION_TYPE_NONE = 0,
            SELECTION_TYPE_RECT,
           // SELECTION_TYPE_FREE //#TODO?
        };

        enum class eSelectionMode
        {
            SELECTION_OBJECTS = 0,
            SELECTION_FACES,
            SELECTION_EDGES,
            SELECTION_VERTICES
        };




        SceneSelection(Context* _pContext, EditorLayer* _pLayer);

        void            DrawSelection(const SceneView* _pView);
        void            Clear();
        bool            IsActive() const;

        void            SetSelectionMode(eSelectionMode _mode);
        eSelectionMode  GetSelectionMode() const;


    private:
        friend class EditorLayer;

        bool            HandleRectangleSelect(Vector2i _p1, Vector2i _p2);
        bool            HandleSelectionGeneric(const EntityVector& _selectedObjects);
        bool            CanPerformSelection(const MouseInfo& _info) const;

        bool            KeyDown(const KeyInfo& _info) ;
        bool            KeyUp(const KeyInfo& _info) ;
        bool            MouseMove(const MouseInfo& _info) ;
        bool            MouseDown(const MouseInfo& _info) ;
        bool            MouseUp(const MouseInfo& _info) ;
        bool            MouseWheel(const MouseInfo& _info) ;
        bool            Resize(const Vector2i& _size) ;
       

        eSelectionType m_selectionType = eSelectionType::SELECTION_TYPE_RECT;
        eSelectionMode m_selectionMode = eSelectionMode::SELECTION_OBJECTS;

        EditorLayer* m_pEditLayer = nullptr;

        std::vector<Vector2i> m_clickPoints;
        Vector2i              m_currentPoint;
        bool                  m_leftMouseDown = false;
        int                   m_width = 0,
                              m_height = 0;
    };

   


}