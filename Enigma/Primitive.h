#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Defines.h"
#include "MediumInterface.h"
#include "Transform.h"


namespace RayTrace
{
	
#pragma region Primitive
	class Primitive: std::enable_shared_from_this<Primitive> {
	public:
		Primitive();
		virtual ~Primitive();

  
        virtual BBox3f				worldBound() const = 0;
        virtual bool						intersect(const Ray& r, SurfaceInteraction*) const = 0;
        virtual bool						intersectP(const Ray& r) const = 0;
        virtual const AreaLight* getAreaLight() const = 0;
        virtual const Material*  getMaterial() const = 0;
        virtual void						computeScatteringFunctions(SurfaceInteraction* isect,MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const = 0;


		// Primitive Public Data
		const uint32_t m_primitiveId;
	protected:
		// Primitive Protected Data
		static uint32_t m_nextprimitiveId;
	};
#pragma endregion


#pragma region GeometricPrimive
	// GeometricPrimitive Declarations
	class GeometricPrimitive : public Primitive {
	public:
		GeometricPrimitive(const std::shared_ptr<Shape>& s,
			const std::shared_ptr<Material>& m,
			const std::shared_ptr<AreaLight>& area,
			const MediumInterface& _mi );


		
       BBox3f					worldBound() const override;
       bool							intersect(const Ray& r, SurfaceInteraction*) const override;
       bool							intersectP(const Ray& r) const override;
       const AreaLight*  getAreaLight() const override;
       const Material*   getMaterial() const override;
       void						    computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const override;


	private:
		// GeometricPrimitive Private Data
		std::shared_ptr<Shape>		m_pShape;
		std::shared_ptr<Material>	m_pMaterial;
		std::shared_ptr<AreaLight>	m_pAreaLight;
		MediumInterface					m_mi;
	};
#pragma endregion

#pragma region TransformedPrimitive
	// TransformedPrimitive Declarations
	class TransformedPrimitive : public Primitive {
	public:
		// TransformedPrimitive Public Methods
		TransformedPrimitive( const std::shared_ptr<Primitive>& prim, const AnimatedTransform& p2w );


        BBox3f					worldBound() const override;
        bool							intersect(const Ray& r, SurfaceInteraction*) const override;
        bool							intersectP(const Ray& r) const override;
        const AreaLight*		getAreaLight() const override;
        const Material*		getMaterial() const override;
        void						    computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const override;


	private:
		// TransformedPrimitive Private Data
		std::shared_ptr<Primitive>	  m_pPrimitive;
		const AnimatedTransform m_primitiveToWorld;
	};
#pragma endregion

#pragma region Aggregate
	class Aggregate : public Primitive
	{
	public:
        // Aggregate Public Methods
        const AreaLight* getAreaLight() const override;
        const Material*  getMaterial() const override;
        void			 computeScatteringFunctions(SurfaceInteraction* isect,
			MemoryArena& arena, eTransportMode mode,
            bool allowMultipleLobes) const;
	};
#pragma endregion
}