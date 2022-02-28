#pragma once
#include "ObjectBase.h"

namespace RayTrace
{
    class DrawableBase : public ObjectBase
    {
        RT_OBJECT(DrawableBase, ObjectBase)
    public:
        DrawableBase(Context* _pContext);       
    };
}