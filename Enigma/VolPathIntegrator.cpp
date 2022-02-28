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

#include "VolPathIntegrator.h"

namespace RayTrace
{

    VolPathIntegrator::VolPathIntegrator(int maxDepth, std::shared_ptr<Camera> camera, std::shared_ptr<Sampler> sampler, 
        const BBox2i& pixelBounds, float rrThreshold /*= 1*/, const std::string& lightSampleStrategy /*= "spatial"*/)
        : SamplerIntegrator(camera, sampler, pixelBounds),
        m_maxDepth(maxDepth),
        m_rrThreshold(rrThreshold),
        m_lightSampleStrategy(lightSampleStrategy)
    {
      
    }

    void VolPathIntegrator::Preprocess(const Scene& scene, Sampler& sampler)
    {
        m_lightDistribution =
            CreateLightSampleDistribution(m_lightSampleStrategy, scene);

        UNUSED(sampler)
    }

    Spectrum VolPathIntegrator::Li(const RayDifferential& _ray, const Scene& _scene, Sampler& _sampler, MemoryArena& _arena, int _depth) const
    {
        UNUSED(_depth)
        
        
        Spectrum L(0.f), beta(1.f);
        RayDifferential ray(_ray);
        bool specularBounce = false;
        int bounces;
        // Added after book publication: etaScale tracks the accumulated effect
        // of radiance scaling due to rays passing through refractive
        // boundaries (see the derivation on p. 527 of the third edition). We
        // track this value in order to remove it from beta when we apply
        // Russian roulette; this is worthwhile, since it lets us sometimes
        // avoid terminating refracted rays that are about to be refracted back
        // out of a medium and thus have their beta value increased.
        float etaScale = 1;

        for (bounces = 0;; ++bounces) {
            // Intersect _ray_ with scene and store intersection in _isect_
            SurfaceInteraction isect;
            bool foundIntersection = _scene.intersect(ray, &isect);

            // Sample the participating medium, if present
            MediumInteraction mi;
            if (ray.m_medium) beta *= ray.m_medium->Sample(ray, _sampler, _arena, &mi);
            if (beta.IsBlack()) break;

            // Handle an interaction with a medium or a surface
            if (mi.IsValid()) {
                // Terminate path if ray escaped or _maxDepth_ was reached
                if (bounces >= m_maxDepth) break;
               
                // Handle scattering at point in medium for volumetric path tracer
                const Distribution1D* lightDistrib = m_lightDistribution->Lookup(mi.m_p);
                L += beta * UniformSampleOneLight(mi, _scene, _arena, _sampler, true,
                    lightDistrib);

                Vector3f wo = -ray.m_dir, wi;
                mi.phase->Sample_p(wo, &wi, _sampler.Get2D());
                ray = mi.SpawnRay(wi);
                specularBounce = false;
            }
            else {
               // ++surfaceInteractions;
                // Handle scattering at point on surface for volumetric path tracer

                // Possibly add emitted light at intersection
                if (bounces == 0 || specularBounce) {
                    // Add emitted light at path vertex or from the environment
                    if (foundIntersection)
                        L += beta * isect.Le(-ray.m_dir);
                    else
                        for (const auto& light : _scene.m_infiniteLights)
                            L += beta * light->Le(ray);
                }

                // Terminate path if ray escaped or _maxDepth_ was reached
                if (!foundIntersection || bounces >= m_maxDepth) break;

                // Compute scattering functions and skip over medium boundaries
                isect.ComputeScatteringFunctions(ray, _arena, true);
                if (!isect.m_bsdf) {
                    ray = isect.SpawnRay(ray.m_dir);
                    bounces--;
                    continue;
                }

                // Sample illumination from lights to find attenuated path
                // contribution
                const Distribution1D* lightDistrib =
                    m_lightDistribution->Lookup(isect.m_p);
                L += beta * UniformSampleOneLight(isect, _scene, _arena, _sampler,
                    true, lightDistrib);

                // Sample BSDF to get new path direction
                Vector3f wo = -ray.m_dir, wi;
                float pdf;
                eBxDFType flags;
                Spectrum f = isect.m_bsdf->sample_f(wo, &wi, _sampler.Get2D(), &pdf, BSDF_ALL, &flags);
                if (f.IsBlack() || pdf == 0.f) break;
                beta *= f * AbsDot(wi, isect.shading.m_n) / pdf;
                assert(std::isinf(beta.y()) == false);
                specularBounce = (flags & BSDF_SPECULAR) != 0;
                if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
                    float eta = isect.m_bsdf->m_eta;
                    // Update the term that tracks radiance scaling for refraction
                    // depending on whether the ray is entering or leaving the
                    // medium.
                    etaScale *= (Dot(wo, isect.m_n) > 0) ? (eta * eta) : 1 / (eta * eta);
                }
                ray = isect.SpawnRay(wi);

                // Account for attenuated subsurface scattering, if applicable
                if (isect.m_bssrdf && (flags & BSDF_TRANSMISSION)) {
                    // Importance sample the BSSRDF
                    SurfaceInteraction pi;
                    Spectrum S = isect.m_bssrdf->Sample_S(
                        _scene, _sampler.Get1D(), _sampler.Get2D(), _arena, &pi, &pdf);
                    assert(std::isinf(beta.y()) == false);
                    if (S.IsBlack() || pdf == 0) break;
                    beta *= S / pdf;

                    // Account for the attenuated direct subsurface scattering
                    // component
                    L += beta *
                        UniformSampleOneLight(pi, _scene, _arena, _sampler, true,
                            m_lightDistribution->Lookup(pi.m_p));

                    // Account for the indirect subsurface scattering component
                    Spectrum f2 = pi.m_bsdf->sample_f(pi.m_wo, &wi, _sampler.Get2D(),
                        &pdf, BSDF_ALL, &flags);
                    if (f2.IsBlack() || pdf == 0) break;
                    beta *= f2 * AbsDot(wi, pi.shading.m_n) / pdf;
                    assert(std::isinf(beta.y()) == false);
                    specularBounce = (flags & BSDF_SPECULAR) != 0;
                    ray = pi.SpawnRay(wi);
                }
            }

            // Possibly terminate the path with Russian roulette
            // Factor out radiance scaling due to refraction in rrBeta.
            Spectrum rrBeta = beta * etaScale;
            if (rrBeta.MaxComponentValue() < m_rrThreshold && bounces > 3) {
                float q = std::max((float).05, 1 - rrBeta.MaxComponentValue());
                if (_sampler.Get1D() < q) break;
                beta /= 1 - q;
                assert(std::isinf(beta.y()) == false);
            }
        }
        //ReportValue(pathLength, bounces);
        return L;
    }

    VolPathIntegrator* CreateVolPathIntegrator(const ParamSet& params, std::shared_ptr<Sampler> sampler, std::shared_ptr<Camera> camera)
    {
        int maxDepth = params.FindOneInt("maxdepth", 5);
        int np;
        const int* pb = params.FindInt("pixelbounds", &np);
        BBox2i pixelBounds = camera->m_film->GetSampleBounds();
        if (pb) {
            if (np != 4)
                Error("Expected four values for \"pixelbounds\" parameter. Got %d.",
                    np);
            else {
                pixelBounds = Intersection(pixelBounds,
                    BBox2i{ {pb[0], pb[2]}, {pb[1], pb[3]} });
                if (pixelBounds.area() == 0)
                    Error("Degenerate \"pixelbounds\" specified.");
            }
        }
        float rrThreshold = params.FindOneFloat("rrthreshold", 1.);
        std::string lightStrategy =
            params.FindOneString("lightsamplestrategy", "spatial");
        return new VolPathIntegrator(maxDepth, camera, sampler, pixelBounds, rrThreshold, lightStrategy);
    }

}

