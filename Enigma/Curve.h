#pragma once
#include "Shape.h"
namespace RayTrace
{

    class Curve : public Shape
    {
    public:
    };

    ShapesVector 			CreateCurveShape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, const ParamSet& _set);


}