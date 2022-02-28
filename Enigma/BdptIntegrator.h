#pragma once

#include "Integrator.h"

namespace RayTrace
{


    /// Forward declaration (correction term for adjoint BSDF with shading normals)
    extern float CorrectShadingNormal(const SurfaceInteraction& isect,
        const Vector3f& wo, const Vector3f& wi,
        eTransportMode mode);

    // EndpointInteraction Declarations
    struct EndpointInteraction : Interaction {
        union {
            const Camera* camera;
            const Light* light;
        };
        // EndpointInteraction Public Methods
        EndpointInteraction() : Interaction(), light(nullptr) {}
        EndpointInteraction(const Interaction& it, const Camera* camera)
            : Interaction(it), camera(camera) {}
        EndpointInteraction(const Camera* camera, const Ray& ray)
            : Interaction(ray.m_origin, ray.m_time, ray.m_medium), camera(camera) {}
        EndpointInteraction(const Light* light, const Ray& r, const Normal3f& nl)
            : Interaction(r.m_origin, r.m_time, r.m_medium), light(light) {
            m_n = nl;
        }
        EndpointInteraction(const Interaction& it, const Light* light)
            : Interaction(it), light(light) {}
        EndpointInteraction(const Ray& ray)
            : Interaction(ray.scale(1.0f), ray.m_time, ray.m_medium), light(nullptr) {
            m_n = Normal3f(-ray.m_dir);
        }
    };

    // BDPT Helper Definitions
    enum class VertexType { Camera, Light, Surface, Medium };
    struct Vertex;
    template <typename Type>
    class ScopedAssignment {
    public:
        // ScopedAssignment Public Methods
        ScopedAssignment(Type* target = nullptr, Type value = Type())
            : target(target) {
            if (target) {
                backup = *target;
                *target = value;
            }
        }
        ~ScopedAssignment() {
            if (target) *target = backup;
        }
        ScopedAssignment(const ScopedAssignment&) = delete;
        ScopedAssignment& operator=(const ScopedAssignment&) = delete;
        ScopedAssignment& operator=(ScopedAssignment&& other) {
            if (target) *target = backup;
            target = other.target;
            backup = other.backup;
            other.target = nullptr;
            return *this;
        }

    private:
        Type* target, backup;
    };

    float InfiniteLightDensity(
        const Scene& scene, const Distribution1D& lightDistr,
        const std::unordered_map<const Light*, size_t>& lightToDistrIndex,
        const Vector3f& w);

    // BDPT Declarations
    class BDPTIntegrator : public Integrator {
    public:
        // BDPTIntegrator Public Methods
        BDPTIntegrator(std::shared_ptr<Sampler> sampler,
            std::shared_ptr<Camera> camera, int maxDepth,
            bool visualizeStrategies, bool visualizeWeights,
            const BBox2i& pixelBounds,
            const std::string& lightSampleStrategy = "power");
        void Render(const Scene& scene);

        const SamplerPtr& GetSampler() const { return sampler; }
        const CameraPtr& GetCamera() const override{ return camera; }
        bool VisWeights() const { return visualizeWeights; }
        bool VisStrategies() const { return visualizeStrategies; }
        bool VisualizeDebug() const { return visualizeStrategies || visualizeWeights;}
        int  MaxDepth() const { return maxDepth; }
      
        const BBox2i& GetPixelBounds() const {
            return pixelBounds;
        }


    private:
        // BDPTIntegrator Private Data
        std::shared_ptr<Sampler> sampler;
        std::shared_ptr<Camera> camera;
        const int maxDepth;
        const bool visualizeStrategies;
        const bool visualizeWeights;
        const BBox2i pixelBounds;
        const std::string lightSampleStrategy;
    };

    struct Vertex {
        // Vertex Public Data
        VertexType type;
        Spectrum beta;
#ifdef PBRT_HAVE_NONPOD_IN_UNIONS
        union {
#else
        struct {
#endif  // PBRT_HAVE_NONPOD_IN_UNIONS
            EndpointInteraction ei;
            MediumInteraction mi;
            SurfaceInteraction si;
        };
        bool delta = false;
        float pdfFwd = 0, pdfRev = 0;

        // Vertex Public Methods
        Vertex();
        Vertex(VertexType type, const EndpointInteraction& ei, const Spectrum& beta);
        Vertex(const SurfaceInteraction& si, const Spectrum& beta);

        // Need to define these two to make compilers happy with the non-POD
        // objects in the anonymous union above.
        Vertex(const Vertex& v);
        Vertex& operator=(const Vertex& v);

        static inline Vertex CreateCamera(const Camera* camera, const Ray& ray,
            const Spectrum& beta);
        static inline Vertex CreateCamera(const Camera* camera,
            const Interaction& it,
            const Spectrum& beta);
        static inline Vertex CreateLight(const Light* light, const Ray& ray,
            const Normal3f& nLight, const Spectrum& Le,
            float pdf);
        static inline Vertex CreateLight(const EndpointInteraction& ei,
            const Spectrum& beta, float pdf);
        static inline Vertex CreateMedium(const MediumInteraction& mi,
            const Spectrum& beta, float pdf,
            const Vertex& prev);
        static inline Vertex CreateSurface(const SurfaceInteraction& si,
            const Spectrum& beta, float pdf,
            const Vertex& prev);
        Vertex(const MediumInteraction& mi, const Spectrum& beta);
        const Interaction& GetInteraction() const;
        const Point3f& p() const;
        float time() const;
        const Normal3f& ng() const;
        const Normal3f& ns() const;
        bool IsOnSurface() const;
        Spectrum f(const Vertex& next, eTransportMode mode) const;
        bool IsConnectible() const;
        bool IsLight() const;

        bool IsDeltaLight() const;

        bool IsInfiniteLight() const;
        Spectrum Le(const Scene& scene, const Vertex& v) const;
       
        float ConvertDensity(float pdf, const Vertex& next) const;
        float Pdf(const Scene& scene, const Vertex* prev,
            const Vertex& next) const;
        float PdfLight(const Scene& scene, const Vertex& v) const;
        float PdfLightOrigin(const Scene& scene, const Vertex& v,
            const Distribution1D& lightDistr,
            const std::unordered_map<const Light*, size_t>
            & lightToDistrIndex) const;
        };

    extern int GenerateCameraSubpath(const Scene& scene, Sampler& sampler,
        MemoryArena& arena, int maxDepth,
        const Camera& camera, const Point2f& pFilm,
        Vertex* path);

    extern int GenerateLightSubpath(
        const Scene& scene, Sampler& sampler, MemoryArena& arena, int maxDepth,
        float time, const Distribution1D& lightDistr,
        const std::unordered_map<const Light*, size_t>& lightToIndex,
        Vertex* path);
    Spectrum ConnectBDPT(
        const Scene& scene, Vertex* lightVertices, Vertex* cameraVertices, int s,
        int t, const Distribution1D& lightDistr,
        const std::unordered_map<const Light*, size_t>& lightToIndex,
        const Camera& camera, Sampler& sampler, Point2f* pRaster,
        float* misWeight = nullptr);




    BDPTIntegrator* CreateBDPTIntegrator(const ParamSet& params,
        std::shared_ptr<Sampler> sampler,
        std::shared_ptr<Camera> camera);

}