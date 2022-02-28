#pragma once
#include "Shape.h"
namespace RayTrace
{
    class Nurbs : public Shape {
    public:

    };

    ShapesVector		CreateNURBSShape(const Transform* _o2w, const Transform* _w2o,
        bool _reverseOrientation, const ParamSet& _set);
}
