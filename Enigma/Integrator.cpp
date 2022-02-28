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
#include "Integrator.h"





namespace RayTrace
{
    
    std::unique_ptr<Distribution1D> ComputeLightPowerDistribution(const Scene& scene)
    {
        if (!scene.getNumLights()) 
            return nullptr;
        FloatVector lightPower;
        for (const auto& light : scene.m_lights )
            lightPower.push_back(light->Power().y());
        return std::unique_ptr<Distribution1D>( new Distribution1D( &lightPower[0], lightPower.size() ) );
    }

   

    Spectrum UniformSampleAllLights(const Interaction& it, const Scene& scene, MemoryArena& arena, Sampler& sampler, const std::vector<int>& nLightSamples, bool handleMedia /*= false*/)
    {
        Spectrum L(0.f);
        for (int i = 0; i < scene.getNumLights(); ++i) {
            // Accumulate contribution of _j_th light to _L_
            const std::shared_ptr<Light>& light = scene.getLight(i);
            int nSamples = nLightSamples[i];
            const Vector2f* uLightArray = sampler.Get2DArray(nSamples);
            const Vector2f* uScatteringArray = sampler.Get2DArray(nSamples);
            if (!uLightArray || !uScatteringArray) {
                // Use a single sample for illumination from _light_
                Vector2f uLight = sampler.Get2D();
                Vector2f uScattering = sampler.Get2D();
                L += EstimateDirect(it, uScattering, *light, uLight, scene, sampler,
                    arena, handleMedia);
            }
            else {
                // Estimate direct lighting using sample arrays
                Spectrum Ld(0.f);
                for (int k = 0; k < nSamples; ++k)
                    Ld += EstimateDirect(it, uScatteringArray[k], *light,
                        uLightArray[k], scene, sampler, arena,
                        handleMedia);
                L += Ld / nSamples;
            }
        }
        return L;
    }

    Spectrum UniformSampleOneLight(const Interaction& it, const Scene& scene, MemoryArena& arena, Sampler& sampler, bool handleMedia /*= false*/, const Distribution1D* lightDistrib /*= nullptr*/)
    {
        // Randomly choose a single light to sample, _light_
        int nLights = scene.getNumLights();
        if (nLights == 0) return Spectrum(0.f);

        int lightNum;
        float lightPdf;
        if (lightDistrib) {
            lightNum = lightDistrib->SampleDiscrete(sampler.Get1D(), &lightPdf);
            if (lightPdf == 0) return Spectrum(0.f);
        }
        else {
            lightNum = std::min((int)(sampler.Get1D() * nLights), nLights - 1);
            lightPdf = 1.0f / nLights;
        }
        const std::shared_ptr<Light>& light = scene.getLight(lightNum);
        Vector2f uLight = sampler.Get2D();
        Vector2f uScattering = sampler.Get2D();
        return EstimateDirect(it, uScattering, *light, uLight,
            scene, sampler, arena, handleMedia) / lightPdf;
    }

    Spectrum EstimateDirect(const Interaction& it, const Vector2f& uScattering, const Light& light,
        const Vector2f& uLight, const Scene& scene, Sampler& sampler, MemoryArena& arena, bool handleMedia /*= false*/, bool specular /*= false*/)
    {
        eBxDFType bsdfFlags = specular ? BSDF_ALL : eBxDFType(BSDF_ALL & ~BSDF_SPECULAR);
        Spectrum Ld(0.f);
        // Sample light source with multiple importance sampling
        Vector3f wi;
        float lightPdf = 0,
            scatteringPdf = 0;
        VisibilityTester visibility;
        Spectrum Li = light.Sample_Li(it, uLight, &wi, &lightPdf, &visibility);

        if (lightPdf > 0 && !Li.IsBlack()) {
            // Compute BSDF or phase function's value for light sample
            Spectrum f;
            if (it.IsSurfaceInteraction()) {
                // Evaluate BSDF for light sampling strategy
                const SurfaceInteraction& isect = (const SurfaceInteraction&)it;
                f = isect.m_bsdf->f(isect.m_wo, wi, bsdfFlags) * AbsDot(wi, isect.shading.m_n);
                scatteringPdf = isect.m_bsdf->Pdf(isect.m_wo, wi, bsdfFlags);
                // VLOG(2) << "  surf f*dot :" << f << ", scatteringPdf: " << scatteringPdf;
            }
            else {
                // Evaluate phase function for light sampling strategy
                const MediumInteraction& mi = (const MediumInteraction&)it;
                float p = mi.phase->p(mi.m_wo, wi);
                f = Spectrum(p);
                scatteringPdf = p;
            }
            if (!f.IsBlack()) {
                // Compute effect of visibility for light source sample
                if (handleMedia) {
                    Li *= visibility.Tr(scene, sampler);
                }
                else {
                    if (!visibility.Unoccluded(scene)) {
                        Li = Spectrum(0.f);
                    }
                    else {

                    }

                }

                // Add light's contribution to reflected radiance
                if (!Li.IsBlack()) {
                    if (IsDeltaLight(light.m_flags))
                        Ld += f * Li / lightPdf;
                    else {
                        float weight = PowerHeuristic(1, lightPdf, 1, scatteringPdf);
                        Ld += f * Li * weight / lightPdf;
                    }
                }
            }
        }

        // Sample BSDF with multiple importance sampling
        if (!IsDeltaLight(light.m_flags)) {
            Spectrum f;
            bool sampledSpecular = false;
            if (it.IsSurfaceInteraction()) {
                // Sample scattered direction for surface interactions
                eBxDFType sampledType;
                const SurfaceInteraction& isect = (const SurfaceInteraction&)it;
                f = isect.m_bsdf->sample_f(isect.m_wo, &wi, uScattering, &scatteringPdf, bsdfFlags, &sampledType);
                f *= AbsDot(wi, isect.shading.m_n);
                sampledSpecular = (sampledType & BSDF_SPECULAR) != 0;
            }
            else {
                // Sample scattered direction for medium interactions
                const MediumInteraction& mi = (const MediumInteraction&)it;
                float p = mi.phase->Sample_p(mi.m_wo, &wi, uScattering);
                f = Spectrum(p);
                scatteringPdf = p;
            }
            // VLOG(2) << "  BSDF / phase sampling f: " << f << ", scatteringPdf: " <<
            //     scatteringPdf;
            if (!f.IsBlack() && scatteringPdf > 0) {
                // Account for light contributions along sampled direction _wi_
                float weight = 1;
                if (!sampledSpecular) {
                    lightPdf = light.Pdf_Li(it, wi);
                    if (lightPdf == 0) return Ld;
                    weight = PowerHeuristic(1, scatteringPdf, 1, lightPdf);
                }

                // Find intersection and compute transmittance
                SurfaceInteraction lightIsect;
                Ray ray = it.SpawnRay(wi);
                Spectrum Tr(1.f);
                bool foundSurfaceInteraction =
                    handleMedia ? scene.intersectTr(ray, sampler, &lightIsect, &Tr)
                    : scene.intersect(ray, &lightIsect);

                // Add light contribution from material sampling
                Spectrum Li(0.f);
                if (foundSurfaceInteraction) {
                    if (lightIsect.m_primitive->getAreaLight() == &light)
                        Li = lightIsect.Le(-wi);
                }
                else
                    Li = light.Le(ray);
                if (!Li.IsBlack())
                    Ld += f * Li * Tr * weight / scatteringPdf;
            }
        }
        if (Ld.HasNaNs()) {
            std::cout << "Estimate Direct nans\n";
        }
        return Ld;
    }

   

    //////////////////////////////////////////////////////////////////////////
    // SamplerIntegrator Implementation
    //////////////////////////////////////////////////////////////////////////
    SamplerIntegrator::SamplerIntegrator(const CameraPtr& camera, std::shared_ptr<Sampler> sampler, const BBox2i& pixelBounds) 
        : m_camera(camera)
        , m_sampler(sampler)
        , m_pixelBounds(pixelBounds)
    {

    }

    void SamplerIntegrator::Preprocess(const Scene& scene, Sampler& sampler)
    {
        UNUSED(scene) 
        UNUSED(sampler)
    }

    void SamplerIntegrator::Render(const Scene& scene)
    {
       
        
        Preprocess(scene, *m_sampler);
        // Render image tiles in parallel

        // Compute number of tiles, _nTiles_, to use for parallel rendering
        BBox2i   sampleBounds = m_camera->m_film->GetSampleBounds();
        Vector2i sampleExtent = sampleBounds.diagonal();
        
        Vector2i nTiles((sampleExtent.x + TileSize - 1) / TileSize,
                        (sampleExtent.y + TileSize - 1) / TileSize);

        struct TileTask
        {
            void operator()()
            {
                MemoryArena arena;
                auto sampleBounds = m_camera->m_film->GetSampleBounds();
                // Get sampler instance for tile
                int seed = m_curTile.y * m_nTiles.x + m_curTile.x;
                
                std::unique_ptr<Sampler> tileSampler = m_integrator->GetSampler()->Clone(seed);

                // Compute sample bounds for tile
                int x0 = sampleBounds.m_min.x + m_curTile.x * TileSize;
                int x1 = std::min(x0 + TileSize, sampleBounds.m_max.x);

                int y0 = sampleBounds.m_min.x + m_curTile.y * TileSize;
                int y1 = std::min(y0 + TileSize, sampleBounds.m_max.y);
                BBox2i tileBounds(Vector2i(x0, y0), Vector2i(x1, y1));
                // LOG(INFO) << "Starting image tile " << tileBounds;
                std::unique_ptr<FilmTile> filmTile = m_camera->m_film->GetFilmTile(tileBounds);

                for (int pixY = y0; pixY < y1; pixY++)
                    for (int pixX = x0; pixX < x1; pixX++)
                    {
                        Vector2i pixel(pixX, pixY);
                        if (!m_pixelBounds.insideExclusive(pixel))
                            continue;
                        tileSampler->StartPixel(pixel);

                      
                        do {
                           
                            // Initialize _CameraSample_ for current sample
                            CameraSample cameraSample = tileSampler->GetCameraSample(pixel);
                            // Generate camera ray for current sample
                            RayDifferential ray;
                            float rayWeight = m_camera->GenerateRayDifferential(cameraSample, &ray);
                            ray.scaleDifferentials(1 / std::sqrt((float)tileSampler->m_samplesPerPixel));

                            // Evaluate radiance along camera ray
                            Spectrum L(0.f);
                            if (rayWeight > 0)
                                L = m_integrator->Li(ray, *m_scene, *tileSampler, arena);

                            // Issue warning if unexpected radiance value returned                           
                            if (L.HasNaNs()) {
                                L = Spectrum(0.f);
                            }
                            else if (L.y() < -1e-5) {
                                L = Spectrum(0.f);
                            }
                            else if (std::isinf(L.y())) {
                                L = Spectrum(0.f);                               
                            }
                        

                            filmTile->AddSample(cameraSample.m_image, L, rayWeight);
                            arena.Reset();
                        } while (tileSampler->StartNextSample());
                    }
                m_camera->m_film->MergeFilmTile(std::move(filmTile));
            }
            
            BBox2i              m_pixelBounds;
            Vector2i            m_nTiles;
            Vector2i            m_curTile;
            SamplerIntegrator*  m_integrator;
            const Camera*             m_camera;
            const Scene*              m_scene;
        };

        TaskQueue<TileTask> renderQueue(NumSystemCores());
        for (int y = 0; y < nTiles.y; ++y) 
        {
            for (int x = 0; x < nTiles.x; ++x)
            {
                TileTask t = { m_pixelBounds, nTiles, {x, y}, this, m_camera.get(), &scene };
                renderQueue.Enqueue(t);                
            }//for x tiles
        }//for y tiles
        // Save final image after rendering
        renderQueue.Join();
        m_camera->m_film->WriteImage( 1.0f / m_sampler->m_samplesPerPixel);
    }

    Spectrum SamplerIntegrator::Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth /*= 0*/) const
    {
        return 0.f;
    }

    Spectrum SamplerIntegrator::SpecularReflect(const RayDifferential& ray, const SurfaceInteraction& isect, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const
    {
        Vector3f wo = isect.m_wo, 
                 wi;
        float pdf;
        eBxDFType type = eBxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
        Spectrum f = isect.m_bsdf->sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

        // Return contribution of specular reflection
        const Vector3f& ns = isect.shading.m_n;
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
            // Compute ray differential _rd_ for specular reflection
            RayDifferential rd = isect.SpawnRay(wi);
            if (ray.m_hasDifferentials) {
                rd.m_hasDifferentials = true;
                rd.m_rxOrigin = isect.m_p + isect.m_dpdx;
                rd.m_ryOrigin = isect.m_p + isect.m_dpdy;
                // Compute differential reflected directions
                Vector3f dndx = isect.shading.m_dndu * isect.m_dudx + isect.shading.m_dndv * isect.m_dvdx;
                Vector3f dndy = isect.shading.m_dndu * isect.m_dudy +
                    isect.shading.m_dndv * isect.m_dvdy;
                Vector3f dwodx = -ray.m_rxDirection - wo,
                    dwody = -ray.m_ryDirection - wo;
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
                rd.m_rxDirection =
                    wi - dwodx + 2.f * Vector3f(Dot(wo, ns) * dndx + dDNdx * ns);
                rd.m_ryDirection =
                    wi - dwody + 2.f * Vector3f(Dot(wo, ns) * dndy + dDNdy * ns);
            }
            return f * Li(rd, scene, sampler, arena, depth + 1) * AbsDot(wi, ns) /
                pdf;
        }
        else
            return Spectrum(0.f);
    }

    Spectrum SamplerIntegrator::SpecularTransmit(const RayDifferential& ray,
        const SurfaceInteraction& isect, const Scene& scene,
        Sampler& sampler, MemoryArena& arena, int depth) const
    {
        Vector3f wo = isect.m_wo, wi;
        float pdf;
        const Vector3f& p = isect.m_p;
        const BSDF& bsdf = *isect.m_bsdf;
        Spectrum f = bsdf.sample_f(wo, &wi, sampler.Get2D(), &pdf, eBxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
        Spectrum L = Spectrum(0.f);
        Vector3f ns = isect.shading.m_n;
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
            // Compute ray differential _rd_ for specular transmission
            RayDifferential rd = isect.SpawnRay(wi);
            if (ray.m_hasDifferentials) {
                rd.m_hasDifferentials = true;
                rd.m_rxOrigin = p + isect.m_dpdx;
                rd.m_ryOrigin = p + isect.m_dpdy;

                Vector3f dndx = isect.shading.m_dndu * isect.m_dudx +
                                isect.shading.m_dndv * isect.m_dvdx;
                Vector3f dndy = isect.shading.m_dndu * isect.m_dudy +
                                isect.shading.m_dndv * isect.m_dvdy;

                // The BSDF stores the IOR of the interior of the object being
                // intersected.  Compute the relative IOR by first out by
                // assuming that the ray is entering the object.
                float eta = 1 / bsdf.m_eta;
                if (Dot(wo, ns) < 0) {
                    // If the ray isn't entering, then we need to invert the
                    // relative IOR and negate the normal and its derivatives.
                    eta = 1 / eta;
                    ns = -ns;
                    dndx = -dndx;
                    dndy = -dndy;
                }

                /*
                  Notes on the derivation:
                  - pbrt computes the refracted ray as: \wi = -\eta \omega_o + [ \eta (\wo \cdot \N) - \cos \theta_t ] \N
                    It flips the normal to lie in the same hemisphere as \wo, and then \eta is the relative IOR from
                    \wo's medium to \wi's medium.
                  - If we denote the term in brackets by \mu, then we have: \wi = -\eta \omega_o + \mu \N
                  - Now let's take the partial derivative. (We'll use "d" for \partial in the following for brevity.)
                    We get: -\eta d\omega_o / dx + \mu dN/dx + d\mu/dx N.
                  - We have the values of all of these except for d\mu/dx (using bits from the derivation of specularly
                    reflected ray deifferentials).
                  - The first term of d\mu/dx is easy: \eta d(\wo \cdot N)/dx. We already have d(\wo \cdot N)/dx.
                  - The second term takes a little more work. We have:
                     \cos \theta_i = \sqrt{1 - \eta^2 (1 - (\wo \cdot N)^2)}.
                     Starting from (\wo \cdot N)^2 and reading outward, we have \cos^2 \theta_o, then \sin^2 \theta_o,
                     then \sin^2 \theta_i (via Snell's law), then \cos^2 \theta_i and then \cos \theta_i.
                  - Let's take the partial derivative of the sqrt expression. We get:
                    1 / 2 * 1 / \cos \theta_i * d/dx (1 - \eta^2 (1 - (\wo \cdot N)^2)).
                  - That partial derivatve is equal to:
                    d/dx \eta^2 (\wo \cdot N)^2 = 2 \eta^2 (\wo \cdot N) d/dx (\wo \cdot N).
                  - Plugging it in, we have d\mu/dx =
                    \eta d(\wo \cdot N)/dx - (\eta^2 (\wo \cdot N) d/dx (\wo \cdot N))/(-\wi \cdot N).
                 */
                Vector3f dwodx = -ray.m_rxDirection - wo,
                    dwody = -ray.m_ryDirection - wo;
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);

                float mu = eta * Dot(wo, ns) - AbsDot(wi, ns);
                float dmudx =
                    (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdx;
                float dmudy =
                    (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdy;

                rd.m_rxDirection =
                    wi - eta * dwodx + Vector3f(mu * dndx + dmudx * ns);
                rd.m_ryDirection =
                    wi - eta * dwody + Vector3f(mu * dndy + dmudy * ns);
            }
            L = f * Li(rd, scene, sampler, arena, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        return L;
    }

   
  
    const BBox2i& SamplerIntegrator::GetPixelBounds() const
    {
        return m_pixelBounds;
    }

    const SamplerPtr& SamplerIntegrator::GetSampler() const
    {
        return m_sampler;
    }


    const CameraPtr& SamplerIntegrator::GetCamera() const
    {
        return m_camera;
    }

}

