#pragma once

#include "LayerManager.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
    //SkyBoxLayer Declarations
    //////////////////////////////////////////////////////////////////////////     
    class OverlayLayer : public LayerBase
    {
    public:

        OverlayLayer(Context* _pContext);
                

        void Draw() override;
   

    private:

        

    };
}