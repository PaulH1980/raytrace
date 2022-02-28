#include "RNG.h"
#include "Renderer.h"
#include "Ray.h"
#include "Camera.h"
#include "Scene.h"
#include "Sample.h"
#include "Sampler.h"
#include "MonteCarlo.h"
#include "Primitive.h"
#include "BxDF.h"
#include "Volume.h"
#include "Interaction.h"
#include "Medium.h"
#include "MediumInterface.h"
#include "Film.h"
#include "LightDist.h"
#include "Error.h"
#include "ParameterSet.h"
#include "Spectrum.h"
#include "Concurrency.h"
#include "DirectLightingIntegrator.h"

namespace RayTrace
{


    //////////////////////////////////////////////////////////////////////////
    //DirectLightingIntegrator Implementation
    //////////////////////////////////////////////////////////////////////////
    DirectLightingIntegrator::DirectLightingIntegrator(eLightStrategy strategy, int maxDepth, std::shared_ptr<Camera> camera, std::shared_ptr<Sampler> sampler, const BBox2i& pixelBounds) : SamplerIntegrator(camera, sampler, pixelBounds),
        m_strategy(strategy),
        m_maxDepth(maxDepth)
    {

    }

    Spectrum DirectLightingIntegrator::Li(const RayDifferential& ray,
        const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const
    {
        Spectrum L(0.f);
        // Find closest ray intersection or return background radiance
        SurfaceInteraction isect;
        if (!scene.intersect(ray, &isect)) {
            for (const auto& light : scene.m_lights)
                L += light->Le(ray);
            return L;
        }

        // Compute scattering functions for surface interaction
        isect.ComputeScatteringFunctions(ray, arena);
        if (!isect.m_bsdf)
            return Li(isect.SpawnRay(ray.m_dir), scene, sampler, arena, depth);
        Vector3f wo = isect.m_wo;
        // Compute emitted light if ray hit an area light source
        L += isect.Le(wo);
        if (scene.m_lights.size() > 0) {
            // Compute direct lighting for _DirectLightingIntegrator_ integrator
            if (m_strategy == eLightStrategy::UniformSampleAll)
                L += UniformSampleAllLights(isect, scene, arena, sampler, m_nLightSamples);
            else
                L += UniformSampleOneLight(isect, scene, arena, sampler);
        }
        if (depth + 1 < m_maxDepth) {
            // Trace rays for specular reflection and refraction
            L += SpecularReflect(ray, isect, scene, sampler, arena, depth);
            L += SpecularTransmit(ray, isect, scene, sampler, arena, depth);
        }
        return L;
    }

    void DirectLightingIntegrator::Preprocess(const Scene& scene, Sampler& sampler)
    {
        if (m_strategy == eLightStrategy::UniformSampleAll)
        {
            // Compute number of samples to use for each light
            for (const auto& light : scene.m_lights)
                m_nLightSamples.push_back(sampler.RoundCount(light->m_nSamples));

            // Request samples for sampling all lights
            for (int i = 0; i < m_maxDepth; ++i) {
                for (size_t j = 0; j < scene.m_lights.size(); ++j) {
                    sampler.Request2DArray(m_nLightSamples[j]);
                    sampler.Request2DArray(m_nLightSamples[j]);
                }
            }
        }
    }

    Integrator* CreateDirectLightingIntegrator(const ParamSet& _param, const CameraPtr& _cam, const SamplerPtr& _sampler)
    {
        using Strategy = DirectLightingIntegrator::eLightStrategy;

        int maxDepth = _param.FindOneInt("maxdepth", 5);

        Strategy  strategy = Strategy::UniformSampleAll;
        std::string st = _param.FindOneString("strategy", "all");
        if (st == "one")
            strategy = Strategy::UniformSampleOne;
        else if (st == "all")
            strategy = Strategy::UniformSampleAll;

        int np;
        const int* pb = _param.FindInt("pixelbounds", &np);
        BBox2i pixelBounds = _cam->GetFilm().GetSampleBounds();
        if (pb) {
            if (np != 4) {

            }
            else {
                pixelBounds = Intersection(pixelBounds, BBox2i{ {pb[0], pb[2]}, {pb[1], pb[3]} });
            }
        }
        return new DirectLightingIntegrator(strategy, maxDepth, _cam, _sampler, pixelBounds);
    }

}

