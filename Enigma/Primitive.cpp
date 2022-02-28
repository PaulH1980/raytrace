#include <exception>
#include "BBox.h"
#include "BxDF.h"
#include "Ray.h"
#include "Memory.h"
#include "Lights.h"
#include "Shape.h"
#include "Material.h"
#include "Interaction.h"
#include "Medium.h"
#include "Primitive.h"





namespace RayTrace
{
	uint32_t Primitive::m_nextprimitiveId = 1;
#pragma region Primitive

	Primitive::Primitive()
		: m_primitiveId(m_nextprimitiveId++)
	{

	}

    Primitive::~Primitive()
    {

    }

   /* BSDF* Primitive::getBSDF(const DifferentialGeometry& _dg, const Transform& _objectToWorld, MemoryArena& _ma) const
	{
		throw std::exception("Not Implemented");
		return nullptr;
	}

	BSSRDF* Primitive::getBSSRDF(const DifferentialGeometry& _dg, const Transform& _objectToWorld, MemoryArena& _ma) const
	{
		throw std::exception("Not Implemented");
		return nullptr;
	}*/

	const AreaLight* Primitive::getAreaLight() const
	{
		return nullptr;
	}

	/*BBox3f Primitive::worldBound() const
	{
		throw std::exception("Not Implemented");
		return {};
	}*/

	//bool Primitive::canIntersect() const
	//{
	//	return true;
	//}

	//bool Primitive::intersect(const Ray& r, Intersection* in) const
	//{
	//	throw std::exception("Not Implemented");
	//	return false;
	//}

	//bool Primitive::intersectP(const Ray& r) const
	//{
	//	throw std::exception("Not Implemented");
	//	return false;
	//}

	//void Primitive::refine(PrimitiveVector& refined)
	//{
	//	throw std::exception("Not Implemented");
	//}

	//void Primitive::fullyRefine(PrimitiveVector& refined) 
	//{
	//	PrimitiveVector todo;
	//	todo.push_back(shared_from_this());
	//	while (todo.size()) {
	//		// Refine last primitive in todo list
	//		auto prim = todo.back();
	//		todo.pop_back();
	//		if (prim->canIntersect())
	//			refined.push_back(prim);
	//		else
	//			prim->refine(todo);
	//	}
	//}
#pragma endregion

#pragma region GeometricPrimitive
	/*GeometricPrimitive::GeometricPrimitive( std::shared_ptr<Shape>& s, std::shared_ptr<Material>& m, const AreaLight* a)
		: m_pShape( s )
		, m_pMaterial( m )
		, m_pAreaLight( a )
	{
		
	}*/

    GeometricPrimitive::GeometricPrimitive(const std::shared_ptr<Shape>& s, 
		const std::shared_ptr<Material>& m, 
		const std::shared_ptr<AreaLight>& area, 
		const MediumInterface& _mi)
        : m_pShape(s)
        , m_pMaterial(m)
        , m_pAreaLight(area)
		, m_mi(_mi)
    {

    }

    /*SDF* GeometricPrimitive::getBSDF(const DifferentialGeometry& _dg, const Transform& _objectToWorld, MemoryArena& _ma) const
	{
		DifferentialGeometry dgs;
		m_pShape->getShadingGeometry(_objectToWorld, _dg, &dgs);
		return m_pMaterial->getBSDF(_dg, dgs, _ma);
	}

	BSSRDF* GeometricPrimitive::getBSSRDF(const DifferentialGeometry& _dg, const Transform& _objectToWorld, MemoryArena& _ma) const
	{
		DifferentialGeometry dgs;
		m_pShape->getShadingGeometry(_objectToWorld, _dg, &dgs);
		return m_pMaterial->getBSSRDF(_dg, dgs, _ma);
	}*/

	const AreaLight* GeometricPrimitive::getAreaLight() const
	{
		return m_pAreaLight.get();
	}

    const Material* GeometricPrimitive::getMaterial() const
    {
		return m_pMaterial.get();
    }

  

    BBox3f GeometricPrimitive::worldBound() const
	{
		return m_pShape->worldBounds();
	}

    bool GeometricPrimitive::intersect(const Ray& r, SurfaceInteraction* isect) const
    {
        float tHit;
        if (!m_pShape->intersect(r, &tHit, isect)) 
			return false;
        r.m_maxT = tHit;
        isect->m_primitive = this;
		assert(Dot(isect->m_n, isect->shading.m_n) >= 0.0f);
     
        // Initialize _SurfaceInteraction::mediumInterface_ after _Shape_
        // intersection
        if (m_mi.IsMediumTransition())
            isect->m_mediumInterface = m_mi;
        else
            isect->m_mediumInterface = MediumInterface(r.m_medium);
        return true;
    }

    bool GeometricPrimitive::intersectP(const Ray& r) const
    {
		return m_pShape->intersectP(r);
    }

    /*bool GeometricPrimitive::canIntersect() const
    {
        return m_pShape->canIntersect();
    }*/

	/*bool GeometricPrimitive::intersect(const Ray& r, Intersection* _isect) const
	{
		float thit, rayEpsilon;
		if (!m_pShape->intersect(r, &thit, &rayEpsilon, &_isect->m_dg))
			return false;
		_isect->m_primitive	    = this;
		_isect->m_worldToObject = *m_pShape->m_worldToObject;
		_isect->m_objectToWorld = *m_pShape->m_objectToWorld;
		_isect->m_shapeId		= m_pShape->m_shapeId;
		_isect->m_primitiveId   = m_primitiveId;
		_isect->m_rayEpsilon    = rayEpsilon;
		r.m_maxT = thit;
		return true;
	}

	bool GeometricPrimitive::intersectP(const Ray& r) const
	{
		return m_pShape->intersectP(r);
	}

	void GeometricPrimitive::refine(PrimitiveVector& refined)
	{
		ShapesVector r;
		m_pShape->refine(r);
		for (uint32_t i = 0; i < r.size(); ++i) {

			auto gp = std::make_shared<GeometricPrimitive>(r[i], m_pMaterial, m_pAreaLight, m_mi );
			refined.push_back(gp);
		}
	}	*/

    void GeometricPrimitive::computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        if (m_pMaterial)
			m_pMaterial->computeScatteringFunctions(isect, arena, mode, allowMultipleLobes);

		assert(Dot(isect->m_n, isect->shading.m_n) >= 0.0f);
    }

#pragma endregion

#pragma region TransformedPrimitive
	TransformedPrimitive::TransformedPrimitive(const std::shared_ptr<Primitive>& prim, const AnimatedTransform& p2w) 
		: m_pPrimitive(prim)
		, m_primitiveToWorld(p2w)
	{

	}


	BBox3f TransformedPrimitive::worldBound() const
	{
		return m_primitiveToWorld.motionBounds(m_pPrimitive->worldBound());
	}

    bool TransformedPrimitive::intersect(const Ray& r, SurfaceInteraction* isect) const
    {
        // Compute _ray_ after transformation by _PrimitiveToWorld_
        Transform InterpolatedPrimToWorld;
        m_primitiveToWorld.interpolate(r.m_time, &InterpolatedPrimToWorld);
		Transform InterpolatedWorldToPrim = InterpolatedPrimToWorld.inverted();
		Ray ray = InterpolatedWorldToPrim.transformRay(r);
        if (!m_pPrimitive->intersect(ray, isect)) 
			return false;
        r.m_maxT = ray.m_maxT;
        // Transform instance's intersection data to world space
		if (!InterpolatedPrimToWorld.isIdentity())
			*isect = InterpolatedPrimToWorld.transformSurfInteraction(*isect);

		assert(Dot(isect->m_n, isect->shading.m_n) >= 0.0f);        
        return true;
    }

    bool TransformedPrimitive::intersectP(const Ray& r) const
    {
        Transform InterpolatedPrimToWorld;
        m_primitiveToWorld.interpolate(r.m_time, &InterpolatedPrimToWorld);
        Transform InterpolatedWorldToPrim = InterpolatedPrimToWorld.inverted();
        return m_pPrimitive->intersectP( InterpolatedWorldToPrim.transformRay(r));
    }

    const AreaLight* TransformedPrimitive::getAreaLight() const
    {
		return nullptr;
    }

    const Material* TransformedPrimitive::getMaterial() const
    {
		return nullptr;
    }

    void TransformedPrimitive::computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
		throw std::exception("Not Implemented");
    }

#pragma endregion
#pragma region Aggregate
	const AreaLight* Aggregate::getAreaLight() const
	{
		throw std::exception("Not Implemented");
		return {};
	}

	
    const Material* Aggregate::getMaterial() const
    {
        throw std::exception("Not Implemented");
        return {};
    }

    void Aggregate::computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
		throw std::exception("Not Implemented");
    }

#pragma endregion
}

