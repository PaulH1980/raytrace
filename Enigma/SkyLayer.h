#pragma once

#include "LayerManager.h"

namespace RayTrace
{
    //////////////////////////////////////////////////////////////////////////
    //SkyBoxLayer Declarations
    //////////////////////////////////////////////////////////////////////////     
    class SkyBoxLayer : public LayerBase
    {
        RT_OBJECT(SkyBoxLayer, LayerBase)

    public:

        SkyBoxLayer(Context* _pContext);

    private:

    };
}