//#pragma once
//#include <memory>
//#include "Defines.h"
//#include "Spectrum.h"
//
//
//namespace RayTrace
//{
//	//////////////////////////////////////////////////////////////////////////
//	/// class Renderer
//	//////////////////////////////////////////////////////////////////////////
//	class Renderer
//	{
//	public:
//
//        virtual ~Renderer() {}
//        virtual void Render(const Scene* _scene) = 0;
//        virtual Spectrum Li(const Scene* _scene, const RayDifferential& _ray,
//                            const Sample* _sample, RNG& _rng, MemoryArena& _arena,
//                            Intersection* _isect = nullptr, Spectrum* _T = nullptr) const = 0;
//        virtual Spectrum Transmittance(const Scene* _scene, const RayDifferential& _ray,
//                                       const Sample* _sample, RNG& _rng, MemoryArena& _arena) const = 0;
//	};
//
//   
//    //////////////////////////////////////////////////////////////////////////
//    // SamplerRenderer Declarations
//    //////////////////////////////////////////////////////////////////////////
//    class SamplerRenderer : public Renderer 
//    {
//    public:
//        // SamplerRenderer Public Methods
//        SamplerRenderer(const std::shared_ptr<Sampler>& _s, const std::shared_ptr<Camera>& _c, const  std::shared_ptr<SurfaceIntegrator>&  _si, 
//                        const std::shared_ptr<VolumeIntegrator> & _vi, bool visIds);
//        ~SamplerRenderer();
//        void Render(const Scene* _scene) override;
//        Spectrum Li(const Scene* _scene, const RayDifferential& _ray, const Sample* _sample, RNG& _rng, MemoryArena& _arena, Intersection* _isect = nullptr, Spectrum* _T = nullptr) const override;
//        Spectrum Transmittance(const Scene* _scene, const RayDifferential& _ray, const Sample* _sample, RNG& _rng, MemoryArena& _arena) const override;
//
//    private:
//        // SamplerRenderer Private Data
//        bool                                m_visualizeObjectIds;
//        std::shared_ptr<Sampler>            m_pSampler;
//        std::shared_ptr<Camera>             m_pCamera;
//        std::shared_ptr<SurfaceIntegrator>  m_pSurfaceIntegrator;
//        std::shared_ptr<VolumeIntegrator>   m_pVolumeIntegrator;
//    };
//
//
//
//
//    //////////////////////////////////////////////////////////////////////////
//   // TestRenderer Declarations
//   //////////////////////////////////////////////////////////////////////////
//    class TestRenderer : public Renderer
//    {
//    public:
//
//
//        void Render(const Scene* _scene) override;
//        Spectrum Li(const Scene* _scene, const RayDifferential& _ray, const Sample* _sample, RNG& _rng, MemoryArena& _arena, Intersection* _isect = nullptr, Spectrum* _T = nullptr) const override;
//        Spectrum Transmittance(const Scene* _scene, const RayDifferential& _ray, const Sample* _sample, RNG& _rng, MemoryArena& _arena) const override;
//    };
//
//
//
// 
//}