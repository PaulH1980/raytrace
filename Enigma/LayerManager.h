#pragma once
#include "LayerBase.h"

namespace RayTrace
{
    static std::vector<eLayer> g_layers = {
        LAYER_PRE_RENDER,     
        LAYER_WORLD_SHADOWS                 ,
        LAYER_WORLD_SOLID                   ,
        LAYER_WORLD_TRANS                   ,
        LAYER_DEBUG_3D                      ,
        LAYER_EDITOR                        ,
        LAYER_SKYBOX                        ,
        LAYER_POST_RENDER                   ,
        LAYER_DEBUG_2D                      ,
        LAYER_OVERLAY                       ,
        LAYER_UI
    };
    class LayerInputOutput
    {
    public:

        std::vector<HWTexture*> m_textures;
    };



    //////////////////////////////////////////////////////////////////////////
    //LayerManager Declarations
    //////////////////////////////////////////////////////////////////////////   
    class LayerManager : public SystemBase
    {
        RT_OBJECT(LayerManager, SystemBase)

    public:
        LayerManager(Context* _pContext);

        bool                    PostConstructor() override;
       

        bool                    Clear() override;

        virtual void            Update(float _dt) override;
 
        virtual void            Draw(  uint32_t _layerFlags, const HWViewport& _view, const HWCamera* _pCamera);

        virtual void            ActivateLayers(uint32_t _layerFlags, bool _activate);
       
        LayerBase*              GetLayer(eLayer _layer) const;
        bool                    AddLayer(eLayer _layer, std::unique_ptr<LayerBase> _pLayer);

        std::vector<SceneView*> GetActiveViews() const;

        std::vector<LayerBase*> GetLayers( eLayer _flags =  eLayer::LAYER_ALL ) const;
    
    private:
        bool                    ConstructReceivers(bool _activeSceneView);

        using LayerMap = std::map<eLayer, std::unique_ptr<LayerBase>>;
        LayerMap m_layerMap;        
    };
}