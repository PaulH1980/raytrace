#pragma once
#include "Defines.h"
#include "Spectrum.h"
#include "Lights.h"
#include "BxDF.h"


namespace RayTrace
{
    static constexpr int TileSize = 32;
    using LightToIndexMap = std::unordered_map<const Light*, size_t>;
        
   
    Spectrum UniformSampleAllLights(const Interaction& it, const Scene& scene,
        MemoryArena& arena, Sampler& sampler,
        const std::vector<int>& nLightSamples,
        bool handleMedia = false);
    
    Spectrum UniformSampleOneLight(const Interaction& it, const Scene& scene,
        MemoryArena& arena, Sampler& sampler,
        bool handleMedia = false,
        const Distribution1D* lightDistrib = nullptr);
   
    Spectrum EstimateDirect(const Interaction& it, const Vector2f& uShading,
        const Light& light, const Vector2f& uLight,
        const Scene& scene, Sampler& sampler,
        MemoryArena& arena, bool handleMedia = false,
        bool specular = false);

    std::unique_ptr<Distribution1D> ComputeLightPowerDistribution( const Scene& scene );
    
    
    
    // Integrator Declarations
    class Integrator {
    public:
        // Integrator Interface
        virtual ~Integrator() {}
        virtual void Render(const Scene& scene) = 0;
        virtual const CameraPtr& GetCamera() const = 0;
    };

    

    // SamplerIntegrator Declarations
    class SamplerIntegrator : public Integrator {
    public:
        // SamplerIntegrator Public Methods
        SamplerIntegrator(const CameraPtr& camera,
            std::shared_ptr<Sampler> sampler,
            const BBox2i& pixelBounds);

        virtual void                Preprocess(const Scene& scene, Sampler& sampler);
        virtual void                Render(const Scene& scene);
        virtual Spectrum            Li(const RayDifferential& ray, const Scene& scene,Sampler& sampler, MemoryArena& arena, int depth = 0) const = 0;
        Spectrum                    SpecularReflect(const RayDifferential& ray, const SurfaceInteraction& isect, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const;
        Spectrum                    SpecularTransmit(const RayDifferential& ray, const SurfaceInteraction& isect, const Scene& scene, Sampler& sampler, MemoryArena& arena, int depth) const;
        const BBox2i&               GetPixelBounds() const;
        const SamplerPtr&           GetSampler() const;
        const CameraPtr&            GetCamera() const override;

    protected:
        // SamplerIntegrator Protected Data
        CameraPtr m_camera;

    private:
        // SamplerIntegrator Private Data
        std::shared_ptr<Sampler> m_sampler;
        const BBox2i       m_pixelBounds;
    };

   
    
}