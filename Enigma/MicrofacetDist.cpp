#include "BxDF.h"
#include "MicrofacetDist.h"



namespace RayTrace
{
    inline float ErfInv(float x) {
        float w, p;
        x = std::clamp(x, -.99999f, .99999f);
        w = -std::log((1 - x) * (1 + x));
        if (w < 5) {
            w = w - 2.5f;
            p = 2.81022636e-08f;
            p = 3.43273939e-07f + p * w;
            p = -3.5233877e-06f + p * w;
            p = -4.39150654e-06f + p * w;
            p = 0.00021858087f + p * w;
            p = -0.00125372503f + p * w;
            p = -0.00417768164f + p * w;
            p = 0.246640727f + p * w;
            p = 1.50140941f + p * w;
        }
        else {
            w = std::sqrt(w) - 3;
            p = -0.000200214257f;
            p = 0.000100950558f + p * w;
            p = 0.00134934322f + p * w;
            p = -0.00367342844f + p * w;
            p = 0.00573950773f + p * w;
            p = -0.0076224613f + p * w;
            p = 0.00943887047f + p * w;
            p = 1.00167406f + p * w;
            p = 2.83297682f + p * w;
        }
        return p * x;
    }

    inline float Erf(float x) {
        // constants
        float a1 = 0.254829592f;
        float a2 = -0.284496736f;
        float a3 = 1.421413741f;
        float a4 = -1.453152027f;
        float a5 = 1.061405429f;
        float p = 0.3275911f;

        // Save the sign of x
        int sign = 1;
        if (x < 0) sign = -1;
        x = std::abs(x);

        // A&S formula 7.1.26
        float t = 1 / (1 + p * x);
        float y =
            1 -
            (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x);

        return sign * y;
    }

    // Microfacet Utility Functions
    static void BeckmannSample11(float cosThetaI, float U1, float U2,
        float* slope_x, float* slope_y) {
        /* Special case (normal incidence) */
        if (cosThetaI > .9999) {
            float r = std::sqrt(-std::log(1.0f - U1));
            float sinPhi = std::sin(2 * PI * U2);
            float cosPhi = std::cos(2 * PI * U2);
            *slope_x = r * cosPhi;
            *slope_y = r * sinPhi;
            return;
        }

        /* The original inversion routine from the paper contained
           discontinuities, which causes issues for QMC integration
           and techniques like Kelemen-style MLT. The following code
           performs a numerical inversion with better behavior */
        float sinThetaI =
            std::sqrt(std::max((float)0, (float)1 - cosThetaI * cosThetaI));
        float tanThetaI = sinThetaI / cosThetaI;
        float cotThetaI = 1 / tanThetaI;

        /* Search interval -- everything is parameterized
           in the Erf() domain */
        float a = -1, c = Erf(cotThetaI);
        float sample_x = std::max(U1, (float)1e-6f);

        /* Start with a good initial guess */
        // float b = (1-sample_x) * a + sample_x * c;

        /* We can do better (inverse of an approximation computed in
         * Mathematica) */
        float thetaI = std::acos(cosThetaI);
        float fit = 1 + thetaI * (-0.876f + thetaI * (0.4265f - 0.0594f * thetaI));
        float b = c - (1 + c) * std::pow(1 - sample_x, fit);

        /* Normalization factor for the CDF */
        static const float SQRT_PI_INV = 1.f / std::sqrt(PI);
        float normalization =
            1 /
            (1 + c + SQRT_PI_INV * tanThetaI * std::exp(-cotThetaI * cotThetaI));

        int it = 0;
        while (++it < 10) {
            /* Bisection criterion -- the oddly-looking
               Boolean expression are intentional to check
               for NaNs at little additional cost */
            if (!(b >= a && b <= c)) b = 0.5f * (a + c);

            /* Evaluate the CDF and its derivative
               (i.e. the density function) */
            float invErf = ErfInv(b);
            float value =
                normalization *
                (1 + b + SQRT_PI_INV * tanThetaI * std::exp(-invErf * invErf)) -
                sample_x;
            float derivative = normalization * (1 - invErf * tanThetaI);

            if (std::abs(value) < 1e-5f) break;

            /* Update bisection intervals */
            if (value > 0)
                c = b;
            else
                a = b;

            b -= value / derivative;
        }

        /* Now convert back into a slope value */
        *slope_x = ErfInv(b);

        /* Simulate Y component */
        *slope_y = ErfInv(2.0f * std::max(U2, (float)1e-6f) - 1.0f);

        assert(!std::isinf(*slope_x));
        assert(!std::isnan(*slope_x));
        assert(!std::isinf(*slope_y));
        assert(!std::isnan(*slope_y));
    }

    static Vector3f BeckmannSample(const Vector3f& wi, float alpha_x, float alpha_y,
        float U1, float U2) {
        // 1. stretch wi
        Vector3f wiStretched = Normalize( Vector3f(alpha_x * wi.x, alpha_y * wi.y, wi.z) );      

        // 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
        float slope_x, slope_y;
        BeckmannSample11(CosTheta(wiStretched), U1, U2, &slope_x, &slope_y);

        // 3. rotate
        float tmp = CosPhi(wiStretched) * slope_x - SinPhi(wiStretched) * slope_y;
        slope_y = SinPhi(wiStretched) * slope_x + CosPhi(wiStretched) * slope_y;
        slope_x = tmp;

        // 4. unstretch
        slope_x = alpha_x * slope_x;
        slope_y = alpha_y * slope_y;

        // 5. compute normal
        return Normalize( Vector3f(-slope_x, -slope_y, 1.f));
    }

    static void TrowbridgeReitzSample11(float cosTheta, float U1, float U2,
        float* slope_x, float* slope_y) {
        // special case (normal incidence)
        if (cosTheta > .9999) {
            float r = sqrt(U1 / (1 - U1));
            float phi = 6.28318530718 * U2;
            *slope_x = r * cos(phi);
            *slope_y = r * sin(phi);
            return;
        }

        float sinTheta =
            std::sqrt(std::max((float)0, (float)1 - cosTheta * cosTheta));
        float tanTheta = sinTheta / cosTheta;
        float a = 1 / tanTheta;
        float G1 = 2 / (1 + std::sqrt(1.f + 1.f / (a * a)));

        // sample slope_x
        float A = 2 * U1 / G1 - 1;
        float tmp = 1.f / (A * A - 1.f);
        if (tmp > 1e10) tmp = 1e10;
        float B = tanTheta;
        float D = std::sqrt(
            std::max(float(B * B * tmp * tmp - (A * A - B * B) * tmp), float(0)));
        float slope_x_1 = B * tmp - D;
        float slope_x_2 = B * tmp + D;
        *slope_x = (A < 0 || slope_x_2 > 1.f / tanTheta) ? slope_x_1 : slope_x_2;

        // sample slope_y
        float S;
        if (U2 > 0.5f) {
            S = 1.f;
            U2 = 2.f * (U2 - .5f);
        }
        else {
            S = -1.f;
            U2 = 2.f * (.5f - U2);
        }
        float z =
            (U2 * (U2 * (U2 * 0.27385f - 0.73369f) + 0.46341f)) /
            (U2 * (U2 * (U2 * 0.093073f + 0.309420f) - 1.000000f) + 0.597999f);
        *slope_y = S * z * std::sqrt(1.f + *slope_x * *slope_x);

        assert(!std::isinf(*slope_y));
        assert(!std::isnan(*slope_y));
    }

    static Vector3f TrowbridgeReitzSample(const Vector3f& wi, float alpha_x,
        float alpha_y, float U1, float U2) {
        // 1. stretch wi
        Vector3f wiStretched = Normalize(  Vector3f(alpha_x * wi.x, alpha_y * wi.y, wi.z));

        // 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
        float slope_x, slope_y;
        TrowbridgeReitzSample11(CosTheta(wiStretched), U1, U2, &slope_x, &slope_y);

        // 3. rotate
        float tmp = CosPhi(wiStretched) * slope_x - SinPhi(wiStretched) * slope_y;
        slope_y = SinPhi(wiStretched) * slope_x + CosPhi(wiStretched) * slope_y;
        slope_x = tmp;

        // 4. unstretch
        slope_x = alpha_x * slope_x;
        slope_y = alpha_y * slope_y;

        // 5. compute normal
        return Normalize(Vector3f(-slope_x, -slope_y, 1.));
    }



    float MicrofacetDistribution::G1(const Vector3f& w) const
    {
        return 1 / (1 + Lambda(w));
    }

    float MicrofacetDistribution::Pdf(const Vector3f& wo, const Vector3f& wh) const
    {
        if (m_sampleVisArea)
            return D(wh) * G1(wo) * AbsDot(wo, wh) / AbsCosTheta(wo);
        else
            return D(wh) * AbsCosTheta(wh);
    }

    float MicrofacetDistribution::G(const Vector3f& wo, const Vector3f& wi) const
    {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }



    TrowbridgeReitzDistribution::TrowbridgeReitzDistribution(
        float alphax, float alphay, bool samplevis /*= true*/)
        : MicrofacetDistribution(samplevis),
        alphax(std::max(float(0.001), alphax)),
        alphay(std::max(float(0.001), alphay))
    {

    }

    float TrowbridgeReitzDistribution::D(const Vector3f& wh) const
    {

        float tan2Theta = Tan2Theta(wh);
        if (std::isinf(tan2Theta))
            return 0.f;
        const float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
        float e =
            (Cos2Phi(wh) / (alphax * alphax) + Sin2Phi(wh) / (alphay * alphay)) *
            tan2Theta;
        return 1 / (PI * alphax * alphay * cos4Theta * (1 + e) * (1 + e));
    }

    Vector3f TrowbridgeReitzDistribution::Sample_wh(const Vector3f& wo, const Vector2f& u) const
    {
        Vector3f wh;
        if (!m_sampleVisArea) {
            float cosTheta = 0, phi = (2 * PI) * u[1];
            if (alphax == alphay) {
                float tanTheta2 = alphax * alphax * u[0] / (1.0f - u[0]);
                cosTheta = 1 / std::sqrt(1 + tanTheta2);
            }
            else {
                phi =
                    std::atan(alphay / alphax * std::tan(2 * PI * u[1] + .5f * PI));
                if (u[1] > .5f) phi += PI;
                float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
                const float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
                const float alpha2 =
                    1 / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
                float tanTheta2 = alpha2 * u[0] / (1 - u[0]);
                cosTheta = 1 / std::sqrt(1 + tanTheta2);
            }
            float sinTheta =
                std::sqrt(std::max((float)0., (float)1. - cosTheta * cosTheta));
            wh = SphericalDirection(sinTheta, cosTheta, phi);
            if (!SameHemisphere(wo, wh)) wh = -wh;
        }
        else {
            bool flip = wo.z < 0;
            wh = TrowbridgeReitzSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
            if (flip) wh = -wh;
        }
        return wh;

    }

    float TrowbridgeReitzDistribution::Lambda(const Vector3f& w) const
    {
        float absTanTheta = std::abs(TanTheta(w));
        if (std::isinf(absTanTheta))
            return 0.;
        // Compute _alpha_ for direction _w_
        float alpha = std::sqrt(Cos2Phi(w) * alphax * alphax + Sin2Phi(w) * alphay * alphay);
        float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
        return (-1 + std::sqrt(1.f + alpha2Tan2Theta)) / 2;
    }

    float TrowbridgeReitzDistribution::RoughnessToAlpha(float roughness)
    {
        roughness = std::max(roughness, (float)1e-3);
        float x = std::log(roughness);
        return 1.62142f + 0.819955f * x + 0.1734f * x * x + 0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
    }

    float BeckmannDistribution::RoughnessToAlpha(float roughness)
    {
        roughness = std::max(roughness, (float)1e-3);
        float x = std::log(roughness);
        return 1.62142f + 0.819955f * x + 0.1734f * x * x +
            0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
    }

    BeckmannDistribution::BeckmannDistribution(float alphax, float alphay, bool samplevis /*= true*/) 
        : MicrofacetDistribution(samplevis),
        alphax(std::max(float(0.001), alphax)),
        alphay(std::max(float(0.001), alphay))
    {

    }

    float BeckmannDistribution::D(const Vector3f& wh) const
    {


        float tan2Theta = Tan2Theta(wh);
        if (std::isinf(tan2Theta)) return 0.;
        float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
        return std::exp(-tan2Theta * (Cos2Phi(wh) / (alphax * alphax) +
            Sin2Phi(wh) / (alphay * alphay))) /
            (PI * alphax * alphay * cos4Theta);
    }

    Vector3f BeckmannDistribution::Sample_wh(const Vector3f& wo, const Vector2f& u) const
    {
        if (!m_sampleVisArea) {
            // Sample full distribution of normals for Beckmann distribution

            // Compute $\tan^2 \theta$ and $\phi$ for Beckmann distribution sample
            float tan2Theta, phi;
            if (alphax == alphay) {
                float logSample = std::log(1 - u[0]);
                // DCHECK(!std::isinf(logSample));
                tan2Theta = -alphax * alphax * logSample;
                phi = u[1] * 2 * PI;
            }
            else {
                // Compute _tan2Theta_ and _phi_ for anisotropic Beckmann
                // distribution
                float logSample = std::log(1 - u[0]);
                //DCHECK(!std::isinf(logSample));
                phi = std::atan(alphay / alphax *
                    std::tan(2 * PI * u[1] + 0.5f * PI));
                if (u[1] > 0.5f) phi += PI;
                float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
                float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
                tan2Theta = -logSample /
                    (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
            }

            // Map sampled Beckmann angles to normal direction _wh_
            float cosTheta = 1 / std::sqrt(1 + tan2Theta);
            float sinTheta = std::sqrt(std::max((float)0, 1 - cosTheta * cosTheta));
            Vector3f wh = SphericalDirection(sinTheta, cosTheta, phi);
            if (!SameHemisphere(wo, wh)) wh = -wh;
            return wh;
        }
        else {
            // Sample visible area of normals for Beckmann distribution
            Vector3f wh;
            bool flip = wo.z < 0;
            wh = BeckmannSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
            if (flip)
                wh = -wh;
            return wh;
        }
    }

    float BeckmannDistribution::Lambda(const Vector3f& w) const
    {
        float absTanTheta = std::abs(TanTheta(w));
        if (std::isinf(absTanTheta)) return 0.;
        // Compute _alpha_ for direction _w_
        float alpha =
            std::sqrt(Cos2Phi(w) * alphax * alphax + Sin2Phi(w) * alphay * alphay);
        float a = 1 / (alpha * absTanTheta);
        if (a >= 1.6f) 
            return 0;
        return (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
    }
}

