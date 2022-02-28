#pragma once
#include "FrontEndDef.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "HWStates.h"
#include "HWFormats.h"
#include "HWTexInfo.h"
#include "HWFrameBufferClear.h"
#include "SceneItem.h"

namespace RayTrace
{
    struct PassInput
    {
        int         m_texUnit;
        std::string m_name;
        HWTexInfo   m_texInfo;
    };

    struct PassOutput
    {
        int         m_texUnit;
        HWTexInfo   m_texInfo;
    };
    
    
    
    class HWPass
    {
    public:

        HWPass(Context* _pContext, const std::string& _name = "Pass",
            SceneItemCollectorUPtr _pCollector = nullptr, 
            RenderState* _state = nullptr);

        virtual void            Begin( eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera);
        virtual void            Process();
        virtual void            End();

        void                    SetEnabled(bool _enabled);
        bool                    IsEnabled() const;
        void                    SetClearFrameBuffer(bool _clear);
        void                    SetShader(ShaderProgram* _pActiveShader);
        void                    SetMaterial( HWMaterial* _pMaterial );
        const HWViewport&       GetViewport() const { return m_activeView;}


    protected:

        const HWCamera*         m_pActiveCamera  = nullptr;      
        SceneItemCollectorUPtr  m_collector      = nullptr;
        Context*                m_pContext       = nullptr;
        HWMaterial*             m_activeMaterial = nullptr; //overrules object's material
        ShaderProgram*          m_activeShader   = nullptr; //overrules object's shader
        RenderState*            m_activeState    = nullptr;

        HWViewport              m_activeView;    
        HWRenderer&             m_renderer;

        std::string             m_name;
        FrameBufferClear        m_clear;
        SceneItemVector         m_passSceneItems; //items that belong to this pass to be drawn
        bool                    m_enabled          = true;  
        bool                    m_clearFrameBuffer = false;

        std::vector<PassInput>  m_inputs;
        std::vector<PassOutput> m_outputs;    

    };

    class EditorPass : public HWPass
    {
    public:      

        EditorPass(Context* _pContext, const std::string& _name = "EditorPass", 
            SceneItemCollectorUPtr _pCollector = nullptr, RenderState* _state = nullptr);


        void                    Begin(eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera) override;
        void                    Process() override;
        void                    End() override;    
        const uint32_t*         GetObjectIdPixels() const;
        const float*            GetDepthPixels() const;
        int                     GetViewportDownScale() const;

    private:
        void                    BinItems();
        void                    DrawToObjectIdBuffer();
        void                    CopyPixelsFromGpu();
        void                    DrawGrid();
        void                    DrawEditorSprites();
        void                    DrawSelectedMeshes();
        void                    Clear();

        HWFrameBuffer*          m_objectIdsFrameBuffer = nullptr;
        std::vector<uint32_t>   m_objIdPixels;
        std::vector<float>      m_depthPixels;
        uint32_t                m_windowId = INVALID_ID;
        int                     m_downScaleRatio = 4;    
        Vector2f                m_spritScale = { 64.0f, 64.0f };

        SceneItemVector         m_meshes, //meshes both indexed and non-indexed
                                m_sprites;
    };

    /*
        @brief: Depth pre-pass
    */
    class DepthPrePass : public HWPass
    {
    public:
        DepthPrePass(Context* _pContext, const std::string& _name = "DepthPass", SceneItemCollectorUPtr _pCollector = nullptr, RenderState* _state = nullptr);


        void                    Begin(eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera) override;
        void                    Process() override;
        void                    End() override;

        void                    DrawOpaque();
        void                    DrawMasked();


    private:
        void                    BinItems(); //sort items in a opaqua and masked vector

        SceneItemVector         m_opaqueItems;
        SceneItemVector         m_maskedItems;

        HWFrameBuffer*          m_depthFrameBuffer = nullptr;

        int                     m_width,
                                m_height;
     
        struct DataImpl;
        std::unique_ptr<DataImpl> m_pImpl;
    };



  
}