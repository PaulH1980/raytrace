#pragma once
#include "Integrator.h"


namespace RayTrace
{
    // DirectLightingIntegrator Declarations
    class DirectLightingIntegrator : public SamplerIntegrator {
    public:
        // LightStrategy Declarations
        enum class eLightStrategy { UniformSampleAll, UniformSampleOne };

        // DirectLightingIntegrator Public Methods
        DirectLightingIntegrator(eLightStrategy strategy, int maxDepth,
            std::shared_ptr<Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const BBox2i& pixelBounds);
        Spectrum        Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const override;
        void            Preprocess(const Scene& scene, Sampler& sampler) override;

    private:
        // DirectLightingIntegrator Private Data
        const eLightStrategy    m_strategy;
        const int               m_maxDepth;
        std::vector<int>        m_nLightSamples;
    };


    Integrator* CreateDirectLightingIntegrator(const ParamSet& _param, const CameraPtr& _cam, const SamplerPtr& _sampler);

}