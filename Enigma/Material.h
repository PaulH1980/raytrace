#pragma once
#include <memory>
#include "BxDF.h"
#include "Defines.h"
#include "Texture.h"

namespace RayTrace
{
    //Material* CreateMatteMaterial(const TextureParams& _param)
    
    class Material
	{
	public:
		Material() {}
		virtual ~Material() {}

	/*
		virtual BSDF*   getBSDF(const DifferentialGeometry& _dg, const DifferentialGeometry& _dgs, MemoryArena& _ma)  const;
		virtual BSSRDF* getBSSRDF(const DifferentialGeometry& _dg, const DifferentialGeometry& _dgs, MemoryArena& _ma)  const;
        */

        static void     Bump(const std::shared_ptr<Texture<float>>& d, SurfaceInteraction* si);

        virtual void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, 
                                                   eTransportMode mode, bool allowMultipleLobes) const = 0;

	private:
		/*static void Bump(const Reference<Texture<float> >& d, const DifferentialGeometry& dgGeom,
			const DifferentialGeometry& dgShading, DifferentialGeometry* dgBump);*/
	};

	class MatteMaterial : public Material {
	public:
		MatteMaterial(const SpectrumTexturePtr& _Kd, const FloatTexturePtr& _sigma, const FloatTexturePtr& _bump);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
                                           eTransportMode mode, bool allowMultipleLobes) const override;

	
	private:
		SpectrumTexturePtr	m_Kd;
		FloatTexturePtr     m_sigma, m_bump;
	};


	class MetalMaterial : public Material
	{
	public:
        MetalMaterial(
            const SpectrumTexturePtr& eta,
            const SpectrumTexturePtr& k,
			const FloatTexturePtr& rough,
            const FloatTexturePtr& u,
            const FloatTexturePtr& v,
            const FloatTexturePtr& bump,
            bool remapRoughness = false );
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;

    private:
        // MetalMaterial Private Data
		SpectrumTexturePtr         m_eta, 
								   m_k;
        FloatTexturePtr			   m_roughness, 
                                   m_uRoughness, 
                                   m_vRoughness;
        FloatTexturePtr			   m_bumpMap;
        bool                       m_remapRoughness;
	};

    // MixMaterial Declarations
    class MixMaterial : public Material {
    public:
        // MixMaterial Public Methods
        MixMaterial(const MaterialPtr& _mat1, const MaterialPtr& _mat2, const SpectrumTexturePtr& _scale);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // MixMaterial Private Data
        MaterialPtr m_m1, m_m2;
		SpectrumTexturePtr m_scale;
    };

    class GlassMaterial : public Material {
    public:
        // GlassMaterial Public Methods
        GlassMaterial(
            const SpectrumTexturePtr& Kr,
            const SpectrumTexturePtr& Kt,
            const FloatTexturePtr& uRoughness,
            const FloatTexturePtr& vRoughness,
            const FloatTexturePtr& index,
            const FloatTexturePtr& bumpMap,
            bool remapRoughness);

        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // GlassMaterial Private Data
		SpectrumTexturePtr  m_Kr, 
							m_Kt;
        FloatTexturePtr     m_index;
        FloatTexturePtr     m_bumpMap;
        FloatTexturePtr     m_uRoughness;
        FloatTexturePtr     m_vRoughness;
        bool                m_remapRoughness;
    };

    // PlasticMaterial Declarations
    class PlasticMaterial : public Material {
    public:
        // PlasticMaterial Public Methods
        PlasticMaterial(const SpectrumTexturePtr& _kd,
                        const SpectrumTexturePtr& _ks,
                        const FloatTexturePtr& _rough,
                        const FloatTexturePtr& _bump,
                        bool  _remapRoughness);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // PlasticMaterial Private Data
        SpectrumTexturePtr m_Kd,
                           m_Ks;
        FloatTexturePtr m_roughness, 
                        m_bumpMap;
        bool            m_remapRoughness;
    };

    //// ShinyMetalMaterial Class Declarations
    //class ShinyMetalMaterial : public Material {
    //public:
    //    // ShinyMetalMaterial Public Methods
    //    ShinyMetalMaterial(
    //        const SpectrumTexturePtr&   _ks,
    //        const FloatTexturePtr&      _rough,
    //        const SpectrumTexturePtr&   _kr,
    //        const FloatTexturePtr&      _bump);

    //    void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
    //        eTransportMode mode, bool allowMultipleLobes) const override;
    //private:
    //    // ShinyMetalMaterial Private Data
    //    SpectrumTexturePtr       m_Ks, 
    //                             m_Kr;
    //    FloatTexturePtr          m_roughness;
    //    FloatTexturePtr          m_bumpMap;
    //};


    //// MeasuredMaterial Declarations
    //class MeasuredMaterial : public Material {
    //public:
    //    // MeasuredMaterial Public Methods
    //    MeasuredMaterial(const std::string& filename, const FloatTexturePtr& _bump);

    //    void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
    //        eTransportMode mode, bool allowMultipleLobes) const override;
    //private:
    //    // MeasuredMaterial Private Data
    //    KdTree<IrregIsotropicBRDFSample>* m_thetaPhiData;
    //    float*                                    m_regularHalfangleData;
    //    uint32_t                                  m_nThetaH, 
    //                                              m_nThetaD, 
    //                                              m_nPhiD;
    //    FloatTexturePtr                           m_bumpMap;
    //};


    class TranslucentMaterial : public Material {
    public:
        // TranslucentMaterial Public Methods
        TranslucentMaterial(
            const SpectrumTexturePtr& _kd, 
            const SpectrumTexturePtr& _ks,
            const FloatTexturePtr&    _rough,
            const SpectrumTexturePtr& _refl,
            const SpectrumTexturePtr& _trans,
            const FloatTexturePtr&    _bump,
            bool _remapRoughness);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // TranslucentMaterial Private Data
        SpectrumTexturePtr  m_Kd, 
                            m_Ks;
        FloatTexturePtr     m_roughness;
        SpectrumTexturePtr  m_reflect, 
                            m_transmit;
        FloatTexturePtr     m_bumpMap;
        bool                m_remapRoughness;
    };


    // MirrorMaterial Declarations
    class MirrorMaterial : public Material {
    public:
        // MirrorMaterial Public Methods
        MirrorMaterial(const SpectrumTexturePtr& _r, const FloatTexturePtr& _bump);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // MirrorMaterial Private Data
        SpectrumTexturePtr m_Kr;
        FloatTexturePtr m_bumpMap;
    };

    // KdSubsurfaceMaterial Declarations
    class KdSubsurfaceMaterial : public Material {
    public:
        // KdSubsurfaceMaterial Public Methods
        KdSubsurfaceMaterial(
            float                     scale,
            float                     g,
            float                     eta,
            const SpectrumTexturePtr& Kd,
            const SpectrumTexturePtr& Kr,
            const SpectrumTexturePtr& Kt,
            const SpectrumTexturePtr& mfp,
            
            const FloatTexturePtr&    uRoughness,
            const FloatTexturePtr&    vRoughness,
            const FloatTexturePtr&    bumpMap,
            bool                      remapRoughness);

        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // KdSubsurfaceMaterial Private Data
        SpectrumTexturePtr m_Kd, m_Kr, m_Kt, m_meanfreepath;

        FloatTexturePtr    m_uRoughness, 
                           m_vRoughness,   
                           m_bumpMap;
        float              m_scale, m_g, m_eta;
        bool               m_remapRoughness;
        BSSRDFTable        m_table;
    };


    // SubstrateMaterial Declarations
    class SubstrateMaterial : public Material {
    public:
        // SubstrateMaterial Public Methods
        SubstrateMaterial(const SpectrumTexturePtr& _kd, const SpectrumTexturePtr& _ks, const FloatTexturePtr& _u, 
            const FloatTexturePtr& _v, const FloatTexturePtr& _bump, bool _remapRoughness);
        void	computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // SubstrateMaterial Private Data
        SpectrumTexturePtr         m_Kd, m_Ks;
        FloatTexturePtr            m_nu, m_nv;
        FloatTexturePtr            m_bumpMap;
        bool                       m_remapRoughness;
    };


    // SubsurfaceMaterial Declarations
    class SubsurfaceMaterial : public Material {
    public:
        // SubsurfaceMaterial Public Methods
        SubsurfaceMaterial(
            float                     m_scale,
            float                     m_g,
            float                     m_eta,
            const SpectrumTexturePtr& m_Kr,
            const SpectrumTexturePtr& m_Kt,
            const SpectrumTexturePtr& m_sigma_a,
            const SpectrumTexturePtr& m_sigma_s,
            const FloatTexturePtr&    m_uRoughness,
            const FloatTexturePtr&    m_vRoughness,
            const FloatTexturePtr&    m_bumpMap,
            bool                      m_remapRoughness
        );


        void	computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // SubsurfaceMaterial Private Data
        float              m_scale, m_g, m_eta;
        SpectrumTexturePtr m_Kr, 
                           m_Kt,
                           m_sigma_a, 
                           m_sigma_s;
        FloatTexturePtr    m_uRoughness, m_vRoughness, m_bumpMap;
        bool               m_remapRoughness;
        BSSRDFTable        m_table;
    };


    // UberMaterial Declarations
    class UberMaterial : public Material {
    public:
        UberMaterial(
            const SpectrumTexturePtr&   _Kd,
            const SpectrumTexturePtr&   _Ks,
            const SpectrumTexturePtr&   _Kr,
            const SpectrumTexturePtr&   _Kt,
            const SpectrumTexturePtr&   _opacity,
            const FloatTexturePtr&      _roughness,
            const FloatTexturePtr&      _roughnessu,
            const FloatTexturePtr&      _roughnessv,           
            const FloatTexturePtr&      _eta,
            const FloatTexturePtr&      _bumpMap,
            bool                        _remapRoughness);


        void	computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena,
            eTransportMode mode, bool allowMultipleLobes) const override;
    private:
        // UberMaterial Private Data
        SpectrumTexturePtr  m_Kd,
                            m_Ks,
                            m_Kr,
                            m_Kt,
                            m_opacity;
        FloatTexturePtr m_roughness,
                        m_roughnessU,
                        m_roughnessV,
                        m_eta,
                        m_bumpMap;
        bool m_remapRoughness;
    };

    class FourierMaterial : public Material
    {
    public:
        // FourierMaterial Public Methods
        FourierMaterial(const std::string& _filename, const FloatTexturePtr& _bump);      


        void computeScatteringFunctions(SurfaceInteraction* isect, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const override;

    private:
        FourierBSDFTable*    m_bsdfTable = nullptr;
        FloatTexturePtr      m_bumpMap;
        static std::map<std::string, std::unique_ptr<FourierBSDFTable>> m_loadedBSDFs;
    };



#pragma region CreateMaterials
    MatteMaterial*          CreateMatteMaterial(const TextureParams& _param);
    MetalMaterial*          CreateMetalMaterial(const TextureParams& _param);
    MixMaterial*            CreateMixMaterial(const TextureParams& _param, const MaterialPtr& _m1, const MaterialPtr& _m2);
    GlassMaterial*          CreateGlassMaterial(const TextureParams& _param);
    PlasticMaterial*        CreatePlasticMaterial(const TextureParams& _param);
    MirrorMaterial*         CreateMirrorMaterial(const TextureParams& _param);
    KdSubsurfaceMaterial*   CreateKdSubsurfaceMaterial(const TextureParams& _param);
    SubstrateMaterial*      CreateSubstrateMaterial(const TextureParams& _param);
    SubsurfaceMaterial*     CreateSubsurfaceMaterial(const TextureParams& _param);
    UberMaterial*           CreateUberMaterial(const TextureParams& _param);
    TranslucentMaterial*    CreateTranslucentMaterial(const TextureParams& _param);
    FourierMaterial*        CreateFourierMaterial(const TextureParams& _param);

#pragma endregion


}