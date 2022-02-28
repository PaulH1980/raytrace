#pragma once
#include "Defines.h"
#include "Medium.h"
#include "MediumInterface.h"
#include "Ray.h"

namespace RayTrace 
{
    static constexpr float ShadowEpsilon = 0.0001f;
    // Interaction Declarations
    class Interaction 
    {
    public:
        // Interaction Public Methods
        Interaction();
        Interaction(const Vector3f& p, const Vector3f& n, const Vector3f& pError, const Vector3f& wo, float time, const MediumInterface& mediumInterface);
        Interaction(const Vector3f& p, const Vector3f& wo, float time, const MediumInterface& mediumInterface);
        Interaction(const Vector3f& p, float time, const MediumInterface& mediumInterface);
        bool IsSurfaceInteraction() const { return m_n != Vector3f(); }
        Ray SpawnRay(const Vector3f& d) const;
        Ray SpawnRayTo(const Vector3f& p2) const;
        Ray SpawnRayTo(const Interaction& it) const;
      
        
        bool          IsMediumInteraction() const { return !IsSurfaceInteraction(); }
        const Medium* GetMedium(const Vector3f& w) const;
        const Medium* GetMedium() const;

        // Interaction Public Data
        Vector3f  m_p;
        float           m_time;
        Vector3f  m_pError;
        Vector3f  m_wo;
        Vector3f  m_n;
        MediumInterface m_mediumInterface;
    };

    class MediumInteraction : public Interaction {
    public:
        // MediumInteraction Public Methods
        MediumInteraction();
        MediumInteraction(const Vector3f& p, const Vector3f& wo, float time,
            const Medium* medium, const PhaseFunction* phase);

        bool IsValid() const;

        // MediumInteraction Public Data
        const PhaseFunction* phase;
    };

    // SurfaceInteraction Declarations
    class SurfaceInteraction : public Interaction {
    public:
        // SurfaceInteraction Public Methods
        SurfaceInteraction() {}
        SurfaceInteraction(const Vector3f& p, const Vector3f& pError,
            const Vector2f& uv,   const Vector3f& wo,
            const Vector3f& dpdu, const Vector3f& dpdv,
            const Vector3f& dndu, const Vector3f& dndv, 
            float time,
            const Shape* sh,
            int faceIndex = 0);
        void SetShadingGeometry(const Vector3f& dpdu, const Vector3f& dpdv,
            const Vector3f& dndu, const Vector3f& dndv,
            bool orientationIsAuthoritative);
        void ComputeScatteringFunctions(
            const RayDifferential& ray, MemoryArena& arena,
            bool allowMultipleLobes = false,
            eTransportMode mode = eTransportMode::TRANSPORTMODE_RADIANCE);
        void ComputeDifferentials(const RayDifferential& r) const;
        Spectrum Le(const Vector3f& w) const;

        // SurfaceInteraction Public Data
        Vector2f m_uv;
        Vector3f m_dpdu, m_dpdv;
        Vector3f m_dndu, m_dndv;
        const Shape* m_shape = nullptr;
        struct {
            Vector3f m_n;
            Vector3f m_dpdu, m_dpdv;
            Vector3f m_dndu, m_dndv;
        } shading;
        const Primitive* m_primitive = nullptr;
        BSDF* m_bsdf     = nullptr;
        BSSRDF* m_bssrdf = nullptr;
        mutable Vector3f m_dpdx, m_dpdy;
        mutable float m_dudx = 0, m_dvdx = 0, m_dudy = 0, m_dvdy = 0;

        // Added after book publication. Shapes can optionally provide a face
        // index with an intersection point for use in Ptex texture lookups.
        // If Ptex isn't being used, then this value is ignored.
        int m_faceIndex = 0;
    };

}  // namespace pbrt