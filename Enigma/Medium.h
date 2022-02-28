#pragma once

#include "Defines.h"
#include "Transform.h"
#include "Spectrum.h"
#include "BBox.h"

namespace RayTrace
{
    // Medium Declarations
    class Medium {
    public:
        // Medium Interface
        virtual ~Medium() {}
        virtual Spectrum Tr(const Ray& ray, Sampler& sampler) const = 0;
        virtual Spectrum Sample(const Ray& ray, Sampler& sampler, MemoryArena& arena, MediumInteraction* mi) const = 0;
    };

    // HomogeneousMedium Declarations
    class HomogeneousMedium : public Medium {
    public:
        // HomogeneousMedium Public Methods
        HomogeneousMedium(const Spectrum& sigma_a, const Spectrum& sigma_s, float g);
        Spectrum Tr(const Ray& ray, Sampler& sampler) const override;
        Spectrum Sample(const Ray& ray, Sampler& sampler, MemoryArena& arena, MediumInteraction* mi) const override;


    private:
        // HomogeneousMedium Private Data
        const Spectrum m_sigma_a, m_sigma_s, m_sigma_t;
        const float m_g;
    };

    // GridDensityMedium Declarations
    class GridDensityMedium : public Medium {
    public:
        // GridDensityMedium Public Methods
        GridDensityMedium(const Spectrum& sigma_a, const Spectrum& sigma_s, float g,
            int nx, int ny, int nz, const Transform& mediumToWorld, const float* d);

        float Density(const Vector3f& p) const;
        float D(const Vector3i& p) const;

        Spectrum Tr(const Ray& ray, Sampler& sampler) const override;
        Spectrum Sample(const Ray& ray, Sampler& sampler, MemoryArena& arena, MediumInteraction* mi) const override;


    private:
        // GridDensityMedium Private Data
        const Spectrum m_sigma_a,
                               m_sigma_s;
        const float m_g;
        const int   m_nx, m_ny, m_nz;
        const Transform m_worldToMedium;
        std::unique_ptr<float[]> m_density;
        float m_sigma_t;
        float m_invMaxDensity;
    };

   
    
   
}