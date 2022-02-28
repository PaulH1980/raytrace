#pragma once
#include <vector>
#include <memory>
#include "Defines.h"
#include "BBox.h"
#include "RNG.h"



namespace RayTrace
{
	
	class Scene
	{
		using LightVector = std::vector<LightPtr>;
	public:
		Scene( const PrimitivePtr& _accel,  
			   const LightVector			_lights);
		
        bool						intersect(const Ray& ray, SurfaceInteraction* isect) const;
        bool						intersectP(const Ray& ray) const;
        bool						intersectTr(Ray ray, Sampler& sampler, SurfaceInteraction* isect, Spectrum* transmittance) const;

		const BBox3f&			worldBound() const;
		int							getNumLights() const;

		const LightPtr&	getLight(int _idx) const;

	public:
		LightVector									   m_lights;
		LightVector									   m_infiniteLights;
		std::shared_ptr<Primitive>		   m_accel;
		BBox3f								   m_bounds;		
	};
}