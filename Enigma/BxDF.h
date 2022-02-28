#pragma once

#include "Defines.h"
#include "Spectrum.h"
#include "KDTree.h"
#include "Memory.h"

#define MAX_BxDFS 8



namespace RayTrace
{
    // BSSRDF Utility Declarations
    float FresnelMoment1(float invEta);
    float FresnelMoment2(float invEta);
    float BeamDiffusionSS(float sigma_s, float sigma_a, float g, float eta,
        float r);
    float BeamDiffusionMS(float sigma_s, float sigma_a, float g, float eta,
        float r);
    void ComputeBeamDiffusionBSSRDF(float g, float eta, BSSRDFTable* t);
    void SubsurfaceFromDiffuse(const BSSRDFTable& table, const Spectrum& rhoEff,
        const Spectrum& mfp, Spectrum* sigma_a,
		Spectrum* sigma_s);
	
	
	
	inline float	CosTheta(const Vector3f& w) { 
		return w.z; 
	}

	inline float	AbsCosTheta(const Vector3f& w) { 
		return std::abs(CosTheta(w)); 
	}

	inline float	SinTheta2(const Vector3f& w) {
		return std::max(0.f, 1.f - CosTheta(w) * CosTheta(w));
	}

	inline float	SinTheta(const Vector3f& w) {
		return std::sqrt(SinTheta2(w));
	}

	inline float	CosPhi(const Vector3f& w) {
		float sintheta = SinTheta(w);
		if (sintheta == 0.f) 
			return 1.f;
		return std::clamp(w.x / sintheta, -1.f, 1.f);
	}

	inline float SinPhi(const Vector3f& w) {
		float sintheta = SinTheta(w);
		if (sintheta == 0.f) return 0.f;
		return std::clamp(w.y / sintheta, -1.f, 1.f);
	}

	inline bool SameHemisphere(const Vector3f& w, const Vector3f& wp) {
		return w.z * wp.z > 0.f;
	}

	inline Vector3f GetOtherHemisphere(const Vector3f& w) {
		return Vector3f(w.x, w.y, -w.z);
	}

    // BSDF Inline Functions
    inline float Cos2Theta(const Vector3f& w) { return w.z * w.z; }  
    inline float Sin2Theta(const Vector3f& w) {
        return std::max((float)0, (float)1 - Cos2Theta(w));
    }

  
    inline float TanTheta(const Vector3f& w) { return SinTheta(w) / CosTheta(w); }

    inline float Tan2Theta(const Vector3f& w) {
        return Sin2Theta(w) / Cos2Theta(w);
    }   

    inline float Cos2Phi(const Vector3f& w) { return CosPhi(w) * CosPhi(w); }

    inline float Sin2Phi(const Vector3f& w) { return SinPhi(w) * SinPhi(w); }

    inline float CosDPhi(const Vector3f& wa, const Vector3f& wb) {
        float waxy = wa.x * wa.x + wa.y * wa.y;
        float wbxy = wb.x * wb.x + wb.y * wb.y;
        if (waxy == 0 || wbxy == 0)
            return 1;
        return std::clamp((wa.x * wb.x + wa.y * wb.y) / std::sqrt(waxy * wbxy), -1.f, 1.f );
	}						 

    inline Vector3f Reflect(const Vector3f& wo, const Vector3f& n) {
        return -wo + 2 * Dot(wo, n) * n;
    }

    inline bool Refract(const Vector3f& wi, const Vector3f& n, float eta,
		Vector3f* wt) {
        // Compute $\cos \theta_\roman{t}$ using Snell's law
        float cosThetaI = Dot(n, wi);
        float sin2ThetaI = std::max(float(0), float(1 - cosThetaI * cosThetaI));
        float sin2ThetaT = eta * eta * sin2ThetaI;

        // Handle total internal reflection for transmission
        if (sin2ThetaT >= 1) return false;
        float cosThetaT = std::sqrt(1 - sin2ThetaT);
        *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * Vector3f(n);
        return true;
    }


	float FrDielectric(float cosThetaI, float etaI, float etaT);
	Spectrum FrConductor(float cosThetaI, const Spectrum& etai, const Spectrum& etat, const Spectrum& k);
	Vector3f BRDFRemap(const Vector3f& wo, const Vector3f& wi);


	
	//////////////////////////////////////////////////////////////////////////
	/// class BxDF
	//////////////////////////////////////////////////////////////////////////
	class BxDF
	{
	public:
		BxDF(eBxDFType _type) 
			: m_type(_type) 
		{
		}

		bool						matchesFlags(eBxDFType _flags) const;

		virtual Spectrum			f(const Vector3f& wo, const Vector3f& wi) const = 0;
		virtual Spectrum			sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type ) const;
		virtual Spectrum			rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const;
		virtual Spectrum			rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const;
		virtual float				pdf(const Vector3f& wo, const Vector3f& wi) const;

		eBxDFType	m_type;
	};


	//////////////////////////////////////////////////////////////////////////
	/// class BRDFToBTDF
	//////////////////////////////////////////////////////////////////////////
	class BRDFToBTDF : public BxDF {
	public:
		BRDFToBTDF( BxDF* _brdf );

		Spectrum	f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum	sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const override;
		Spectrum	rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const override;
		Spectrum	rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const override;
		float				pdf(const Vector3f& wi, const Vector3f& wo) const override;
	private:
		BxDF* m_brdf = nullptr;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class ScaledBxDF
	//////////////////////////////////////////////////////////////////////////
	class ScaledBxDF : public BxDF
	{
	public:
		ScaledBxDF(BxDF* _brdf, const Spectrum& s);

		Spectrum	f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum	sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const override;
		Spectrum	rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const override;
		Spectrum	rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const override;
	
	private:
		BxDF*			 m_brdf = nullptr;
		Spectrum m_spectrum;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class Fresnel
	//////////////////////////////////////////////////////////////////////////
	class Fresnel
	{
	public:
		virtual Spectrum evaluate(float cosi) const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class FresnelConductor
	//////////////////////////////////////////////////////////////////////////
	class FresnelConductor : public Fresnel {
	public:
		FresnelConductor( const  Spectrum& _etaI, const Spectrum& _etaT, const Spectrum& _k);

		Spectrum evaluate(float cosi) const override;

	private:
		Spectrum 
			
						 m_etaI,
						 m_etaT,
						 m_k;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class FresnelDielectric
	//////////////////////////////////////////////////////////////////////////
	class FresnelDielectric : public Fresnel {
	public:
		FresnelDielectric(float _etai, float _etat);

		Spectrum evaluate(float cosi) const override;

	private:
		float m_etai,
			  m_etat;
	};


	//////////////////////////////////////////////////////////////////////////
	/// class BxDF
	//////////////////////////////////////////////////////////////////////////
	class FresnelNoOp : public Fresnel {
		Spectrum evaluate(float cosi) const override {
			UNUSED(cosi)
			
			return 1.0f; }
	};

	//////////////////////////////////////////////////////////////////////////
	/// class SpecularRelection
	//////////////////////////////////////////////////////////////////////////
	class SpecularReflection : public BxDF
	{
	public:
		SpecularReflection(const Spectrum& _m_spectrum, Fresnel* _pFresnel);

		Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const override;
		float pdf(const Vector3f& wi, const Vector3f& wo)  const override;

	private:
		Spectrum m_spectrum;
		Fresnel* m_pFresnel = nullptr;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class SpecularTransmission
	//////////////////////////////////////////////////////////////////////////
	class SpecularTransmission : public BxDF
	{
	public:
		SpecularTransmission(const Spectrum& _spectrum, float _etai, float _etat, eTransportMode _mode);

		Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const override;
		float pdf(const Vector3f& wi, const Vector3f& wo)  const override;


	private:

		float			  m_etai = 1.0f;
		float			  m_etat = 1.0f;
		Spectrum  m_spectrumTransmision;
		FresnelDielectric m_fresnel;
		eTransportMode    m_mode;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class LambertianReflection 
	//////////////////////////////////////////////////////////////////////////
	class LambertianReflection : public BxDF
	{
	public:
		LambertianReflection(const Spectrum& _s)
			: BxDF(eBxDFType(BSDF_REFLECTION | BSDF_DIFFUSE))
			, m_spectrum(_s)
		{}

		Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const override;
		Spectrum rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const override;

	private:
		Spectrum m_spectrum;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class LambertianTransmission
	//////////////////////////////////////////////////////////////////////////
    class LambertianTransmission : public BxDF {
    public:
        // LambertianTransmission Public Methods
        LambertianTransmission(const Spectrum& T)
            : BxDF(eBxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE)), m_T(T) {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        Spectrum rho(const Vector3f&, int, const Vector2f* sample) const;
        Spectrum rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const;
        Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const;
        float			 pdf(const Vector3f& wo, const Vector3f& wi) const;
       

    private:
        // LambertianTransmission Private Data
		Spectrum m_T;
    };


	//////////////////////////////////////////////////////////////////////////
	/// class OrenNayer 
	//////////////////////////////////////////////////////////////////////////
	class OrenNayer : public BxDF
	{
	public:
		OrenNayer(const Spectrum& _refl, float _sig);
		Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;

	private:
		// OrenNayar Private Data
		Spectrum m_refl;
		float m_A = 0.0f, 
			  m_B = 0.0f;
	};

	//////////////////////////////////////////////////////////////////////////
	/// class FresnelBlend
	//////////////////////////////////////////////////////////////////////////
	class FresnelBlend : public BxDF {
	public:
		FresnelBlend(const Spectrum& _d, const Spectrum& _s, MicrofacetDistribution* _dist);

		Spectrum    f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum	SchlickFresnel(float _cosTheta) const ;
        Spectrum	sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u, float* pdf, eBxDFType* sampledType) const override;
        float		pdf(const Vector3f& wo, const Vector3f& wi) const override;

	private:
		Spectrum m_diffuse,
						 m_spec;
		MicrofacetDistribution* m_pDist = nullptr;
	};

    //////////////////////////////////////////////////////////////////////////
    /// class FresnelSpecular
    //////////////////////////////////////////////////////////////////////////
    class FresnelSpecular : public BxDF {
    public:
        // FresnelSpecular Public Methods
        FresnelSpecular(const Spectrum& R, const Spectrum& T, float etaA,
            float etaB, eTransportMode mode)
            : BxDF(eBxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR)),
            m_R(R),
            m_T(T),
            m_etaA(etaA),
            m_etaB(etaB),
            m_mode(mode) {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const { return Spectrum(0.f); }
        Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const;
        float			 pdf(const  Vector3f& wo, const  Vector3f& wi) const { 
			UNUSED(wo)
			UNUSED(wi)
			return 0; }
       

    private:
        // FresnelSpecular Private Data
        const Spectrum m_R, 
							   m_T;
        const float m_etaA, 
			        m_etaB;
        const eTransportMode m_mode;
    };

	////////////////////////////////////////////////////////////////////////////
	////KdTree Stuff
	////////////////////////////////////////////////////////////////////////////
	//struct IrregIsotropicBRDFSample {
	//	IrregIsotropicBRDFSample(const Vector3f& pp, const Spectrum& vv);
	//	IrregIsotropicBRDFSample() { }
	//	Vector3f	 m_point;
	//	Spectrum m_v;
	//};
	//
	//class IrregIsotropicBRDF : public BxDF {
	//public:
	//	// IrregIsotropicBRDF Public Methods
	//	IrregIsotropicBRDF(const KdTree<IrregIsotropicBRDFSample>* d);
	//	Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
	//private:
	//	// IrregIsotropicBRDF Private Data
	//	const KdTree<IrregIsotropicBRDFSample>* m_isoBRDFData;
	//};

	////////////////////////////////////////////////////////////////////////////
	////KdTree Stuff
	////////////////////////////////////////////////////////////////////////////
	//class RegularHalfangleBRDF : public BxDF {
	//public:
	//	// RegularHalfangleBRDF Public Methods
	//	RegularHalfangleBRDF(const float* d, uint32_t nth, uint32_t ntd, uint32_t npd);
	//	Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
	//private:
	//	// RegularHalfangleBRDF Private Data
	//	const float* brdf;
	//	const uint32_t nThetaH, nThetaD, nPhiD;
	//};

    //////////////////////////////////////////////////////////////////////////
    //class MicrofacetReflection
    //////////////////////////////////////////////////////////////////////////
    class MicrofacetReflection : public BxDF {
    public:
        // MicrofacetReflection Public Methods
        MicrofacetReflection(const Spectrum& R,
            MicrofacetDistribution* distribution, Fresnel* fresnel);

        Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
        Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u,
            float* pdf, eBxDFType* sampledType) const override;
        float pdf(const Vector3f& wo, const Vector3f& wi) const override;
      

    private:
        // MicrofacetReflection Private Data
        const  Spectrum m_R;
        const MicrofacetDistribution* m_distribution;
        const Fresnel* m_fresnel;
    };
	//////////////////////////////////////////////////////////////////////////
	//class MicrofacetTransmission
	//////////////////////////////////////////////////////////////////////////
    class MicrofacetTransmission : public BxDF {
    public:
        // MicrofacetTransmission Public Methods
        MicrofacetTransmission(const Spectrum& T,
            MicrofacetDistribution* distribution, 
			float etaA,
            float etaB, 
			eTransportMode mode);

        Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
        Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u,
            float* pdf, eBxDFType* sampledType) const override;
        float pdf(const Vector3f& wo, const Vector3f& wi) const override;
       
    private:
        // MicrofacetTransmission Private Data
        const Spectrum			m_T;
        const MicrofacetDistribution* m_distribution;
        const float m_etaA, m_etaB;
        const FresnelDielectric m_fresnel;
        const eTransportMode m_mode;
    };

	class BSDF {
	public:
		BSDF(const SurfaceInteraction& si, float _eta = 1.0f);

        void				addBxDF(BxDF* _pBxDF);

        int					numComponents() const { return m_numBxDFS; }
        int					numComponents(eBxDFType _type) const;

        Vector3f		worldToLocal(const Vector3f& _v) const;
        Vector3f		localToWorld(const Vector3f& _v) const;

        Spectrum	sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u,
								     float* pdf, eBxDFType flags = BSDF_ALL, eBxDFType* sampledType = NULL ) const;

		float				Pdf(const Vector3f& wo, const Vector3f& wi, eBxDFType flags = BSDF_ALL ) const;

        Spectrum	f(const Vector3f& woW, const Vector3f& wiW, eBxDFType flags = BSDF_ALL) const;

		Spectrum	rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2, eBxDFType flags = BSDF_ALL) const;
		Spectrum	rho(const Vector3f& wo, int nSamples, const Vector2f* samples, eBxDFType flags = BSDF_ALL) const;
				
		BxDF* m_pBxDFS[MAX_BxDFS] = { nullptr };
		float			m_eta = 1.0f;
	private:		
		

		Vector3f	m_normalGeom,
						m_normalShading;

		Vector3f	m_sTang,
						m_tTang;

		int				m_numBxDFS = 0;
		
	};

    class FourierBSDF : public BxDF {
    public:
        // FourierBSDF Public Methods
        FourierBSDF(const FourierBSDFTable& bsdfTable, eTransportMode mode);

		Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;
		Spectrum sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const override;
		float			 pdf(const Vector3f& wo, const Vector3f& wi) const override;


        

    private:
        // FourierBSDF Private Data
        const FourierBSDFTable& m_bsdfTable;
        const eTransportMode    m_mode;
    };


    // BSSRDF Declarations
    class BSSRDF {
    public:
        // BSSRDF Public Methods
        BSSRDF(const SurfaceInteraction& po, float eta);
        virtual ~BSSRDF() {}

        // BSSRDF Interface
        virtual Spectrum S(const SurfaceInteraction& pi, const Vector3f& wi) = 0;
        virtual Spectrum Sample_S(const Scene& scene, float u1, const Vector2f& u2, MemoryArena& arena, SurfaceInteraction* si, float* pdf) const = 0;

    protected:
        // BSSRDF Protected Data
        const SurfaceInteraction& m_po;
        float m_eta;
    };

    class SeparableBSSRDF : public BSSRDF {
        friend class SeparableBSSRDFAdapter;

    public:
        // SeparableBSSRDF Public Methods
        SeparableBSSRDF(const SurfaceInteraction& po, float eta,
            const Material* material, eTransportMode mode);
        
		Spectrum S(const SurfaceInteraction& pi, const Vector3f& wi) override;
        
		Spectrum Sw(const Vector3f& w) const;
        
		Spectrum Sp(const SurfaceInteraction& pi) const;
        
		Spectrum Sample_S(const Scene& scene, float u1, const Vector2f& u2,
			MemoryArena& arena, SurfaceInteraction* si,
            float* pdf) const override;
        
		Spectrum Sample_Sp(const Scene& scene, float u1, const Vector2f& u2,
			MemoryArena& arena, SurfaceInteraction* si,
            float* pdf) const;

        float Pdf_Sp(const SurfaceInteraction& si) const;

        // SeparableBSSRDF Interface
        virtual Spectrum Sr(float d) const = 0;
        virtual float Sample_Sr(int ch, float u) const = 0;
        virtual float Pdf_Sr(int ch, float r) const = 0;

    private:
        // SeparableBSSRDF Private Data
        const Vector3f	m_ns;			//normal
        const Vector3f	m_ss, m_ts;		//vector
        const Material*			m_material;
        const eTransportMode	m_mode;
    };

    class TabulatedBSSRDF : public SeparableBSSRDF {
    public:
        // TabulatedBSSRDF Public Methods
        TabulatedBSSRDF(const SurfaceInteraction& po, const Material* material,
            eTransportMode mode, float eta, const Spectrum& sigma_a,
            const Spectrum& sigma_s, const BSSRDFTable& table);
        Spectrum	Sr(float distance) const override;
        float				Pdf_Sr(int ch, float distance) const override;
        float				Sample_Sr(int ch, float sample) const override;

    private:
        // TabulatedBSSRDF Private Data
        const BSSRDFTable& m_table;
        Spectrum m_sigma_t, m_rho;
    };

    struct BSSRDFTable {
        // BSSRDFTable Public Data
        const int m_nRhoSamples, m_nRadiusSamples;
        std::unique_ptr<float[]> m_rhoSamples, m_radiusSamples;
        std::unique_ptr<float[]> m_profile;
        std::unique_ptr<float[]> m_rhoEff;
        std::unique_ptr<float[]> m_profileCDF;

        // BSSRDFTable Public Methods
        BSSRDFTable(int nRhoSamples, int nRadiusSamples);
        inline float EvalProfile(int rhoIndex, int radiusIndex) const {
            return m_profile[rhoIndex * m_nRadiusSamples + radiusIndex];
        }
    };

    class SeparableBSSRDFAdapter : public BxDF {
    public:
        // SeparableBSSRDFAdapter Public Methods
        SeparableBSSRDFAdapter(const SeparableBSSRDF* bssrdf);
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const override;       

    private:
        const SeparableBSSRDF* m_bssrdf;
    };
}

