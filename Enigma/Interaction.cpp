#include "Misc.h"
#include "MonteCarlo.h"
#include "Shape.h"
#include "Primitive.h"
#include "Lights.h"
#include "Interaction.h"





namespace RayTrace
{

    Interaction::Interaction()
        : m_time(0.f)
    {

    }

    Interaction::Interaction(const Vector3f& p, const Vector3f& n, const Vector3f& pError,
        const Vector3f& wo, float time, const MediumInterface& mediumInterface)
        : m_p(p)
        , m_time(time)
        , m_pError(pError)
        , m_wo( Normalize( wo ) )
        , m_n(n)
        , m_mediumInterface(mediumInterface)
    {

    }

    Interaction::Interaction(const Vector3f& p, const Vector3f& wo,
        float time, const MediumInterface& mediumInterface)
        : m_p(p)
        , m_time(time)
        , m_wo(wo)
        , m_mediumInterface(mediumInterface)
    {

    }

    Interaction::Interaction(const Vector3f& p, float time, const MediumInterface& mediumInterface)
        : m_p(p)
        , m_time(time)
        , m_mediumInterface(mediumInterface)
    {

    }

    Ray Interaction::SpawnRayTo(const Vector3f& p2) const
    {
        Vector3f origin = OffsetRayOrigin(m_p, m_pError, m_n, p2 - m_p);
        Vector3f d = p2 - m_p;
        return Ray(origin, d, 1 - ShadowEpsilon, m_time, GetMedium(d));
    }

    Ray Interaction::SpawnRayTo(const Interaction& it) const
    {
        Vector3f origin = OffsetRayOrigin(m_p, m_pError, m_n, it.m_p - m_p);
        Vector3f target = OffsetRayOrigin(it.m_p, it.m_pError, it.m_n, origin - it.m_p);
        Vector3f d = target - origin;
        return Ray(origin, d, 1 - ShadowEpsilon, m_time, GetMedium(d));
    }

    Ray Interaction::SpawnRay(const Vector3f& d) const
    {
        Vector3f o = OffsetRayOrigin(m_p, m_pError, m_n, d);
        return Ray(o, d, InfinityF32, m_time, GetMedium(d));
    }

    const Medium* Interaction::GetMedium(const Vector3f& w) const
    {
        return Dot(w, m_n) > 0 ? m_mediumInterface.outside : m_mediumInterface.inside;
    }

    const Medium* Interaction::GetMedium() const
    {
        assert(m_mediumInterface.inside == m_mediumInterface.outside);
        return m_mediumInterface.inside;
    }

    MediumInteraction::MediumInteraction(const Vector3f& p, const Vector3f& wo, float time, const Medium* medium, const PhaseFunction* phase) 
        : Interaction(p, wo, time, medium), phase(phase)
    {

    }

    MediumInteraction::MediumInteraction() 
        : phase(nullptr)
    {

    }

    bool MediumInteraction::IsValid() const
    {
        return phase != nullptr;
    }

    // SurfaceInteraction Method Definitions
    SurfaceInteraction::SurfaceInteraction(const Vector3f& p, const Vector3f& pError,
        const Vector2f& uv, const Vector3f& wo,
        const Vector3f& dpdu, const Vector3f& dpdv,
        const Vector3f& dndu, const Vector3f& dndv,
        float time,
        const Shape* shape,
        int faceIndex )
        : Interaction(p,  Normalize(Cross(dpdu, dpdv)), pError, wo, time, MediumInterface{ nullptr })
        , m_uv(uv)
        , m_dpdu(dpdu)
        , m_dpdv(dpdv)
        , m_dndu(dndu)
        , m_dndv(dndv)
        , m_shape(shape)
        , m_faceIndex(faceIndex)
        {
        // Initialize shading geometry from true geometry
        shading.m_n    = m_n;
        shading.m_dpdu = m_dpdu;
        shading.m_dpdv = m_dpdv;
        shading.m_dndu = m_dndu;
        shading.m_dndv = m_dndv;

        // Adjust normal based on orientation and handedness
        if (m_shape &&
            (m_shape->m_reverseOrientation ^ m_shape->m_transformSwapsHandedness)) {
            m_n *= -1;
            shading.m_n *= -1;
        }
    }

    void SurfaceInteraction::SetShadingGeometry(const Vector3f& dpdus,
        const Vector3f& dpdvs,
        const Vector3f& dndus,
        const Vector3f& dndvs,
        bool orientationIsAuthoritative) 
    {
        // Compute _shading.n_ for _SurfaceInteraction_
        shading.m_n = Normalize(Cross(dpdus, dpdvs));
        if (orientationIsAuthoritative)
            m_n = FaceForward (m_n, shading.m_n);
        else
            shading.m_n = FaceForward(shading.m_n, m_n);

        // Initialize _shading_ partial derivative values
        shading.m_dpdu = dpdus;
        shading.m_dpdv = dpdvs;
        shading.m_dndu = dndus;
        shading.m_dndv = dndvs;
    }

    void SurfaceInteraction::ComputeScatteringFunctions(const RayDifferential& ray,
        MemoryArena& arena,
        bool allowMultipleLobes,
        eTransportMode mode) 
    {
        ComputeDifferentials(ray);
        m_primitive->computeScatteringFunctions( this, arena, mode, allowMultipleLobes );
    }

    void SurfaceInteraction::ComputeDifferentials(
        const RayDifferential& ray) const {
        if (ray.m_hasDifferentials) {
            // Estimate screen space change in $\pt{}$ and $(u,v)$

            // Compute auxiliary intersection points with plane
            float d = Dot(m_n, m_p );
            float tx = -(Dot(m_n, Vector3f(ray.m_rxOrigin)) - d) / Dot(m_n, ray.m_rxDirection);
            if (std::isinf(tx) || std::isnan(tx)) goto fail;
            Vector3f px = ray.m_rxOrigin + tx * ray.m_rxDirection;
            float ty =
                -(Dot(m_n, Vector3f(ray.m_ryOrigin)) - d) / Dot(m_n, ray.m_ryDirection);
            if (std::isinf(ty) || std::isnan(ty)) goto fail;
            Vector3f py = ray.m_ryOrigin + ty * ray.m_ryDirection;
            m_dpdx = px - m_p;
            m_dpdy = py - m_p;

            // Compute $(u,v)$ offsets at auxiliary points

            // Choose two dimensions to use for ray offset computation
            int dim[2];
            if (std::abs(m_n.x) > std::abs(m_n.y) && std::abs(m_n.x) > std::abs(m_n.z)) {
                dim[0] = 1;
                dim[1] = 2;
            }
            else if (std::abs(m_n.y) > std::abs(m_n.z)) {
                dim[0] = 0;
                dim[1] = 2;
            }
            else {
                dim[0] = 0;
                dim[1] = 1;
            }

            // Initialize _A_, _Bx_, and _By_ matrices for offset computation
            float A[2][2] = { {m_dpdu[dim[0]], m_dpdv[dim[0]]},
                              {m_dpdu[dim[1]], m_dpdv[dim[1]]} };
            float Bx[2] = { px[dim[0]] - m_p[dim[0]], px[dim[1]] - m_p[dim[1]] };
            float By[2] = { py[dim[0]] - m_p[dim[0]], py[dim[1]] - m_p[dim[1]] };
            if (!SolveLinearSystem2x2(A, Bx, &m_dudx, &m_dvdx)) m_dudx = m_dvdx = 0;
            if (!SolveLinearSystem2x2(A, By, &m_dudy, &m_dvdy)) m_dudy = m_dvdy = 0;
        }
        else {
        fail:
            m_dudx = m_dvdx = 0;
            m_dudy = m_dvdy = 0;
            m_dpdx = m_dpdy = Vector3f(0, 0, 0);
        }
    }

    Spectrum SurfaceInteraction::Le(const Vector3f& w) const {
        const AreaLight* area = m_primitive->getAreaLight();
        return area ? area->L(*this, w) : Spectrum(0.f);
    }


}

