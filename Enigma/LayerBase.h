#pragma once

#include "FrontEndDef.h"
#include "Defines.h"
#include "SystemBase.h"
#include "InputHandler.h"
#include "SceneItem.h"
#include "Transform.h"


namespace RayTrace
{
    
    /*
        @brief: This must match the ubo's in shaders, both component-and alignment wise
    */
    struct PerLayer
    {
        Matrix4x4 proj,
                  view,
                  skyView,
                  projInv,
                  viewInv,
                  ortho2dProj,
                  ortho2dView;
        Vector4f  cameraPos,
                  cameraRight,
                  cameraUp,
                  cameraForward,                  
                  viewport,
                  frustum,
                  time,
                  sunLightDir,
                  sunLightColor,
                  selectionColor;



        bool operator == (const PerLayer& _rhs) const
        {
            return std::tie(proj, view, skyView, projInv, viewInv, ortho2dProj, ortho2dView, cameraPos, cameraRight, cameraUp, 
                cameraForward, viewport, frustum, time, sunLightDir, sunLightColor, selectionColor) ==
                   std::tie(_rhs.proj, _rhs.view, _rhs.skyView, _rhs.projInv, _rhs.viewInv, _rhs.ortho2dProj, _rhs.ortho2dView, _rhs.cameraPos, _rhs.cameraRight, _rhs.cameraUp,
                       _rhs.cameraForward, _rhs.viewport, _rhs.frustum, _rhs.time, _rhs.sunLightDir, _rhs.sunLightColor, _rhs.selectionColor);

        }

        bool operator !=(const PerLayer& _rhs) const
        {
            return !(*this == _rhs);
        }
    };
    
    
    
    
    //////////////////////////////////////////////////////////////////////////
    //Interface Declarations
    //////////////////////////////////////////////////////////////////////////        
    class LayerBase : public ObjectBase, public InputReceiver
    {
        RT_OBJECT(LayerBase, ObjectBase)

    public:

        LayerBase(Context* _pContext);

        virtual void             PreDraw( const HWViewport& _viewport, const HWCamera* _pCamera );
        virtual void             Draw();
        virtual void             PostDraw();
        virtual void             Attach();
        virtual void             Detach();
      
        virtual PerLayer         GetPerLayerData(const HWCamera* _pCamera) const;

        void                     SetActive(bool _v);
        bool                     IsActive() const;
        virtual void             AddPass(HWPass* _pPass);
        void                     SetWindowId(uint32_t _windowId);
        uint32_t                 GetWindowId() const;
       
        HWRenderer&              GetRenderer();
        const std::string&       GetLayerName() const;

        eLayer                   GetLayerType() const;


    protected:
        bool        m_active  = true;
        uint32_t    m_windowId = INVALID_ID;
        eLayer      m_layer = eLayer::LAYER_UNDEFINED;
        const HWCamera*   m_pActiveCam = nullptr;

        PassVector  m_layerPasses;
        PassVector  m_activePasses;
        
        std::map<std::string, std::unique_ptr<HWBufferBase>> m_perLayerBlocks;        

        std::string m_layerName;
    };
}