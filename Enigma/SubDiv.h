#pragma once
#include "Defines.h"
#include "Shape.h"
namespace RayTrace
{
	class LoopSubdiv : public Shape
	{
	public:
	};

    ShapesVector  CreateLoopSubdiv(const Transform* _o2w, const Transform* _w2o,
        bool _reverseOrientation, const ParamSet& _param);

	//class LoopSubdiv {
	//public:

	//	// LoopSubdiv Public Methods
	//	LoopSubdiv(const Transform* o2w, const Transform* w2o, bool ro, int nt, int nv, const int* vi, const Vector3* P, int nlevels);
	//	~LoopSubdiv();
	//	bool		canIntersect() const;
	//	void		refine(ShapesVector& refined) const;
	//	BBox	objectBound() const;
	//	BBox	worldBound() const;
	//private:
	//
	//
	//};
}