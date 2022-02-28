#pragma once
#include "Integrator.h"

namespace RayTrace
{

    // VolPathIntegrator Declarations
    class VolPathIntegrator : public SamplerIntegrator {
    public:
        // VolPathIntegrator Public Methods
        VolPathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const BBox2i& pixelBounds, float rrThreshold = 1,
            const std::string& lightSampleStrategy = "spatial");

        void        Preprocess(const Scene& _scene, Sampler& _sampler) override;
        Spectrum    Li(const RayDifferential& _ray, const Scene& _scene,
            Sampler& _sampler, MemoryArena& _arena, int _depth) const override;

    private:
        // VolPathIntegrator Private Data
        const int                           m_maxDepth;
        const float                         m_rrThreshold;
        const std::string                   m_lightSampleStrategy;
        std::unique_ptr<LightDistribution>  m_lightDistribution;
    };

    VolPathIntegrator* CreateVolPathIntegrator(
        const ParamSet& params, std::shared_ptr<Sampler> sampler,
        std::shared_ptr<Camera> camera);
}