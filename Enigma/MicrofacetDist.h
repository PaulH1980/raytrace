#pragma once
#include "Defines.h"

namespace RayTrace
{

    

    //////////////////////////////////////////////////////////////////////////
    /// class MicrofacetDistribution 
    //////////////////////////////////////////////////////////////////////////
    class MicrofacetDistribution
    {
    public:
        MicrofacetDistribution(bool _vis) :m_sampleVisArea(_vis) {}
        virtual float			D(const Vector3f& _wh)     const = 0;
        virtual float			Lambda(const  Vector3f& w) const = 0;
        virtual Vector3f	Sample_wh(const Vector3f& wo, const Vector2f& u) const = 0;
        virtual float           Pdf(const Vector3f& wo, const Vector3f& wh) const;

        virtual float			G(const Vector3f& wo, const Vector3f& wi) const;
        float					G1(const Vector3f& w) const;

        bool m_sampleVisArea;
    };


    class BeckmannDistribution : public MicrofacetDistribution {
    public:
        // BeckmannDistribution Public Methods

        BeckmannDistribution(float alphax, float alphay, bool samplevis = true);
        float			D(const Vector3f& wh) const override;
        Vector3f  Sample_wh(const Vector3f& wo, const Vector2f& u) const override;
        static float	RoughnessToAlpha(float roughness);

    private:
        float Lambda(const Vector3f& w) const override;

        // BeckmannDistribution Private Data
        const float alphax, alphay;
    };

    //

    //////////////////////////////////////////////////////////////////////////
    /// class TrowbridgeReitzDistribution
    //////////////////////////////////////////////////////////////////////////
    class TrowbridgeReitzDistribution : public MicrofacetDistribution {
    public:
        // TrowbridgeReitzDistribution Public Methods
        static float RoughnessToAlpha(float roughness);

        TrowbridgeReitzDistribution(float alphax, float alphay, bool samplevis = true);

        float D(const Vector3f& wh) const override;

        Vector3f Sample_wh(const Vector3f& w, const Vector2f& u) const override;
    private:
        // TrowbridgeReitzDistribution Private Methods
        float Lambda(const Vector3f& w) const;

        // TrowbridgeReitzDistribution Private Data
        const float alphax, alphay;
    };
}