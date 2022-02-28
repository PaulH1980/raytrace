#include "Context.h"
#include "HWRenderer.h"
#include "HWMesh.h"
#include "ResourceManager.h"
#include "HWVertexBuffer.h"
#include "SceneManager.h"
#include "HWTexture.h"
#include "HWPass.h"
#include "SkyLayer.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
   //SkyBoxLayer Implementation
   //////////////////////////////////////////////////////////////////////////
    SkyBoxLayer::SkyBoxLayer(Context* _pContext)
        : LayerBase(_pContext)
    {
        m_layerName = "Sky";
        m_layer = eLayer::LAYER_SKYBOX;
    } 
}


