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

#include "PathIntegrator.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
    //PathIntegrator
    //////////////////////////////////////////////////////////////////////////
    PathIntegrator::PathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler, const BBox2i& pixelBounds,
        float rrThreshold /*= 1*/, const std::string& lightSampleStrategy /*= "spatial"*/)
        : SamplerIntegrator(camera, sampler, pixelBounds)
        , m_maxDepth(maxDepth)
        , m_rrThreshold(rrThreshold)
        , m_lightSampleStrategy(lightSampleStrategy)
    {

    }

    void PathIntegrator::Preprocess(const Scene& scene, Sampler& sampler)
    {
        m_lightDistribution = CreateLightSampleDistribution(m_lightSampleStrategy, scene);
    }

    Spectrum PathIntegrator::Li(const RayDifferential& _ray, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const
    {
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
            // Find next path vertex and accumulate contribution
        //    VLOG(2) << "Path tracer bounce " << bounces << ", current L = " << L
          //      << ", beta = " << beta;

           
            // Intersect _ray_ with scene and store intersection in _isect_
            SurfaceInteraction isect;
            bool foundIntersection = scene.intersect(ray, &isect);

            // Possibly add emitted light at intersection
            if (bounces == 0 || specularBounce) {
                // Add emitted light at path vertex or from the environment
                if (foundIntersection) {
                    L += beta * isect.Le(-ray.m_dir);
                    //  VLOG(2) << "Added Le -> L = " << L;
                }
                else {
                    for (const auto& light : scene.m_infiniteLights)
                        L += beta * light->Le(ray);
                    // VLOG(2) << "Added infinite area lights -> L = " << L;
                }               
            }

            // Terminate path if ray escaped or _maxDepth_ was reached00
            if (!foundIntersection || bounces >= m_maxDepth) break;

            // Compute scattering functions and skip over medium boundaries
            isect.ComputeScatteringFunctions(ray, arena, true);
            if (!isect.m_bsdf) {
                //  VLOG(2) << "Skipping intersection due to null bsdf";
                ray = isect.SpawnRay(ray.m_dir);
                bounces--;
                continue;
            }

            const Distribution1D* distrib = m_lightDistribution->Lookup(isect.m_p);

            // Sample illumination from lights to find path contribution.
            // (But skip this for perfectly specular BSDFs.)
            if (isect.m_bsdf->numComponents(eBxDFType(BSDF_ALL & ~BSDF_SPECULAR)) >
                0) {
                //++totalPaths;
                Spectrum Ld = beta * UniformSampleOneLight(isect, scene, arena,
                    sampler, false, distrib);
                //   VLOG(2) << "Sampled direct lighting Ld = " << Ld;
                if (Ld.IsBlack()) {
                    //++zeroRadiancePaths;
                }               
                //  CHECK_GE(Ld.y(), 0.f);
                L += Ld;
            }

            // Sample BSDF to get new path direction
            Vector3f wo = -ray.m_dir, wi;
            float pdf;
            eBxDFType flags;
            Spectrum f = isect.m_bsdf->sample_f(wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);
            // VLOG(2) << "Sampled BSDF, f = " << f << ", pdf = " << pdf;
            if (f.IsBlack() || pdf == 0.f)
                break;
            beta *= f * AbsDot(wi, isect.shading.m_n) / pdf;
            
            //VLOG(2) << "Updated beta = " << beta;
           // CHECK_GE(beta.y(), 0.f);
            //DCHECK(!std::isinf(beta.y()));
            specularBounce = (flags & BSDF_SPECULAR) != 0;
            if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
                float eta = isect.m_bsdf->m_eta;
                // Update the term that tracks radiance scaling for refraction
                // depending on whether the ray is entering or leaving the
                // medium.
                etaScale *= (Dot(wo, isect.m_n) > 0) ? (eta * eta) : 1 / (eta * eta);
            }
            ray = isect.SpawnRay(wi);

            // Account for subsurface scattering, if applicable
            if (isect.m_bssrdf && (flags & BSDF_TRANSMISSION)) {
                // Importance sample the BSSRDF
                SurfaceInteraction pi;
                Spectrum S = isect.m_bssrdf->Sample_S(
                    scene, sampler.Get1D(), sampler.Get2D(), arena, &pi, &pdf);
                // DCHECK(!std::isinf(beta.y()));
                if (S.IsBlack() || pdf == 0) break;
                beta *= S / pdf;
               
                // Account for the direct subsurface scattering component
                L += beta * UniformSampleOneLight(pi, scene, arena, sampler, false, m_lightDistribution->Lookup(pi.m_p));

                // Account for the indirect subsurface scattering component
                Spectrum f = pi.m_bsdf->sample_f(pi.m_wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);
                if (f.IsBlack() || pdf == 0) break;
                beta *= f * AbsDot(wi, pi.shading.m_n) / pdf;
                //DCHECK(!std::isinf(beta.y()));
                specularBounce = (flags & BSDF_SPECULAR) != 0;
                ray = pi.SpawnRay(wi);
            }

           /* if (beta.HasInfs()) {
                float rgb[3] = { 999.0f, 0.0f, 0.0f };
                return Spectrum::FromRGB(rgb, eSpectrumType::SPECTRUM_REFLECTANCE);
            }*/


            // Possibly terminate the path with Russian roulette.
            // Factor out radiance scaling due to refraction in rrBeta.
            Spectrum rrBeta = beta * etaScale;
            if (rrBeta.MaxComponentValue() < m_rrThreshold && bounces > 3)
            {
                float q = std::max((float).05f, 1.0f - rrBeta.MaxComponentValue());
                if (sampler.Get1D() < q)
                    break;
                beta /= 1 - q;
                // DCHECK(!std::isinf(beta.y()));
            }
        }
        // ReportValue(pathLength, bounces);
        return L;
    }

 
    Integrator* CreatePathIntegrator(const ParamSet& _param, const CameraPtr& _cam, const SamplerPtr& _sampler)
    {
        int maxDepth = _param.FindOneInt("maxdepth", 5);
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
        float rrThreshold = _param.FindOneFloat("rrthreshold", 1.);
        std::string lightStrategy = _param.FindOneString("lightsamplestrategy", "spatial");
        return new PathIntegrator(maxDepth, _cam, _sampler, pixelBounds, rrThreshold, lightStrategy);
    }
}

