
#include "Memory.h"
#include "Sample.h"
#include "Sampler.h"
#include "Interaction.h"
#include "MediumInterface.h"
#include "Volume.h"
#include "Error.h"
#include "Medium.h"




namespace RayTrace
{
    GridDensityMedium::GridDensityMedium(const Spectrum& sigma_a, const Spectrum& sigma_s,
        float g, int nx, int ny, int nz, const Transform& mediumToWorld, const float* d)
        : m_sigma_a(sigma_a)
        , m_sigma_s(sigma_s)
        , m_g(g)
        , m_nx(nx)
        , m_ny(ny)
        , m_nz(nz)
        , m_worldToMedium(mediumToWorld.inverted())
        , m_density(new float[nx * ny * nz])
    {
        //densityBytes += nx * ny * nz * sizeof(float);
        memcpy((float*)m_density.get(), d, sizeof(float) * nx * ny * nz);
        // Precompute values for Monte Carlo sampling of _GridDensityMedium_
        m_sigma_t = (m_sigma_a + m_sigma_s)[0];
        if (Spectrum(m_sigma_t) != m_sigma_a + m_sigma_s)
            Error(
                "GridDensityMedium requires a spectrally uniform attenuation "
                "coefficient!");
        float maxDensity = 0;
        for (int i = 0; i < nx * ny * nz; ++i)
            maxDensity = std::max(maxDensity, m_density[i]);
        m_invMaxDensity = 1.0f / maxDensity;
    }

    float GridDensityMedium::D(const Vector3i& p) const
    {
        BBox3i sampleBounds(Vector3i(0, 0, 0), Vector3i(m_nx, m_ny, m_nz));
        if (!sampleBounds.insideExlusive(p))
            return 0.f;

        int idx = (p.z * m_ny + p.y) + m_nx + p.x;
        return m_density[idx];
    }

    Spectrum GridDensityMedium::Tr(const Ray& ray, Sampler& sampler) const
    {
        return {};
    }

    Spectrum GridDensityMedium::Sample(const Ray& ray, Sampler& sampler, MemoryArena& arena, MediumInteraction* mi) const
    {
        return {};
    }

    float GridDensityMedium::Density(const Vector3f& p) const
    {
        return 0;
    }




    HomogeneousMedium::HomogeneousMedium(const Spectrum& sigma_a, const Spectrum& sigma_s, float g) 
        : m_sigma_a(sigma_a),
        m_sigma_s(sigma_s),
        m_sigma_t(sigma_s + sigma_a),
        m_g(g)
    {

    }

    Spectrum HomogeneousMedium::Tr(const Ray& ray, Sampler& sampler) const
    {
        return Exp(-m_sigma_t * std::min(ray.m_maxT * ray.m_dir.length(), MaxF32));
    }

    Spectrum HomogeneousMedium::Sample(const Ray& ray, Sampler& sampler, MemoryArena& arena, MediumInteraction* mi) const
    {
        // Sample a channel and distance along the ray
        int channel = std::min((int)(sampler.Get1D() * Spectrum::nSamples),
            Spectrum::nSamples - 1);
        float dist = -std::log(1 - sampler.Get1D()) / m_sigma_t[channel];
        float t = std::min(dist / ray.m_dir.length(), ray.m_maxT);
        bool sampledMedium = t < ray.m_maxT;
        if (sampledMedium)
            *mi = MediumInteraction(ray.scale(t), -ray.m_dir, ray.m_time, this,
                ARENA_ALLOC(arena, HenyeyGreenstein)(m_g));

        // Compute the transmittance and sampling density
        Spectrum Tr = Exp(-m_sigma_t * std::min(t, MaxF32) * ray.m_dir.length());

        // Return weighting factor for scattering from homogeneous medium
        Spectrum density = sampledMedium ? (m_sigma_t * Tr) : Tr;
        float pdf = 0;
        for (int i = 0; i < Spectrum::nSamples; ++i) pdf += density[i];
        pdf *= 1 / (float)Spectrum::nSamples;
        if (pdf == 0) {
            //assert(Tr.IsBlack());
            pdf = 1;
        }
        return sampledMedium ? (Tr * m_sigma_s / pdf) : (Tr / pdf);
    }
}



