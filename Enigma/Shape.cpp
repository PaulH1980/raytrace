#include <cmath>
#include <exception>
#include "Transform.h"
#include "Lights.h"
#include "MonteCarlo.h"
#include "Interaction.h"
#include "Ray.h"
#include "MediumInterface.h"
#include "Sampler.h"
#include "Shape.h"



namespace RayTrace
{
    bool AddShapeToVector(const ShapePtr& _shape, ShapesVector& _shapeVector)
    {
		const auto oldSize = _shapeVector.size();
		ShapesVector todo;
        todo.push_back(_shape);
        while (todo.size()) {
            ShapePtr sh = todo.back();
            todo.pop_back();
            if (sh->canIntersect())
                _shapeVector.push_back(sh);
            else
                sh->refine(todo);
        }
		return _shapeVector.size() > oldSize;
    }


	Shape::Shape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation) 
		: m_objectToWorld(_o2w)
		, m_worldToObject(_w2o)
		, m_reverseOrientation(_reverseOrientation)
		, m_transformSwapsHandedness(_o2w->swapsHandedness())
	{
		m_shapeId = s_nextShapeId++;
	}

	BBox3f Shape::worldBounds() const
	{
		return m_objectToWorld->transformBounds( objectBound() );
	}

    bool Shape::intersectP(const Ray& ray, bool testAlphaTexture /*= true*/) const
    {
        return intersect(ray, nullptr, nullptr, testAlphaTexture);
    }

    

    Interaction Shape::sample(const Interaction& ref, const Vector2f& u, float* pdf) const
    {
        Interaction intr = sample(u, pdf);
        Vector3f wi = intr.m_p - ref.m_p;
        if (LengthSqr(wi) == 0)
            *pdf = 0;
        else {
            wi = Normalize(wi);
            // Convert from area measure, as returned by the Sample() call
            // above, to solid angle measure.
            *pdf *= DistanceSqr(ref.m_p, intr.m_p) / AbsDot(intr.m_n, -wi);
            if (std::isinf(*pdf)) 
                *pdf = 0.f;
        }
        return intr;
    }

    float Shape::area() const
	{
		throw std::exception("Not Implemented");
		return 0.f;
	}


	

    float Shape::pdf(const Interaction& ref, const Vector3f& wi) const
    {
        // Intersect sample ray with area light geometry
        Ray ray = ref.SpawnRay(wi);
        float tHit;
        SurfaceInteraction isectLight;
        // Ignore any alpha textures used for trimming the shape when performing
        // this intersection. Hack for the "San Miguel" scene, where this is used
        // to make an invisible area light.
        if (!intersect(ray, &tHit, &isectLight, false)) 
			return 0;

        // Convert light sample weight to solid angle measure
        float pdf = DistanceSqr(ref.m_p, isectLight.m_p) /
            (AbsDot(isectLight.m_n, -wi) * area());
        if (std::isinf(pdf)) 
			pdf = 0.f;
        return pdf;
    }

    float Shape::solidAngle(const Vector3f& p, int nSamples /*= 512*/) const
    {

        Interaction ref(p, Vector3f(), Vector3f(), Vector3f(0, 0, 1), 0, MediumInterface{});

        double solidAngle = 0;
        for (int i = 0; i < nSamples; ++i) {
            Vector2f u(RadicalInverse(0, i), RadicalInverse(1, i));
            float pdf;
            Interaction pShape = sample(ref, u, &pdf);
            if (pdf > 0 && !intersectP(Ray(p, pShape.m_p - p, .999f))) {
                solidAngle += 1.0 / pdf;
            }
        }
        return solidAngle / nSamples;
    }

    //   void Shape::computeGradients(
	//	float u, float v, const Vector3f& phit, 
	//	const Vector3f& dpdu, const Vector3f& dpdv, 
	//	const Vector3f& d2Pduu, const Vector3f& d2Pduv, 
	//	const Vector3f& d2Pdvv, DifferentialGeometry* _dg ) const
	//{
	//	// Compute coefficients for fundamental forms
	//	float E = dpdu.dot(dpdu);//Dot(dpdu, dpdu);
	//	float F = dpdu.dot(dpdv);//Dot(dpdu, dpdv);
	//	float G = dpdv.dot(dpdv);//Dot(dpdv, dpdv);
	//	Vector3f N = dpdu.cross(dpdv).normalized(); //Normalize(Cross(dpdu, dpdv));
	//	float e = N.dot(d2Pduu);
	//	float f = N.dot(d2Pduv);
	//	float g = N.dot(d2Pdvv);

	//	// Compute $\dndu$ and $\dndv$ from fundamental form coefficients
	//	float invEGF2 = 1.f / (E * G - F * F);
	//	Vector3f dndu = Vector3f((f * F - e * G) * invEGF2 * dpdu +
	//		(e * F - f * E) * invEGF2 * dpdv);
	//	Vector3f dndv = Vector3f((g * F - f * G) * invEGF2 * dpdu +
	//		(f * F - g * E) * invEGF2 * dpdv);

	//	// Initialize _DifferentialGeometry_ from parametric information
	//	const Transform& o2w = *m_objectToWorld;
	//	*_dg = DifferentialGeometry(o2w.transformPoint(phit),
	//		o2w.transformVector(dpdu), o2w.transformVector(dpdv),
	//		o2w.transformNormal(dndu), o2w.transformNormal(dndv),
	//		u, v, this);
	//}

   

	uint32_t Shape::s_nextShapeId = 1;




    //ShapeSet::ShapeSet(const ShapePtr& _shape)
    //    : m_sumArea(0.f)
    //{
    //    AddShapeToVector(_shape, m_shapes );
    //    m_areas.reserve(m_shapes.size());
    //    for (const auto& shape : m_shapes) {
    //        const auto a = shape->area();
    //        m_areas.push_back(a);
    //        m_sumArea += a;
    //    }
    //    m_areaDistribution = std::make_unique<Distribution1D>(m_areas.data(), (int)m_areas.size());
    //}

    //ShapeSet::~ShapeSet()
    //{

    //}

    //Vector3f ShapeSet::Sample(const Vector3f& _point, const LightSample& _ls, Vector3f* _Ns) const
    //{
    //    int sn = m_areaDistribution->SampleDiscrete(_ls.m_uComponent, nullptr);
    //    return m_shapes[sn]->sample(_point, _ls.m_uPos[0], _ls.m_uPos[1], _Ns);
    //}

    //Vector3f ShapeSet::Sample(const LightSample& _ls, Vector3f* _Ns) const
    //{
    //    int sn = m_areaDistribution->SampleDiscrete(_ls.m_uComponent, nullptr);
    //    return m_shapes[sn]->sample(_ls.m_uPos[0], _ls.m_uPos[1], _Ns);
    //}

    //float ShapeSet::Pdf(const Vector3f& _point, const Vector3f& _wi) const
    //{
    //    float pdf = 0.f;
    //    for (int i = 0; i < m_shapes.size(); ++i)
    //        pdf += m_areas[i] * m_shapes[i]->pdf(_point, _wi);
    //    return pdf / m_sumArea;
    //}

    //float ShapeSet::Pdf(const Vector3f& _point) const
    //{
    //    float pdf = 0.f;
    //    for (int i = 0; i < m_shapes.size(); ++i)
    //        pdf += m_areas[i] * m_shapes[i]->pdf(_point);
    //    return pdf / (m_shapes.size() * m_sumArea);
    //}


}

