#include "Context.h"
#include "HWRenderer.h"
#include "HWMesh.h"
#include "HWVertexBuffer.h"
#include "HWTexture.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Controller.h"
#include "HWPass.h"
#include "WorldLayer.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
  //WorldLayer Implementation
  //////////////////////////////////////////////////////////////////////////
    WorldLayer::WorldLayer(Context* _pContext)
        : LayerBase(_pContext)
    {
        m_layerName = "World";
        m_layer     = eLayer::LAYER_WORLD_SOLID;   



       //_pContext-> GetResourceManager().AddCamera("__DefaultCamera", std::make_unique<HWCamera>(1, 1));
       //_pContext-> GetResourceManager().AddController("__DefaultController",
       //     std::make_unique<CameraController>(this, GetResourceManager().GetCamera("__DefaultCamera")));

    }

   



  

}


