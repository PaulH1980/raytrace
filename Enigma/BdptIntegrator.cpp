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
#include "Filter.h"
#include "Concurrency.h"
#include "StringPrint.h"
#include "Lights.h"
#include "BdptIntegrator.h"

namespace RayTrace
{
    // BDPT Method Definitions
    inline int BufferIndex(int s, int t) {
        int above = s + t - 2;
        return s + above * (5 + above) / 2;
    }
    
    
    
    struct TileTask
    {

        Vector2i                m_nTiles;
        Vector2i                m_curTile;

        BDPTIntegrator*         m_integrator = nullptr;
        LightDistribution*      m_dist = nullptr;

        const LightToIndexMap*  m_lightToIndex = nullptr;       
        const Scene*            m_scene = nullptr;      
        std::vector<std::unique_ptr<Film>>* m_weightFilms;
        
        
        void operator()()
        {

            MemoryArena arena;
            auto& camera = m_integrator->GetCamera();
            auto* film   = camera->m_film.get();
            int maxDepth = m_integrator->MaxDepth();

            
            // Get sampler instance for tile
            int seed = m_curTile.y * m_nTiles.x + m_curTile.x;

            std::unique_ptr<Sampler> tileSampler = m_integrator->GetSampler()->Clone(seed);

            // Compute sample bounds for tile
            auto sampleBounds = m_integrator->GetCamera()->m_film->GetSampleBounds();
            int x0 = sampleBounds.m_min.x + m_curTile.x * TileSize;
            int x1 = std::min(x0 + TileSize, sampleBounds.m_max.x);

            int y0 = sampleBounds.m_min.x + m_curTile.y * TileSize;
            int y1 = std::min(y0 + TileSize, sampleBounds.m_max.y);
            BBox2i tileBounds(Vector2i(x0, y0), Vector2i(x1, y1));
            // LOG(INFO) << "Starting image tile " << tileBounds;
            std::unique_ptr<FilmTile> filmTile = m_integrator->GetCamera()->m_film->GetFilmTile(tileBounds);

            for (int pixY = y0; pixY < y1; ++pixY)
            {
                for (int pixX = x0; pixX < x1; ++pixX)
                {
                    Point2i pixel(pixX, pixY);
                   

                    if (!m_integrator->GetPixelBounds().insideExclusive(pixel))
                        continue;
                    tileSampler->StartPixel(pixel);
                    do
                    {
                        // Generate a single sample using BDPT
                        Point2f pFilm = (Point2f)pixel + tileSampler->Get2D();

                        // Trace the camera subpath
                        Vertex* cameraVertices = arena.Alloc<Vertex>(maxDepth + 2);
                        Vertex* lightVertices = arena.Alloc<Vertex>(maxDepth + 1);
                        int nCamera = GenerateCameraSubpath(*m_scene, *tileSampler, arena, maxDepth + 2, *camera, pFilm, cameraVertices);
                        // Get a distribution for sampling the light at the
                        // start of the light subpath. Because the light path
                        // follows multiple bounces, basing the sampling
                        // distribution on any of the vertices of the camera
                        // path is unlikely to be a good strategy. We use the
                        // PowerLightDistribution by default here, which
                        // doesn't use the point passed to it.
                        const Distribution1D* lightDistr = m_dist->Lookup(cameraVertices[0].p());
                        // Now trace the light subpath
                        int nLight = GenerateLightSubpath(*m_scene, *tileSampler, arena, maxDepth + 1,
                            cameraVertices[0].time(), *lightDistr, *m_lightToIndex,
                            lightVertices);

                        // Execute all BDPT connection strategies
                        Spectrum L(0.f);
                        for (int t = 1; t <= nCamera; ++t)
                        {
                            for (int s = 0; s <= nLight; ++s) {
                                int depth = t + s - 2;
                                if ((s == 1 && t == 1) || depth < 0 ||
                                    depth > maxDepth)
                                    continue;
                                // Execute the $(s, t)$ connection strategy and
                                // update _L_
                                Point2f pFilmNew = pFilm;
                                float misWeight = 0.f;
                                Spectrum Lpath = ConnectBDPT(
                                    *m_scene, lightVertices, cameraVertices, s, t,
                                    *lightDistr, *m_lightToIndex, *camera, *tileSampler,
                                    &pFilmNew, &misWeight);
                                //  VLOG(2) << "Connect bdpt s: " << s << ", t: " << t <<
                                //      ", Lpath: " << Lpath << ", misWeight: " << misWeight;
                               
                                if (m_integrator->VisualizeDebug()) {
                                    Spectrum value;
                                    if (m_integrator->VisStrategies())
                                        value =
                                        misWeight == 0 ? 0 : Lpath / misWeight;
                                    if (m_integrator->VisWeights()) value = Lpath;
                                    (*m_weightFilms)[BufferIndex(s, t)]->AddSplat(pFilmNew, value);
                                }
                                if (t != 1)
                                    L += Lpath;
                                else
                                    m_integrator->GetCamera()->m_film->AddSplat(pFilmNew, Lpath);
                            }//for nlights
                        } //for ncamera           
                        filmTile->AddSample(pFilm, L);
                        arena.Reset();
                       
                    } while (tileSampler->StartNextSample());                
                }//for pixX
            }//for pixY   
            film->MergeFilmTile(std::move(filmTile));
        } //operator ()()
    };
    



    float InfiniteLightDensity(const Scene& scene, const Distribution1D& lightDistr, 
        const std::unordered_map<const Light*, size_t>& lightToDistrIndex, const Vector3f& w)
    {
        float pdf = 0;
        for (const auto& light : scene.m_infiniteLights) {
            assert(lightToDistrIndex.find(light.get()) != lightToDistrIndex.end());
            size_t index = lightToDistrIndex.find(light.get())->second;
            pdf += light->Pdf_Li(Interaction(), -w) * lightDistr.func[index];
        }
        return pdf / (lightDistr.funcInt * lightDistr.count);
    }

    bool Vertex::IsDeltaLight() const
    {
        return type == VertexType::Light && ei.light &&
            RayTrace::IsDeltaLight(ei.light->m_flags);
    }

    Vertex::Vertex() 
        : ei()
    {

    }

    Vertex::Vertex(VertexType type, const EndpointInteraction& ei, const Spectrum& beta) : type(type), beta(beta), ei(ei)
    {

    }

    Vertex::Vertex(const SurfaceInteraction& si, const Spectrum& beta) : type(VertexType::Surface), beta(beta), si(si)
    {

    }

    Vertex::Vertex(const Vertex& v)
    {
        memcpy(this, &v, sizeof(Vertex));
    }

    Vertex& Vertex::operator=(const Vertex& v)
    {
        memcpy(this, &v, sizeof(Vertex));
        return *this;
    }

    Vertex::Vertex(const MediumInteraction& mi, const Spectrum& beta) : type(VertexType::Medium), beta(beta), mi(mi)
    {

    }

    const Interaction& Vertex::GetInteraction() const
    {
        switch (type) {
        case VertexType::Medium:
            return mi;
        case VertexType::Surface:
            return si;
        default:
            return ei;
        }
    }

    const Point3f& Vertex::p() const
    {
        return GetInteraction().m_p;
    }

    float Vertex::time() const
    {
        return GetInteraction().m_time;
    }

    const Normal3f& Vertex::ng() const
    {
        return GetInteraction().m_n;
    }

    const Normal3f& Vertex::ns() const
    {
        if (type == VertexType::Surface)
            return si.shading.m_n;
        else
            return GetInteraction().m_n;
    }

    bool Vertex::IsOnSurface() const
    {
        return ng() != Normal3f();
    }

    Spectrum Vertex::f(const Vertex& next, eTransportMode mode) const
    {
        Vector3f wi = next.p() - p();
        if (LengthSqr(wi) == 0) return 0.;
        wi = Normalize(wi);
        switch (type) {
        case VertexType::Surface:
            return si.m_bsdf->f(si.m_wo, wi) *
                CorrectShadingNormal(si, si.m_wo, wi, mode);
        case VertexType::Medium:
            return mi.phase->p(mi.m_wo, wi);
        default:
            // LOG(FATAL) << "Vertex::f(): Unimplemented";
            assert(false);
            return Spectrum(0.f);
        }
    }

    bool Vertex::IsConnectible() const
    {
        switch (type) {
        case VertexType::Medium:
            return true;
        case VertexType::Light:
            return (ei.light->m_flags & (int)eLightFlags::LIGHTFLAG_DELTADIRECTION) == 0;
        case VertexType::Camera:
            return true;
        case VertexType::Surface:
            return si.m_bsdf->numComponents(eBxDFType(BSDF_DIFFUSE | BSDF_GLOSSY |
                BSDF_REFLECTION |
                BSDF_TRANSMISSION)) > 0;
        }
        //LOG(FATAL) << "Unhandled vertex type in IsConnectable()";
        assert(false);
        return false;  // NOTREACHED
    }

    bool Vertex::IsLight() const
    {
        return type == VertexType::Light ||
            (type == VertexType::Surface && si.m_primitive->getAreaLight());
    }

    bool Vertex::IsInfiniteLight() const
    {
        return type == VertexType::Light &&
            (!ei.light || ei.light->m_flags & (int)eLightFlags::LIGHTFLAG_INFINITE ||
                ei.light->m_flags & (int)eLightFlags::LIGHTFLAG_DELTADIRECTION);
    }

    Spectrum Vertex::Le(const Scene& scene, const Vertex& v) const
    {
        if (!IsLight()) return Spectrum(0.f);
        Vector3f w = v.p() - p();
        if (LengthSqr(w) == 0) return 0.;
        w = Normalize(w);
        if (IsInfiniteLight()) {
            // Return emitted radiance for infinite light sources
            Spectrum Le(0.f);
            for (const auto& light : scene.m_infiniteLights)
                Le += light->Le(Ray(p(), -w));
            return Le;
        }
        else {
            const AreaLight* light = si.m_primitive->getAreaLight();
            assert(light != nullptr);
            return light->L(si, w);
        }
    }

    float Vertex::ConvertDensity(float pdf, const Vertex& next) const
    {
        // Return solid angle density if _next_ is an infinite area light
        if (next.IsInfiniteLight()) return pdf;
        Vector3f w = next.p() - p();
        if (LengthSqr(w) == 0) return 0;
        float invDist2 = 1 / LengthSqr(w);
        if (next.IsOnSurface())
            pdf *= AbsDot(next.ng(), w * std::sqrt(invDist2));
        return pdf * invDist2;
    }

    float Vertex::Pdf(const Scene& scene, const Vertex* prev, const Vertex& next) const
    {
        if (type == VertexType::Light) return PdfLight(scene, next);
        // Compute directions to preceding and next vertex
        Vector3f wn = next.p() - p();
        if (LengthSqr(wn) == 0) return 0;
        wn = Normalize(wn);
        Vector3f wp;
        if (prev) {
            wp = prev->p() - p();
            if (LengthSqr(wp) == 0) return 0;
            wp = Normalize(wp);
        }
        else {
            assert(type == VertexType::Camera);
        }

        // Compute directional density depending on the vertex types
        float pdf = 0, unused;
        if (type == VertexType::Camera)
            ei.camera->Pdf_We(ei.SpawnRay(wn), &unused, &pdf);
        else if (type == VertexType::Surface)
            pdf = si.m_bsdf->Pdf(wp, wn);
        else if (type == VertexType::Medium)
            pdf = mi.phase->p(wp, wn);
        else {
            assert(false);
        }

        // Return probability per unit area at vertex _next_
        return ConvertDensity(pdf, next);
    }

    // BDPT Forward Declarations
    int RandomWalk(const Scene& scene, RayDifferential ray, Sampler& sampler,
        MemoryArena& arena, Spectrum beta, float pdf, int maxDepth,
        eTransportMode mode, Vertex* path);

    // BDPT Utility Functions
    float CorrectShadingNormal(const SurfaceInteraction& isect, const Vector3f& wo,
        const Vector3f& wi, eTransportMode mode) {
        if (mode == eTransportMode::TRANSPORTMODE_IMPORTANCE) {
            float num = AbsDot(wo, isect.shading.m_n) * AbsDot(wi, isect.m_n);
            float denom = AbsDot(wo, isect.m_n) * AbsDot(wi, isect.shading.m_n);
            // wi is occasionally perpendicular to isect.shading.n; this is
            // fine, but we don't want to return an infinite or NaN value in
            // that case.
            if (denom == 0) return 0;
            return num / denom;
        }
        else
            return 1;
    }

    int GenerateCameraSubpath(const Scene& scene, Sampler& sampler,
        MemoryArena& arena, int maxDepth,
        const Camera& camera, const Point2f& pFilm,
        Vertex* path) {
        if (maxDepth == 0) return 0;
        // Sample initial ray for camera subpath
        CameraSample cameraSample;
        cameraSample.m_image = pFilm;
        cameraSample.m_time = sampler.Get1D();
        cameraSample.m_lens = sampler.Get2D();
        RayDifferential ray;
        Spectrum beta = camera.GenerateRayDifferential(cameraSample, &ray);
        ray.scaleDifferentials(1 / std::sqrt(sampler.m_samplesPerPixel));

        // Generate first vertex on camera subpath and start random walk
        float pdfPos, pdfDir;
        path[0] = Vertex::CreateCamera(&camera, ray, beta);
        camera.Pdf_We(ray, &pdfPos, &pdfDir);
       /* VLOG(2) << "Starting camera subpath. Ray: " << ray << ", beta " << beta
            << ", pdfPos " << pdfPos << ", pdfDir " << pdfDir;
     */   return RandomWalk(scene, ray, sampler, arena, beta, pdfDir, maxDepth - 1,
            eTransportMode::TRANSPORTMODE_RADIANCE, path + 1) +
            1;
    }

    int GenerateLightSubpath(
        const Scene& scene, Sampler& sampler, MemoryArena& arena, int maxDepth,
        float time, const Distribution1D& lightDistr,
        const std::unordered_map<const Light*, size_t>& lightToIndex,
        Vertex* path) {
        if (maxDepth == 0) return 0;
         // Sample initial ray for light subpath
        float lightPdf;
        int lightNum = lightDistr.SampleDiscrete(sampler.Get1D(), &lightPdf);
        const std::shared_ptr<Light>& light = scene.m_lights[lightNum];
        RayDifferential ray;
        Normal3f nLight;
        float pdfPos, pdfDir;
        Spectrum Le = light->Sample_Le(sampler.Get2D(), sampler.Get2D(), time, &ray,
            &nLight, &pdfPos, &pdfDir);
        if (pdfPos == 0 || pdfDir == 0 || Le.IsBlack()) return 0;

        // Generate first vertex on light subpath and start random walk
        path[0] =
            Vertex::CreateLight(light.get(), ray, nLight, Le, pdfPos * lightPdf);
        Spectrum beta = Le * AbsDot(nLight, ray.m_dir) / (lightPdf * pdfPos * pdfDir);
        int nVertices =
            RandomWalk(scene, ray, sampler, arena, beta, pdfDir, maxDepth - 1,
                eTransportMode::TRANSPORTMODE_IMPORTANCE, path + 1);

        // Correct subpath sampling densities for infinite area lights
        if (path[0].IsInfiniteLight()) {
            // Set spatial density of _path[1]_ for infinite area light
            if (nVertices > 0) {
                path[1].pdfFwd = pdfPos;
                if (path[1].IsOnSurface())
                    path[1].pdfFwd *= AbsDot(ray.m_dir, path[1].ng());
            }

            // Set spatial density of _path[0]_ for infinite area light
            path[0].pdfFwd =
                InfiniteLightDensity(scene, lightDistr, lightToIndex, ray.m_dir);
        }
        return nVertices + 1;
    }

    int RandomWalk(const Scene& scene, RayDifferential ray, Sampler& sampler,
        MemoryArena& arena, Spectrum beta, float pdf, int maxDepth,
        eTransportMode mode, Vertex* path) {
        if (maxDepth == 0) return 0;
        int bounces = 0;
        // Declare variables for forward and reverse probability densities
        float pdfFwd = pdf, pdfRev = 0;
        while (true) {
            // Attempt to create the next subpath vertex in _path_
            MediumInteraction mi;

             SurfaceInteraction isect;
            bool foundIntersection = scene.intersect(ray, &isect);
            if (ray.m_medium) beta *= ray.m_medium->Sample(ray, sampler, arena, &mi);
            if (beta.IsBlack()) break;
            Vertex& vertex = path[bounces], & prev = path[bounces - 1];
            if (mi.IsValid()) {
                // Record medium interaction in _path_ and compute forward density
                vertex = Vertex::CreateMedium(mi, beta, pdfFwd, prev);
                if (++bounces >= maxDepth) break;

                // Sample direction and compute reverse density at preceding vertex
                Vector3f wi;
                pdfFwd = pdfRev = mi.phase->Sample_p(-ray.m_dir, &wi, sampler.Get2D());
                ray = mi.SpawnRay(wi);
            }
            else {
                // Handle surface interaction for path generation
                if (!foundIntersection) {
                    // Capture escaped rays when tracing from the camera
                    if (mode == eTransportMode::TRANSPORTMODE_RADIANCE) {
                        vertex = Vertex::CreateLight(EndpointInteraction(ray), beta,
                            pdfFwd);
                        ++bounces;
                    }
                    break;
                }

                // Compute scattering functions for _mode_ and skip over medium
                // boundaries
                isect.ComputeScatteringFunctions(ray, arena, true, mode);
                if (!isect.m_bsdf) {
                    ray = isect.SpawnRay(ray.m_dir);
                    continue;
                }

                // Initialize _vertex_ with surface intersection information
                vertex = Vertex::CreateSurface(isect, beta, pdfFwd, prev);
                if (++bounces >= maxDepth) break;

                // Sample BSDF at current vertex and compute reverse probability
                Vector3f wi, wo = isect.m_wo;
                eBxDFType type;
                Spectrum f = isect.m_bsdf->sample_f(wo, &wi, sampler.Get2D(), &pdfFwd, BSDF_ALL, &type);
                if (f.IsBlack() || pdfFwd == 0.f) break;
                beta *= f * AbsDot(wi, isect.shading.m_n) / pdfFwd;
                pdfRev = isect.m_bsdf->Pdf(wi, wo, BSDF_ALL);
                if (type & BSDF_SPECULAR) {
                    vertex.delta = true;
                    pdfRev = pdfFwd = 0;
                }
                beta *= CorrectShadingNormal(isect, wo, wi, mode);
                 ray = isect.SpawnRay(wi);
            }

            // Compute reverse area density at preceding vertex
            prev.pdfRev = vertex.ConvertDensity(pdfRev, prev);
        }
        return bounces;
    }

    Spectrum G(const Scene& scene, Sampler& sampler, const Vertex& v0,
        const Vertex& v1) {
        Vector3f d = v0.p() - v1.p();
        float g = 1 / LengthSqr(d);
        d *= std::sqrt(g);
        if (v0.IsOnSurface()) g *= AbsDot(v0.ns(), d);
        if (v1.IsOnSurface()) g *= AbsDot(v1.ns(), d);
        VisibilityTester vis(v0.GetInteraction(), v1.GetInteraction());
        return g * vis.Tr(scene, sampler);
    }

    float MISWeight(const Scene& scene, Vertex* lightVertices,
        Vertex* cameraVertices, Vertex& sampled, int s, int t,
        const Distribution1D& lightPdf,
        const std::unordered_map<const Light*, size_t>& lightToIndex) {
        if (s + t == 2) return 1;
        float sumRi = 0;
        // Define helper function _remap0_ that deals with Dirac delta functions
        auto remap0 = [](float f) -> float { return f != 0 ? f : 1; };

        // Temporarily update vertex properties for current strategy

        // Look up connection vertices and their predecessors
        Vertex* qs = s > 0 ? &lightVertices[s - 1] : nullptr,
            * pt = t > 0 ? &cameraVertices[t - 1] : nullptr,
            * qsMinus = s > 1 ? &lightVertices[s - 2] : nullptr,
            * ptMinus = t > 1 ? &cameraVertices[t - 2] : nullptr;

        // Update sampled vertex for $s=1$ or $t=1$ strategy
        ScopedAssignment<Vertex> a1;
        if (s == 1)
            a1 = { qs, sampled };
        else if (t == 1)
            a1 = { pt, sampled };

        // Mark connection vertices as non-degenerate
        ScopedAssignment<bool> a2, a3;
        if (pt) a2 = { &pt->delta, false };
        if (qs) a3 = { &qs->delta, false };

        // Update reverse density of vertex $\pt{}_{t-1}$
        ScopedAssignment<float> a4;
        if (pt)
            a4 = { &pt->pdfRev, s > 0 ? qs->Pdf(scene, qsMinus, *pt)
                                     : pt->PdfLightOrigin(scene, *ptMinus, lightPdf,
                                                          lightToIndex) };

        // Update reverse density of vertex $\pt{}_{t-2}$
        ScopedAssignment<float> a5;
        if (ptMinus)
            a5 = { &ptMinus->pdfRev, s > 0 ? pt->Pdf(scene, qs, *ptMinus)
                                          : pt->PdfLight(scene, *ptMinus) };

        // Update reverse density of vertices $\pq{}_{s-1}$ and $\pq{}_{s-2}$
        ScopedAssignment<float> a6;
        if (qs) a6 = { &qs->pdfRev, pt->Pdf(scene, ptMinus, *qs) };
        ScopedAssignment<float> a7;
        if (qsMinus) a7 = { &qsMinus->pdfRev, qs->Pdf(scene, pt, *qsMinus) };

        // Consider hypothetical connection strategies along the camera subpath
        float ri = 1;
        for (int i = t - 1; i > 0; --i) {
            ri *=
                remap0(cameraVertices[i].pdfRev) / remap0(cameraVertices[i].pdfFwd);
            if (!cameraVertices[i].delta && !cameraVertices[i - 1].delta)
                sumRi += ri;
        }

        // Consider hypothetical connection strategies along the light subpath
        ri = 1;
        for (int i = s - 1; i >= 0; --i) {
            ri *= remap0(lightVertices[i].pdfRev) / remap0(lightVertices[i].pdfFwd);
            bool deltaLightvertex = i > 0 ? lightVertices[i - 1].delta
                : lightVertices[0].IsDeltaLight();
            if (!lightVertices[i].delta && !deltaLightvertex) sumRi += ri;
        }
        return 1 / (1 + sumRi);
    }

  

    BDPTIntegrator::BDPTIntegrator(std::shared_ptr<Sampler> sampler, std::shared_ptr<Camera> camera, int maxDepth, bool visualizeStrategies, bool visualizeWeights, const BBox2i& pixelBounds, const std::string& lightSampleStrategy /*= "power"*/) : sampler(sampler),
        camera(camera),
        maxDepth(maxDepth),
        visualizeStrategies(visualizeStrategies),
        visualizeWeights(visualizeWeights),
        pixelBounds(pixelBounds),
        lightSampleStrategy(lightSampleStrategy)
    {

    }

    void BDPTIntegrator::Render(const Scene& scene) {
        std::unique_ptr<LightDistribution> lightDistribution =
            CreateLightSampleDistribution(lightSampleStrategy, scene);

        // Compute a reverse mapping from light pointers to offsets into the
        // scene lights vector (and, equivalently, offsets into
        // lightDistr). Added after book text was finalized; this is critical
        // to reasonable performance with 100s+ of light sources.
        std::unordered_map<const Light*, size_t> lightToIndex;
        for (size_t i = 0; i < scene.m_lights.size(); ++i)
            lightToIndex[scene.m_lights[i].get()] = i;

        // Partition the image into tiles
        Film* film = camera->m_film.get();
        const BBox2i sampleBounds = film->GetSampleBounds();
        const Vector2i sampleExtent = sampleBounds.diagonal();
        const int tileSize = 16;
        const int nXTiles = (sampleExtent.x + tileSize - 1) / tileSize;
        const int nYTiles = (sampleExtent.y + tileSize - 1) / tileSize;
        //ProgressReporter reporter(nXTiles * nYTiles, "Rendering");

        // Allocate buffers for debug visualization
        const int bufferCount = (1 + maxDepth) * (6 + maxDepth) / 2;
        std::vector<std::unique_ptr<Film>> weightFilms(bufferCount);
        if (visualizeStrategies || visualizeWeights) {
            for (int depth = 0; depth <= maxDepth; ++depth) {
                for (int s = 0; s <= depth + 2; ++s) {
                    int t = depth + 2 - s;
                    if (t == 0 || (s == 1 && t == 1)) continue;

                    std::string filename =
                        StringPrintf("bdpt_d%02i_s%02i_t%02i.exr", depth, s, t);

                    weightFilms[BufferIndex(s, t)] = std::unique_ptr<Film>(new Film(
                        film->m_fullResolution,
                        BBox2f(Point2f(0, 0), Point2f(1, 1)),
                        std::unique_ptr<Filter>(CreateBoxFilter(ParamSet())),
                        film->m_diagonal * 1000, filename, 1.f));
                }
            }
        }

        // Render and write the output image to disk
        if (scene.m_lights.size() > 0) {


            TaskQueue<TileTask> renderQueue(NumSystemCores());
            for (int y = 0; y < nYTiles; ++y)
            {
                for (int x = 0; x < nXTiles; ++x)
                {
                    TileTask t = { Vector2i(nXTiles, nYTiles), Vector2i(x,y), this, lightDistribution.get(), &lightToIndex, &scene, &weightFilms };                   
                    renderQueue.Enqueue(t);
                }//for x tiles
            }//for y tiles
            // Save final image after rendering
            renderQueue.Join();          
        }
        const float invSampleCount = 1.0f / sampler->m_samplesPerPixel;
        film->WriteImage(1.0f / sampler->m_samplesPerPixel);

        // Write buffers for debug visualization
        if (visualizeStrategies || visualizeWeights) {
            
            for (size_t i = 0; i < weightFilms.size(); ++i)
                if (weightFilms[i]) weightFilms[i]->WriteImage(invSampleCount);
        }
    }

    Spectrum ConnectBDPT(
        const Scene& scene, Vertex* lightVertices, Vertex* cameraVertices, int s,
        int t, const Distribution1D& lightDistr,
        const std::unordered_map<const Light*, size_t>& lightToIndex,
        const Camera& camera, Sampler& sampler, Point2f* pRaster,
        float* misWeightPtr) {
        Spectrum L(0.f);
        // Ignore invalid connections related to infinite area lights
        if (t > 1 && s != 0 && cameraVertices[t - 1].type == VertexType::Light)
            return Spectrum(0.f);

        // Perform connection and write contribution to _L_
        Vertex sampled;
        if (s == 0) {
            // Interpret the camera subpath as a complete path
            const Vertex& pt = cameraVertices[t - 1];
            if (pt.IsLight()) L = pt.Le(scene, cameraVertices[t - 2]) * pt.beta;
            assert(!L.HasNaNs());
        }
        else if (t == 1) {
            // Sample a point on the camera and connect it to the light subpath
            const Vertex& qs = lightVertices[s - 1];
            if (qs.IsConnectible()) {
                VisibilityTester vis;
                Vector3f wi;
                float pdf;
                Spectrum Wi = camera.Sample_Wi(qs.GetInteraction(), sampler.Get2D(),
                    &wi, &pdf, pRaster, &vis);
                if (pdf > 0 && !Wi.IsBlack()) {
                    // Initialize dynamically sampled vertex and _L_ for $t=1$ case
                    sampled = Vertex::CreateCamera(&camera, vis.P1(), Wi / pdf);
                    L = qs.beta * qs.f(sampled, eTransportMode::TRANSPORTMODE_IMPORTANCE) * sampled.beta;
                    if (qs.IsOnSurface()) L *= AbsDot(wi, qs.ns());
                    assert(!L.HasNaNs());
                    // Only check visibility after we know that the path would
                    // make a non-zero contribution.
                    if (!L.IsBlack()) L *= vis.Tr(scene, sampler);
                }
            }
        }
        else if (s == 1) {
            // Sample a point on a light and connect it to the camera subpath
            const Vertex& pt = cameraVertices[t - 1];
            if (pt.IsConnectible()) {
                float lightPdf;
                VisibilityTester vis;
                Vector3f wi;
                float pdf;
                int lightNum =
                    lightDistr.SampleDiscrete(sampler.Get1D(), &lightPdf);
                const std::shared_ptr<Light>& light = scene.m_lights[lightNum];
                Spectrum lightWeight = light->Sample_Li(
                    pt.GetInteraction(), sampler.Get2D(), &wi, &pdf, &vis);
                if (pdf > 0 && !lightWeight.IsBlack()) {
                    EndpointInteraction ei(vis.P1(), light.get());
                    sampled =
                        Vertex::CreateLight(ei, lightWeight / (pdf * lightPdf), 0);
                    sampled.pdfFwd =
                        sampled.PdfLightOrigin(scene, pt, lightDistr, lightToIndex);
                    L = pt.beta * pt.f(sampled, TRANSPORTMODE_RADIANCE) * sampled.beta;
                    if (pt.IsOnSurface()) L *= AbsDot(wi, pt.ns());
                    // Only check visibility if the path would carry radiance.
                    if (!L.IsBlack()) L *= vis.Tr(scene, sampler);
                }
            }
        }
        else {
            // Handle all other bidirectional connection cases
            const Vertex& qs = lightVertices[s - 1], & pt = cameraVertices[t - 1];
            if (qs.IsConnectible() && pt.IsConnectible()) {
                L = qs.beta * qs.f(pt, TRANSPORTMODE_RADIANCE) * pt.f(qs, TRANSPORTMODE_RADIANCE) * pt.beta;
                
                if (!L.IsBlack()) 
                    L *= G(scene, sampler, qs, pt);
            }
        }

       // ++totalPaths;
        if (L.IsBlack()) {} //++zeroRadiancePaths;
       // ReportValue(pathLength, s + t - 2);

        // Compute MIS weight for connection strategy
        float misWeight =
            L.IsBlack() ? 0.f : MISWeight(scene, lightVertices, cameraVertices,
                sampled, s, t, lightDistr, lightToIndex);
      //  VLOG(2) << "MIS weight for (s,t) = (" << s << ", " << t << ") connection: "
       //     << misWeight;
        assert(!std::isnan(misWeight));
        L *= misWeight;
        if (misWeightPtr) *misWeightPtr = misWeight;
        return L;
    }

    BDPTIntegrator* CreateBDPTIntegrator(const ParamSet& params,
        std::shared_ptr<Sampler> sampler,
        std::shared_ptr<Camera> camera) {
        int maxDepth = params.FindOneInt("maxdepth", 5);
        bool visualizeStrategies = params.FindOneBool("visualizestrategies", false);
        bool visualizeWeights = params.FindOneBool("visualizeweights", false);

        if ((visualizeStrategies || visualizeWeights) && maxDepth > 5) {
            Warning(
                "visualizestrategies/visualizeweights was enabled, limiting "
                "maxdepth to 5");
            maxDepth = 5;
        }
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

        std::string lightStrategy = params.FindOneString("lightsamplestrategy",
            "power");
        return new BDPTIntegrator(sampler, camera, maxDepth, visualizeStrategies,
            visualizeWeights, pixelBounds, lightStrategy);
    }


    float Vertex::PdfLightOrigin(const Scene& scene, const Vertex& v, const Distribution1D& lightDistr, const std::unordered_map<const Light*, size_t>& lightToDistrIndex) const
    {
        Vector3f w = v.p() - p();
        if (LengthSqr(w) == 0) return 0.;
        w = Normalize(w);
        if (IsInfiniteLight()) {
            // Return solid angle density for infinite light sources
            return InfiniteLightDensity(scene, lightDistr, lightToDistrIndex,
                w);
        }
        else {
            // Return solid angle density for non-infinite light sources
            float pdfPos, pdfDir, pdfChoice = 0;

            // Get pointer _light_ to the light source at the vertex
            assert(IsLight());
            const Light* light = type == VertexType::Light
                ? ei.light
                : si.m_primitive->getAreaLight();
            assert(light != nullptr);

            // Compute the discrete probability of sampling _light_, _pdfChoice_
            assert(lightToDistrIndex.find(light) != lightToDistrIndex.end());
            size_t index = lightToDistrIndex.find(light)->second;
            pdfChoice = lightDistr.DiscretePDF(index);

            light->Pdf_Le(Ray(p(), w, InfinityF32, time()), ng(), &pdfPos, &pdfDir);
            return pdfPos * pdfChoice;
        }
    }

    float Vertex::PdfLight(const Scene& scene, const Vertex& v) const
    {
        Vector3f w = v.p() - p();
        float invDist2 = 1 / LengthSqr(w);
        w *= std::sqrt(invDist2);
        float pdf;
        if (IsInfiniteLight()) {
            // Compute planar sampling density for infinite light sources
            Point3f worldCenter;
            float worldRadius;
            scene.worldBound().getBoundingSphere(worldCenter, worldRadius);
            pdf = 1 / (PI * worldRadius * worldRadius);
        }
        else {
            // Get pointer _light_ to the light source at the vertex
            assert(IsLight());
            const Light* light = type == VertexType::Light
                ? ei.light
                : si.m_primitive->getAreaLight();
            assert(light != nullptr);

            // Compute sampling density for non-infinite light sources
            float pdfPos, pdfDir;
            light->Pdf_Le(Ray(p(), w, InfinityF32, time()), ng(), &pdfPos, &pdfDir);
            pdf = pdfDir * invDist2;
        }
        if (v.IsOnSurface()) pdf *= AbsDot(v.ng(), w);
        return pdf;
    }

    Vertex Vertex::CreateLight(const EndpointInteraction& ei, const Spectrum& beta, float pdf)
    {
        Vertex v(VertexType::Light, ei, beta);
        v.pdfFwd = pdf;
        return v;
    }

    Vertex Vertex::CreateLight(const Light* light, const Ray& ray, const Normal3f& Nl, const Spectrum& Le, float pdf)
    {
        Vertex v(VertexType::Light, EndpointInteraction(light, ray, Nl), Le);
        v.pdfFwd = pdf;
        return v;
    }

    Vertex Vertex::CreateMedium(const MediumInteraction& mi, const Spectrum& beta, float pdf, const Vertex& prev)
    {
        Vertex v(mi, beta);
        v.pdfFwd = prev.ConvertDensity(pdf, v);
        return v;
    }

    Vertex Vertex::CreateSurface(const SurfaceInteraction& si, const Spectrum& beta, float pdf, const Vertex& prev)
    {
        Vertex v(si, beta);
        v.pdfFwd = prev.ConvertDensity(pdf, v);
        return v;
    }

    Vertex Vertex::CreateCamera(const Camera* camera, const Interaction& it, const Spectrum& beta)
    {
        return Vertex(VertexType::Camera, EndpointInteraction(it, camera), beta);
    }

    Vertex Vertex::CreateCamera(const Camera* camera, const Ray& ray, const Spectrum& beta)
    {
        return Vertex(VertexType::Camera, EndpointInteraction(camera, ray), beta);
    }

}

