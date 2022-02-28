#pragma once


#include "Integrator.h"

namespace RayTrace
{
    // PathIntegrator Declarations
    class PathIntegrator : public SamplerIntegrator {
    public:
        // PathIntegrator Public Methods
        PathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const  BBox2i& pixelBounds, float rrThreshold = 1,
            const std::string& lightSampleStrategy = "spatial");

        void Preprocess(const Scene& scene, Sampler& sampler);
        Spectrum Li(const RayDifferential& ray, const Scene& scene,
            Sampler& sampler, MemoryArena& arena, int depth) const;

    private:
        // PathIntegrator Private Data
        const int           m_maxDepth;
        const float         m_rrThreshold;
        const std::string   m_lightSampleStrategy;
        std::unique_ptr<LightDistribution> m_lightDistribution;
    };

    Integrator* CreatePathIntegrator(const ParamSet& _param, const CameraPtr& _cam, const SamplerPtr& _sampler);
}