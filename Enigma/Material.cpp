#include <exception>
#include <fstream>
#include "Memory.h"
#include "Shape.h"
#include "Volume.h"
#include "ParameterSet.h"
#include "BxDF.h"
#include "Spectrum.h"
#include "IO.h"
#include "Volume.h"
#include "MicrofacetDist.h"
#include "Interaction.h"
#include "MaterialSpecs.h"
#include "Interpolation.h"
#include "Error.h"
#include "Fourier.h"
#include "Material.h"







namespace RayTrace
{
   

    void Material::Bump(const FloatTexturePtr& d, SurfaceInteraction* si)
    {
        // Compute offset positions and evaluate displacement texture
        SurfaceInteraction siEval = *si;

        // Shift _siEval_ _du_ in the $u$ direction
        float du = .5f * (std::abs(si->m_dudx) + std::abs(si->m_dudy));
        // The most common reason for du to be zero is for ray that start from
        // light sources, where no differentials are available. In this case,
        // we try to choose a small enough du so that we still get a decently
        // accurate bump value.
        if (du == 0) 
            du = .0005f;
        siEval.m_p  = si->m_p + du * si->shading.m_dpdu;
        siEval.m_uv = si->m_uv + Vector2f(du, 0.f);
        siEval.m_n = Normalize( Cross(si->shading.m_dpdu, si->shading.m_dpdv) + du * si->m_dndu) ;
        float uDisplace = d->Evaluate(siEval);

        // Shift _siEval_ _dv_ in the $v$ direction
        float dv = .5f * (std::abs(si->m_dvdx) + std::abs(si->m_dvdy));
        if (dv == 0) dv = .0005f;
        siEval.m_p = si->m_p + dv * si->shading.m_dpdv;
        siEval.m_uv = si->m_uv + Vector2f(0.f, dv);
        siEval.m_n = Normalize(Cross(si->shading.m_dpdu, si->shading.m_dpdv) + dv * si->m_dndv);
        float vDisplace = d->Evaluate(siEval);
        float displace = d->Evaluate(*si);

        // Compute bump-mapped differential geometry
        Vector3f dpdu = si->shading.m_dpdu +
            (uDisplace - displace) / du * Vector3f(si->shading.m_n) +
            displace * Vector3f(si->shading.m_dndu);
        Vector3f dpdv = si->shading.m_dpdv +
            (vDisplace - displace) / dv * Vector3f(si->shading.m_n) +
            displace * Vector3f(si->shading.m_dndv);
        si->SetShadingGeometry(dpdu, dpdv, si->shading.m_dndu, si->shading.m_dndv,
            false);
    }


    MatteMaterial::MatteMaterial(const SpectrumTexturePtr& _Kd, const FloatTexturePtr& _sigma, const FloatTexturePtr& _bump)
        : m_Kd( _Kd )
        , m_sigma( _sigma )
        , m_bump( _bump )
    {

    }

    void MatteMaterial::computeScatteringFunctions(
        SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        // Perform bump mapping with _bumpMap_, if present
        if (m_bump) Bump(m_bump, si );

        // Evaluate textures for _MatteMaterial_ material and allocate BRDF
        si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);

        Spectrum r = m_Kd->Evaluate(*si).Clamp();
        float sig = std::clamp(m_sigma->Evaluate(*si), 0.f, 90.f);
        if (!r.IsBlack()) {
            if (sig == 0)
                si->m_bsdf->addBxDF(ARENA_ALLOC(arena, LambertianReflection)(r));
            else
                si->m_bsdf->addBxDF(ARENA_ALLOC(arena, OrenNayer)(r, sig));
        }
    }

    MetalMaterial::MetalMaterial(
        const SpectrumTexturePtr& _eta, 
        const SpectrumTexturePtr& _k, 
        const FloatTexturePtr& _rough, 
        const FloatTexturePtr& u,
        const FloatTexturePtr& v,
        const FloatTexturePtr& _bump,
        bool remapRoughness)
        : m_eta(_eta)
        , m_k(_k)
        , m_bumpMap(_bump)
        , m_uRoughness(u)
        , m_vRoughness(v)
        , m_roughness(_rough)
        , m_remapRoughness( remapRoughness )
    {

    }


    void MetalMaterial::computeScatteringFunctions(
        SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        if (m_bumpMap) 
            Bump(m_bumpMap, si);

        si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);

        float uRough =
            m_uRoughness ? m_uRoughness->Evaluate(*si) : m_roughness->Evaluate(*si);
        float vRough =
            m_vRoughness ? m_vRoughness->Evaluate(*si) : m_roughness->Evaluate(*si);
        if (m_remapRoughness) {
            uRough = TrowbridgeReitzDistribution::RoughnessToAlpha(uRough);
            vRough = TrowbridgeReitzDistribution::RoughnessToAlpha(vRough);
        }
        Fresnel* frMf = ARENA_ALLOC(arena, FresnelConductor)(1., m_eta->Evaluate(*si), m_k->Evaluate(*si));
        MicrofacetDistribution* distrib =
            ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(uRough, vRough);
        si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetReflection)(1., distrib, frMf));

    }

    MixMaterial::MixMaterial(const MaterialPtr& _mat1, const MaterialPtr& _mat2, const SpectrumTexturePtr& _scale) 
        : m_m1(_mat1)
        , m_m2(_mat2)
        , m_scale(_scale)
    {

    }

    void MixMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        // Compute weights and original _BxDF_s for mix material
        Spectrum s1 = m_scale->Evaluate(*si).Clamp();
        Spectrum s2 = (Spectrum(1.f) - s1).Clamp();
        SurfaceInteraction si2 = *si;
        m_m1->computeScatteringFunctions(si, arena, mode, allowMultipleLobes);
        m_m2->computeScatteringFunctions(&si2, arena, mode, allowMultipleLobes);

        // Initialize _si->bsdf_ with weighted mixture of _BxDF_s
        int n1 = si->m_bsdf->numComponents(),
            n2 = si2.m_bsdf->numComponents();
        for (int i = 0; i < n1; ++i)
            si->m_bsdf->m_pBxDFS[i] =
            ARENA_ALLOC(arena, ScaledBxDF)(si->m_bsdf->m_pBxDFS[i], s1);
        for (int i = 0; i < n2; ++i)
            si->m_bsdf->addBxDF(ARENA_ALLOC(arena, ScaledBxDF)(si2.m_bsdf->m_pBxDFS[i], s2));
    }

    GlassMaterial::GlassMaterial(const SpectrumTexturePtr& Kr, const SpectrumTexturePtr& Kt, const FloatTexturePtr& uRoughness, const FloatTexturePtr& vRoughness, const FloatTexturePtr& index, const FloatTexturePtr& bumpMap, bool remapRoughness) : m_Kr(Kr)
        , m_Kt(Kt)
        , m_uRoughness(uRoughness)
        , m_vRoughness(vRoughness)
        , m_index(index)
        , m_bumpMap(bumpMap)
        , m_remapRoughness(remapRoughness)
    {

    }

    void GlassMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        // Perform bump mapping with _bumpMap_, if present
        if (m_bumpMap) Bump(m_bumpMap, si);
        float eta    = m_index->Evaluate(*si);
        float urough = m_uRoughness->Evaluate(*si);
        float vrough = m_vRoughness->Evaluate(*si);
        Spectrum R   = m_Kr->Evaluate(*si).Clamp();
        Spectrum T   = m_Kt->Evaluate(*si).Clamp();
        // Initialize _bsdf_ for smooth or rough dielectric
        si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, eta);

        if (R.IsBlack() && T.IsBlack()) return;

        bool isSpecular = urough == 0 && vrough == 0;
        if (isSpecular && allowMultipleLobes) {
            si->m_bsdf->addBxDF(
                ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, eta, mode));
        }
        else {
            if (m_remapRoughness) {
                urough = TrowbridgeReitzDistribution::RoughnessToAlpha(urough);
                vrough = TrowbridgeReitzDistribution::RoughnessToAlpha(vrough);
            }
            MicrofacetDistribution* distrib =
                isSpecular ? nullptr
                : ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(
                    urough, vrough);
            if (!R.IsBlack()) {
                Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, eta);
                if (isSpecular)
                    si->m_bsdf->addBxDF(
                        ARENA_ALLOC(arena, SpecularReflection)(R, fresnel));
                else
                    si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetReflection)(
                        R, distrib, fresnel));
            }
            if (!T.IsBlack()) {
                if (isSpecular)
                    si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SpecularTransmission)(T, 1.f, eta, mode));
                else
                    si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetTransmission)(T, distrib, 1.f, eta, mode));
            }
        }
    }  

    PlasticMaterial::PlasticMaterial(const SpectrumTexturePtr& _kd, const SpectrumTexturePtr& _ks, const FloatTexturePtr& _rough, const FloatTexturePtr& _bump, bool _remapRoughness) 
        : m_Kd(_kd)
        , m_Ks(_ks)
        , m_roughness(_rough)
        , m_bumpMap(_bump)
        , m_remapRoughness( _remapRoughness )
    {
      
    }

    void PlasticMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
    {
        if (m_bumpMap) Bump(m_bumpMap, si);
        si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);
        // Initialize diffuse component of plastic material
        Spectrum kd = m_Kd->Evaluate(*si).Clamp();
        if (!kd.IsBlack())
            si->m_bsdf->addBxDF(ARENA_ALLOC(arena, LambertianReflection)(kd));

        // Initialize specular component of plastic material
        Spectrum ks = m_Ks->Evaluate(*si).Clamp();
        if (!ks.IsBlack()) {
            Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.5f, 1.f);
            // Create microfacet distribution _distrib_ for plastic material
            float rough = m_roughness->Evaluate(*si);
            if (m_remapRoughness)
                rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
            MicrofacetDistribution* distrib =
                ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(rough, rough);
            BxDF* spec =
                ARENA_ALLOC(arena, MicrofacetReflection)(ks, distrib, fresnel);
            si->m_bsdf->addBxDF(spec);
        }
    }

   TranslucentMaterial::TranslucentMaterial(
       const SpectrumTexturePtr& _kd, 
       const SpectrumTexturePtr& _ks, 
       const FloatTexturePtr& _rough, 
       const SpectrumTexturePtr& _refl, 
       const SpectrumTexturePtr& _trans, 
       const FloatTexturePtr& _bump,
       bool _remapRoughness) 
       : m_Kd(_kd)
       , m_Ks(_ks)
       , m_roughness(_rough)
       , m_reflect(_refl)
       , m_transmit(_trans)
       , m_bumpMap(_bump)
       , m_remapRoughness(_remapRoughness)
   {

   }

   void TranslucentMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       // Perform bump mapping with _bumpMap_, if present
       if (m_bumpMap) 
           Bump(m_bumpMap, si);
       float eta = 1.5f;
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, eta);

       Spectrum r = m_reflect->Evaluate(*si).Clamp();
       Spectrum t = m_transmit->Evaluate(*si).Clamp();
       if (r.IsBlack() && t.IsBlack()) return;

       Spectrum kd = m_Kd->Evaluate(*si).Clamp();
       if (!kd.IsBlack()) {
           if (!r.IsBlack())
               si->m_bsdf->addBxDF(ARENA_ALLOC(arena, LambertianReflection)(r * kd));
           if (!t.IsBlack())
               si->m_bsdf->addBxDF(ARENA_ALLOC(arena, LambertianTransmission)(t * kd));
       }
       Spectrum ks = m_Ks->Evaluate(*si).Clamp();
       if (!ks.IsBlack() && (!r.IsBlack() || !t.IsBlack())) {
           float rough = m_roughness->Evaluate(*si);
           if (m_remapRoughness)
               rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
           MicrofacetDistribution* distrib =
               ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(rough, rough);
           if (!r.IsBlack()) {
               Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, eta);
               si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetReflection)( r * ks, distrib, fresnel));
           }
           if (!t.IsBlack())
               si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetTransmission)(t * ks, distrib, 1.f, eta, mode));
       }
   }


   MirrorMaterial::MirrorMaterial(const SpectrumTexturePtr& _r, const FloatTexturePtr& _bump) : m_Kr(_r)
       , m_bumpMap(_bump)
   {

   }

   void MirrorMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       // Perform bump mapping with _bumpMap_, if present
       if (m_bumpMap) 
           Bump(m_bumpMap, si);
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);
       Spectrum R = m_Kr->Evaluate(*si).Clamp();
       if (!R.IsBlack())
           si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SpecularReflection)(
               R, ARENA_ALLOC(arena, FresnelNoOp)()));
   }   

   KdSubsurfaceMaterial::KdSubsurfaceMaterial(
       float scale, 
       float g, 
       float eta,
       const SpectrumTexturePtr& Kd, 
       const SpectrumTexturePtr& Kr, 
       const SpectrumTexturePtr& Kt, 
       const SpectrumTexturePtr& mfp, 
      
       const FloatTexturePtr& uRoughness, 
       const FloatTexturePtr& vRoughness, 
       const FloatTexturePtr& bumpMap, 
       bool remapRoughness)
       : m_scale              (scale                )
       , m_g                  (g                    )
       , m_eta                (eta                  )
       , m_Kd                 (Kd                   )
       , m_Kr                 (Kr                   )
       , m_Kt                 (Kt                   )
       , m_meanfreepath       (mfp                  )      
       , m_uRoughness         (uRoughness           )
       , m_vRoughness         (vRoughness           )
       , m_bumpMap            (bumpMap              )
       , m_remapRoughness     (remapRoughness       )
       , m_table              (100, 64              )
   {
       ComputeBeamDiffusionBSSRDF(m_g, m_eta, &m_table);
   }



   void KdSubsurfaceMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       // Perform bump mapping with _bumpMap_, if present
       if (m_bumpMap) 
           Bump(m_bumpMap, si);
       Spectrum R   = m_Kr->Evaluate(*si).Clamp();
       Spectrum T   = m_Kt->Evaluate(*si).Clamp();
       float urough = m_uRoughness->Evaluate(*si);
       float vrough = m_vRoughness->Evaluate(*si);

       // Initialize _bsdf_ for smooth or rough dielectric
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, m_eta);

       if (R.IsBlack() && T.IsBlack()) return;

       bool isSpecular = urough == 0 && vrough == 0;
       if (isSpecular && allowMultipleLobes) {
           si->m_bsdf->addBxDF(
               ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, m_eta, mode));
       }
       else {
           if (m_remapRoughness) {
               urough = TrowbridgeReitzDistribution::RoughnessToAlpha(urough);
               vrough = TrowbridgeReitzDistribution::RoughnessToAlpha(vrough);
           }
           MicrofacetDistribution* distrib =
               isSpecular ? nullptr
               : ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(
                   urough, vrough);
           if (!R.IsBlack()) {
               Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, m_eta);
               if (isSpecular)
                   si->m_bsdf->addBxDF( ARENA_ALLOC(arena, SpecularReflection)(R, fresnel));
               else
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetReflection)( R, distrib, fresnel));
           }
           if (!T.IsBlack()) {
               if (isSpecular)
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SpecularTransmission)(T, 1.f, m_eta, mode));
               else
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetTransmission)( T, distrib, 1.f, m_eta, mode));
           }
       }

       Spectrum mfree = m_scale * m_meanfreepath->Evaluate(*si).Clamp();
       Spectrum kd    = m_Kd->Evaluate(*si).Clamp();
       Spectrum sig_a, sig_s;
       SubsurfaceFromDiffuse(m_table, kd, mfree, &sig_a, &sig_s);
       si->m_bssrdf = ARENA_ALLOC(arena, TabulatedBSSRDF)(*si, this, mode, m_eta, sig_a, sig_s, m_table);
   }

   SubstrateMaterial::SubstrateMaterial(const SpectrumTexturePtr& _kd, const SpectrumTexturePtr& _ks,
       const FloatTexturePtr& _u, const FloatTexturePtr& _v, const FloatTexturePtr& _bump, bool _remapRoughness)
       : m_Kd(_kd)
       , m_Ks(_ks)
       , m_nu(_u)
       , m_nv(_v)
       , m_bumpMap(_bump)
       , m_remapRoughness( _remapRoughness )
   {

   }

   void SubstrateMaterial::computeScatteringFunctions(
       SurfaceInteraction* si, 
       MemoryArena& arena, 
       eTransportMode mode, 
       bool allowMultipleLobes) const
   {
       if (m_bumpMap)
           Bump(m_bumpMap, si);
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);

       Spectrum d   = m_Kd->Evaluate(*si).Clamp();
       Spectrum s   = m_Ks->Evaluate(*si).Clamp();
       float roughu = m_nu->Evaluate(*si);
       float roughv = m_nv->Evaluate(*si);

       if (!d.IsBlack() || !s.IsBlack()) {
           if (m_remapRoughness) {
               roughu = TrowbridgeReitzDistribution::RoughnessToAlpha(roughu);
               roughv = TrowbridgeReitzDistribution::RoughnessToAlpha(roughv);
           }
           MicrofacetDistribution* distrib =
               ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(roughu, roughv);
           si->m_bsdf->addBxDF(ARENA_ALLOC(arena, FresnelBlend)(d, s, distrib));
       }
   }

   
   SubsurfaceMaterial::SubsurfaceMaterial(float _scale, float _g, float _eta, const SpectrumTexturePtr& _Kr, 
       const SpectrumTexturePtr& _Kt, const SpectrumTexturePtr& _sigma_a,
       const SpectrumTexturePtr& _sigma_s, const FloatTexturePtr& _uRoughness,
       const FloatTexturePtr& _vRoughness, const FloatTexturePtr& _bumpMap, bool _remapRoughness)
   
       : m_scale(_scale)
       , m_g(_g)
       , m_eta(_eta)
       , m_Kr(_Kr)
       , m_Kt(_Kt)
       , m_sigma_a(_sigma_a)
       , m_sigma_s(_sigma_s)
       , m_uRoughness(_uRoughness)
       , m_vRoughness(_vRoughness)
       , m_bumpMap(_bumpMap)
       , m_remapRoughness(_remapRoughness)
       , m_table(100, 64)
   {
       ComputeBeamDiffusionBSSRDF(m_g, m_eta, &m_table);
   }


   void SubsurfaceMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       if (m_bumpMap) Bump(m_bumpMap, si);

       // Initialize BSDF for _SubsurfaceMaterial_
       Spectrum R = m_Kr->Evaluate(*si).Clamp();
       Spectrum T = m_Kt->Evaluate(*si).Clamp();
       float urough = m_uRoughness->Evaluate(*si);
       float vrough = m_vRoughness->Evaluate(*si);

       // Initialize _bsdf_ for smooth or rough dielectric
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, m_eta);

       if (R.IsBlack() && T.IsBlack()) return;

       bool isSpecular = urough == 0 && vrough == 0;
       if (isSpecular && allowMultipleLobes) {
           si->m_bsdf->addBxDF(
               ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, m_eta, mode));
       }
       else {
           if (m_remapRoughness) {
               urough = TrowbridgeReitzDistribution::RoughnessToAlpha(urough);
               vrough = TrowbridgeReitzDistribution::RoughnessToAlpha(vrough);
           }
           MicrofacetDistribution* distrib =
               isSpecular ? nullptr
               : ARENA_ALLOC(arena, TrowbridgeReitzDistribution)( urough, vrough );
           if (!R.IsBlack()) {
               Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, m_eta);
               if (isSpecular)
                   si->m_bsdf->addBxDF( ARENA_ALLOC(arena, SpecularReflection)( R, fresnel ) );
               else
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetReflection)( R, distrib, fresnel ) );
           }
           if (!T.IsBlack()) {
               if (isSpecular)
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SpecularTransmission)( T, 1.f, m_eta, mode));
               else
                   si->m_bsdf->addBxDF(ARENA_ALLOC(arena, MicrofacetTransmission)( T, distrib, 1.f, m_eta, mode));
           }
       }
       Spectrum sig_a = m_scale * m_sigma_a->Evaluate(*si).Clamp();
       Spectrum sig_s = m_scale * m_sigma_s->Evaluate(*si).Clamp();
       si->m_bssrdf = ARENA_ALLOC(arena, TabulatedBSSRDF)(*si, this, mode, m_eta, sig_a, sig_s, m_table);
   }

   UberMaterial::UberMaterial(const SpectrumTexturePtr& _Kd, const SpectrumTexturePtr& _Ks, const SpectrumTexturePtr& _Kr, const SpectrumTexturePtr& _Kt, const SpectrumTexturePtr& _opacity, const FloatTexturePtr& _roughness, const FloatTexturePtr& _roughnessu, const FloatTexturePtr& _roughnessv, const FloatTexturePtr& _eta, const FloatTexturePtr& _bumpMap, bool _remapRoughness)
       : m_Kd             (_Kd               )
       , m_Ks             (_Ks               )
       , m_Kr             (_Kr               )
       , m_Kt             (_Kt               )
       , m_opacity        (_opacity          )
       , m_roughness      (_roughness        )
       , m_roughnessU     (_roughnessu       )
       , m_roughnessV     (_roughnessv       )
       , m_eta            (_eta              )
       , m_bumpMap        (_bumpMap          )
       , m_remapRoughness (_remapRoughness   )   
   {

   }

   void UberMaterial::computeScatteringFunctions(SurfaceInteraction* si,
       MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       // Perform bump mapping with _bumpMap_, if present
       if (m_bumpMap) Bump(m_bumpMap, si);
       float e = m_eta->Evaluate(*si);

       Spectrum op = m_opacity->Evaluate(*si).Clamp();
       Spectrum t = (-op + Spectrum(1.f)).Clamp();
       if (!t.IsBlack()) {
           si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, 1.f);
           BxDF* tr = ARENA_ALLOC(arena, SpecularTransmission)(t, 1.f, 1.f, mode);
           si->m_bsdf->addBxDF(tr);
       }
       else
           si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si, e);

       Spectrum kd = op * m_Kd->Evaluate(*si).Clamp();
       if (!kd.IsBlack()) {
           BxDF* diff = ARENA_ALLOC(arena, LambertianReflection)(kd);
           si->m_bsdf->addBxDF(diff);
       }

       Spectrum ks = op * m_Ks->Evaluate(*si).Clamp();
       if (!ks.IsBlack()) {
           Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, e);
           float roughu, roughv;
           if (m_roughnessU)
               roughu = m_roughnessU->Evaluate(*si);
           else
               roughu = m_roughness->Evaluate(*si);
           if (m_roughnessV)
               roughv = m_roughnessV->Evaluate(*si);
           else
               roughv = roughu;
           if (m_remapRoughness) {
               roughu = TrowbridgeReitzDistribution::RoughnessToAlpha(roughu);
               roughv = TrowbridgeReitzDistribution::RoughnessToAlpha(roughv);
           }
           MicrofacetDistribution* distrib =
               ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(roughu, roughv);
           BxDF* spec =
               ARENA_ALLOC(arena, MicrofacetReflection)(ks, distrib, fresnel);
           si->m_bsdf->addBxDF(spec);
       }

       Spectrum kr = op * m_Kr->Evaluate(*si).Clamp();
       if (!kr.IsBlack()) {
           Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, e);
           si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SpecularReflection)(kr, fresnel));
       }

       Spectrum kt = op * m_Kt->Evaluate(*si).Clamp();
       if (!kt.IsBlack())
           si->m_bsdf->addBxDF(
               ARENA_ALLOC(arena, SpecularTransmission)(kt, 1.f, e, mode));
   }

   MatteMaterial* CreateMatteMaterial(const TextureParams& _param)
   {
       SpectrumTexturePtr Kd = _param.GetSpectrumTexture("Kd", Spectrum(0.5f));
       FloatTexturePtr sigma = _param.GetFloatTexture("sigma", 0.f);
       FloatTexturePtr bumpMap = _param.GetFloatTextureOrNull("bumpmap");
       return new MatteMaterial(Kd, sigma, bumpMap);
   }

   MetalMaterial* CreateMetalMaterial(const TextureParams& _param)
   {
       static Spectrum copperN = Spectrum::FromSampled(CopperWavelengths, CopperN, CopperSamples);
       static Spectrum copperK = Spectrum::FromSampled(CopperWavelengths, CopperK, CopperSamples);

       SpectrumTexturePtr eta     = _param.GetSpectrumTexture("eta", copperN);       
       SpectrumTexturePtr k       = _param.GetSpectrumTexture("k", copperK);
       FloatTexturePtr roughness  = _param.GetFloatTexture("roughness", .5f);
       FloatTexturePtr uRoughness = _param.GetFloatTextureOrNull("uroughness");
       FloatTexturePtr vRoughness = _param.GetFloatTextureOrNull("vroughness");
       FloatTexturePtr bumpMap    = _param.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness        = _param.FindBool("remaproughness", true);
       return new MetalMaterial(eta, k, roughness, uRoughness, vRoughness, bumpMap, remapRoughness);
   }

   MixMaterial* CreateMixMaterial(const TextureParams& _param, const MaterialPtr& _m1, const MaterialPtr& _m2)
   {
       SpectrumTexturePtr scale = _param.GetSpectrumTexture("amount", Spectrum(0.5f));
       return new MixMaterial(_m1, _m2, scale);
   }

   GlassMaterial* CreateGlassMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
       SpectrumTexturePtr Kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
       FloatTexturePtr eta = mp.GetFloatTextureOrNull("eta");
       if (!eta) eta =
           mp.GetFloatTexture("index", 1.5f);
       FloatTexturePtr roughu = mp.GetFloatTexture("uroughness", 0.f);
       FloatTexturePtr roughv = mp.GetFloatTexture("vroughness", 0.f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);

       return new GlassMaterial(Kr, Kt, roughu, roughv, eta, bumpMap,remapRoughness);
   }

   PlasticMaterial* CreatePlasticMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kd = mp.GetSpectrumTexture("Kd", Spectrum(0.25f));
       SpectrumTexturePtr Ks = mp.GetSpectrumTexture("Ks", Spectrum(0.25f));
       FloatTexturePtr roughness = mp.GetFloatTexture("roughness", .1f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new PlasticMaterial(Kd, Ks, roughness, bumpMap, remapRoughness);
   } 

   MirrorMaterial* CreateMirrorMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kr = mp.GetSpectrumTexture("Kr", Spectrum(0.9f));
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       return new MirrorMaterial(Kr, bumpMap);
   }

   KdSubsurfaceMaterial* CreateKdSubsurfaceMaterial( const TextureParams& mp )
   {
       float Kd[3] = { .5, .5, .5 };
       SpectrumTexturePtr kd = mp.GetSpectrumTexture("Kd", Spectrum::FromRGB(Kd));
       SpectrumTexturePtr mfp = mp.GetSpectrumTexture("mfp", Spectrum(1.f));
       SpectrumTexturePtr kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
       SpectrumTexturePtr kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
       FloatTexturePtr roughu = mp.GetFloatTexture("uroughness", 0.f);
       FloatTexturePtr roughv = mp.GetFloatTexture("vroughness", 0.f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       float eta = mp.FindFloat("eta", 1.33f);
       float scale = mp.FindFloat("scale", 1.0f);
       float g = mp.FindFloat("g", 0.0f);
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new KdSubsurfaceMaterial(scale, g, eta, kd, kr, kt, mfp, roughu, roughv, bumpMap, remapRoughness);

   }

   SubstrateMaterial* CreateSubstrateMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kd = mp.GetSpectrumTexture("Kd", Spectrum(.5f));
       SpectrumTexturePtr Ks = mp.GetSpectrumTexture("Ks", Spectrum(.5f));
       FloatTexturePtr uroughness = mp.GetFloatTexture("uroughness", .1f);
       FloatTexturePtr vroughness = mp.GetFloatTexture("vroughness", .1f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new SubstrateMaterial(Kd, Ks, uroughness, vroughness, bumpMap, remapRoughness);
   }

   SubsurfaceMaterial* CreateSubsurfaceMaterial(const TextureParams& mp)
   {
       float sig_a_rgb[3] = { .0011f, .0024f, .014f },
             sig_s_rgb[3] = { 2.55f, 3.21f, 3.77f };
       Spectrum sig_a = Spectrum::FromRGB(sig_a_rgb),
                sig_s = Spectrum::FromRGB(sig_s_rgb);
       std::string name = mp.FindString("name");
       bool found = GetMediumScatteringProperties(name, &sig_a, &sig_s);
       float g = mp.FindFloat("g", 0.0f);
       if (name != "") 
       {
           if (!found) {
               // Warning("Named material \"%s\" not found.  Using defaults.",
              //      name.c_str());
           }
           else {
               g = 0; /* Enforce g=0 (the database specifies reduced scattering
                         coefficients) */
           }
       }
       float scale = mp.FindFloat("scale", 1.f);
       float eta =   mp.FindFloat("eta", 1.33f);

       SpectrumTexturePtr sigma_a, sigma_s;
       sigma_a = mp.GetSpectrumTexture("sigma_a", sig_a);
       sigma_s = mp.GetSpectrumTexture("sigma_s", sig_s);
       SpectrumTexturePtr Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
       SpectrumTexturePtr Kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
       FloatTexturePtr roughu  = mp.GetFloatTexture("uroughness", 0.f);
       FloatTexturePtr roughv  = mp.GetFloatTexture("vroughness", 0.f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new SubsurfaceMaterial(scale, g, eta, Kr, Kt, sigma_a, sigma_s,
                                     roughu, roughv, bumpMap, remapRoughness);
   }

   UberMaterial* CreateUberMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kd = mp.GetSpectrumTexture("Kd", Spectrum(0.25f));
       SpectrumTexturePtr Ks = mp.GetSpectrumTexture("Ks", Spectrum(0.25f));
       SpectrumTexturePtr Kr = mp.GetSpectrumTexture("Kr", Spectrum(0.f));
       SpectrumTexturePtr Kt = mp.GetSpectrumTexture("Kt", Spectrum(0.f));
       FloatTexturePtr roughness = mp.GetFloatTexture("roughness", .1f);
       FloatTexturePtr uroughness = mp.GetFloatTextureOrNull("uroughness");
       FloatTexturePtr vroughness = mp.GetFloatTextureOrNull("vroughness");
       FloatTexturePtr eta = mp.GetFloatTextureOrNull("eta");
       if (!eta) 
           eta = mp.GetFloatTexture("index", 1.5f);
       SpectrumTexturePtr opacity = mp.GetSpectrumTexture("opacity", 1.f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new UberMaterial(Kd, Ks, Kr, Kt, opacity, roughness, uroughness, vroughness, eta, bumpMap, remapRoughness);     
   }

   TranslucentMaterial* CreateTranslucentMaterial(const TextureParams& mp)
   {
       SpectrumTexturePtr Kd = mp.GetSpectrumTexture("Kd", Spectrum(0.25f));
       SpectrumTexturePtr Ks = mp.GetSpectrumTexture("Ks", Spectrum(0.25f));
       SpectrumTexturePtr reflect = mp.GetSpectrumTexture("reflect", Spectrum(0.5f));
       SpectrumTexturePtr transmit = mp.GetSpectrumTexture("transmit", Spectrum(0.5f));
       FloatTexturePtr roughness = mp.GetFloatTexture("roughness", .1f);
       FloatTexturePtr bumpMap = mp.GetFloatTextureOrNull("bumpmap");
       bool remapRoughness = mp.FindBool("remaproughness", true);
       return new TranslucentMaterial(Kd, Ks, roughness, reflect, transmit, bumpMap, remapRoughness);
   }  

   FourierMaterial* CreateFourierMaterial(const TextureParams& _param)
   {
      return new FourierMaterial(_param.FindFilename("bsdffile"), _param.GetFloatTextureOrNull("bumpmap") );
   }

   std::map<std::string, std::unique_ptr<FourierBSDFTable>> FourierMaterial::m_loadedBSDFs;


   FourierMaterial::FourierMaterial(const std::string& _filename, const FloatTexturePtr& _bump)
       : m_bsdfTable(nullptr)
       , m_bumpMap(_bump)
   {
       if (m_loadedBSDFs.find(_filename) == std::end(m_loadedBSDFs)) {
           std::unique_ptr<FourierBSDFTable> table(new FourierBSDFTable);
           if (FourierBSDFTable::Read(_filename, table.get()))
               m_loadedBSDFs[_filename] = std::move(table);

       }
       m_bsdfTable = m_loadedBSDFs[_filename].get();
   }

   void FourierMaterial::computeScatteringFunctions(SurfaceInteraction* si, MemoryArena& arena, eTransportMode mode, bool allowMultipleLobes) const
   {
       // Perform bump mapping with _bumpMap_, if present
       if (m_bumpMap) Bump(m_bumpMap, si);
       si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);
       // Checking for zero channels works as a proxy for checking whether the
       // table was successfully read from the file.
       if (m_bsdfTable->header.nChannels > 0)
           si->m_bsdf->addBxDF(ARENA_ALLOC(arena, FourierBSDF)(*m_bsdfTable, mode));
   }

}



