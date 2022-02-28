#include <exception>
#include "MathCommon.h"
#include "MonteCarlo.h"
#include "Sample.h"
#include "MicrofacetDist.h"
#include "Interaction.h"
#include "Interpolation.h"
#include "Scene.h"
#include "Spectrum.h"
#include "Primitive.h"
#include "Fourier.h"
#include "Memory.h"
#include "BxDF.h"




namespace RayTrace
{

#pragma region BSSRDF
    // BSSRDF Utility Functions
    float FresnelMoment1(float eta) {
        float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
            eta5 = eta4 * eta;
        if (eta < 1)
            return 0.45966f - 1.73965f * eta + 3.37668f * eta2 - 3.904945 * eta3 +
            2.49277f * eta4 - 0.68441f * eta5;
        else
            return -4.61686f + 11.1136f * eta - 10.4646f * eta2 + 5.11455f * eta3 -
            1.27198f * eta4 + 0.12746f * eta5;
    }

    float FresnelMoment2(float eta) {
        float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
            eta5 = eta4 * eta;
        if (eta < 1) {
            return 0.27614f - 0.87350f * eta + 1.12077f * eta2 - 0.65095f * eta3 +
                0.07883f * eta4 + 0.04860f * eta5;
        }
        else {
            float r_eta = 1 / eta, r_eta2 = r_eta * r_eta, r_eta3 = r_eta2 * r_eta;
            return -547.033f + 45.3087f * r_eta3 - 218.725f * r_eta2 +
                458.843f * r_eta + 404.557f * eta - 189.519f * eta2 +
                54.9327f * eta3 - 9.00603f * eta4 + 0.63942f * eta5;
        }
    }

    float BeamDiffusionMS(float sigma_s, float sigma_a, float g, float eta, float r) {
        const int nSamples = 100;
        float Ed = 0;
        // Precompute information for dipole integrand

        // Compute reduced scattering coefficients $\sigmaps, \sigmapt$ and albedo
        // $\rhop$
        float sigmap_s = sigma_s * (1 - g);
        float sigmap_t = sigma_a + sigmap_s;
        float rhop = sigmap_s / sigmap_t;

        // Compute non-classical diffusion coefficient $D_\roman{G}$ using
        // Equation (15.24)
        float D_g = (2 * sigma_a + sigmap_s) / (3 * sigmap_t * sigmap_t);

        // Compute effective transport coefficient $\sigmatr$ based on $D_\roman{G}$
        float sigma_tr = std::sqrt(sigma_a / D_g);

        // Determine linear extrapolation distance $\depthextrapolation$ using
        // Equation (15.28)
        float fm1 = FresnelMoment1(eta), fm2 = FresnelMoment2(eta);
        float ze = -2 * D_g * (1 + 3 * fm2) / (1 - 2 * fm1);

        // Determine exitance scale factors using Equations (15.31) and (15.32)
        float cPhi = .25f * (1 - 2 * fm1), cE = .5f * (1 - 3 * fm2);
        for (int i = 0; i < nSamples; ++i) {
            // Sample real point source depth $\depthreal$
            float zr = -std::log(1 - (i + .5f) / nSamples) / sigmap_t;

            // Evaluate dipole integrand $E_{\roman{d}}$ at $\depthreal$ and add to
            // _Ed_
            float zv = -zr + 2 * ze;
            float dr = std::sqrt(r * r + zr * zr), dv = std::sqrt(r * r + zv * zv);

            // Compute dipole fluence rate $\dipole(r)$ using Equation (15.27)
            float phiD = INV_FOUR_PI / D_g * (std::exp(-sigma_tr * dr) / dr -
                std::exp(-sigma_tr * dv) / dv);

            // Compute dipole vector irradiance $-\N{}\cdot\dipoleE(r)$ using
            // Equation (15.27)
            float EDn = INV_FOUR_PI * (zr * (1 + sigma_tr * dr) *
                std::exp(-sigma_tr * dr) / (dr * dr * dr) -
                zv * (1 + sigma_tr * dv) *
                std::exp(-sigma_tr * dv) / (dv * dv * dv));

            // Add contribution from dipole for depth $\depthreal$ to _Ed_
            float E = phiD * cPhi + EDn * cE;
            float kappa = 1 - std::exp(-2 * sigmap_t * (dr + zr));
            Ed += kappa * rhop * rhop * E;
        }
        return Ed / nSamples;
    }

    float BeamDiffusionSS(float sigma_s, float sigma_a, float g, float eta,
        float r) {
        // Compute material parameters and minimum $t$ below the critical angle
        float sigma_t = sigma_a + sigma_s, rho = sigma_s / sigma_t;
        float tCrit = r * std::sqrt(eta * eta - 1);
        float Ess = 0;
        const int nSamples = 100;
        for (int i = 0; i < nSamples; ++i) {
            // Evaluate single scattering integrand and add to _Ess_
            float ti = tCrit - std::log(1 - (i + .5f) / nSamples) / sigma_t;

            // Determine length $d$ of connecting segment and $\cos\theta_\roman{o}$
            float d = std::sqrt(r * r + ti * ti);
            float cosThetaO = ti / d;

            // Add contribution of single scattering at depth $t$
            Ess += rho * std::exp(-sigma_t * (d + tCrit)) / (d * d) *
                PhaseHG(cosThetaO, g) * (1 - FrDielectric(-cosThetaO, 1, eta)) *
                std::abs(cosThetaO);
        }
        return Ess / nSamples;
    }

    void ComputeBeamDiffusionBSSRDF(float g, float eta, BSSRDFTable* t) {
        // Choose radius values of the diffusion profile discretization
        t->m_radiusSamples[0] = 0;
        t->m_radiusSamples[1] = 2.5e-3f;
        for (int i = 2; i < t->m_nRadiusSamples; ++i)
            t->m_radiusSamples[i] = t->m_radiusSamples[i - 1] * 1.2f;

        // Choose albedo values of the diffusion profile discretization
        for (int i = 0; i < t->m_nRhoSamples; ++i)
        {
            t->m_rhoSamples[i] =
                (1 - std::exp(-8 * i / (float)(t->m_nRhoSamples - 1))) /
                (1 - std::exp(-8));
            // ParallelFor([&](int i) 
            {
                // Compute the diffusion profile for the _i_th albedo sample

                // Compute scattering profile for chosen albedo $\rho$
                for (int j = 0; j < t->m_nRadiusSamples; ++j) {
                    float rho = t->m_rhoSamples[i],
                        r = t->m_radiusSamples[j];
                    t->m_profile[i * t->m_nRadiusSamples + j] =
                        2 * PI * r * (BeamDiffusionSS(rho, 1 - rho, g, eta, r) +
                            BeamDiffusionMS(rho, 1 - rho, g, eta, r));
                }

                // Compute effective albedo $\rho_{\roman{eff}}$ and CDF for importance
                // sampling
                t->m_rhoEff[i] =
                    IntegrateCatmullRom(t->m_nRadiusSamples, t->m_radiusSamples.get(),
                        &t->m_profile[i * t->m_nRadiusSamples],
                        &t->m_profileCDF[i * t->m_nRadiusSamples]);
            }
            //, t->m_nRhoSamples);
        }//for i
    }

    void SubsurfaceFromDiffuse(const BSSRDFTable& t, const Spectrum& rhoEff,
        const Spectrum& mfp, Spectrum* sigma_a,
        Spectrum* sigma_s) {
        for (int c = 0; c < Spectrum::nSamples; ++c) {
            float rho = InvertCatmullRom(t.m_nRhoSamples, t.m_rhoSamples.get(), t.m_rhoEff.get(), rhoEff[c]);
            (*sigma_s)[c] = rho / mfp[c];
            (*sigma_a)[c] = (1 - rho) / mfp[c];
        }
    }

    BSSRDF::BSSRDF(const SurfaceInteraction& po, float eta)
        : m_po(po)
        , m_eta(eta)
    {

    }



    SeparableBSSRDFAdapter::SeparableBSSRDFAdapter(const SeparableBSSRDF* bssrdf)
        : BxDF(eBxDFType(BSDF_REFLECTION | BSDF_DIFFUSE))
        , m_bssrdf(bssrdf)
    {

    }

    Spectrum SeparableBSSRDFAdapter::f(const Vector3f& wo, const Vector3f& wi) const
    {
        UNUSED(wo)
        
        Spectrum F = m_bssrdf->Sw(wi);
        // Update BSSRDF transmission term to account for adjoint light
        // transport
        if (m_bssrdf->m_mode == eTransportMode::TRANSPORTMODE_RADIANCE)
            F *= m_bssrdf->m_eta * m_bssrdf->m_eta;
        return F;
    }

    TabulatedBSSRDF::TabulatedBSSRDF(const SurfaceInteraction& po, const Material* material,
        eTransportMode mode, float eta, const Spectrum& sigma_a,
        const Spectrum& sigma_s, const BSSRDFTable& table)
        : SeparableBSSRDF(po, eta, material, mode)
        , m_table(table)
    {
        m_sigma_t = sigma_a + sigma_s;
        for (int c = 0; c < Spectrum::nSamples; ++c)
            m_rho[c] = m_sigma_t[c] != 0 ? (sigma_s[c] / m_sigma_t[c]) : 0;
    }



    Spectrum TabulatedBSSRDF::Sr(float r) const
    {
        Spectrum Sr(0.f);
        for (int ch = 0; ch < Spectrum::nSamples; ++ch) {
            // Convert $r$ into unitless optical radius $r_{\roman{optical}}$
            float rOptical = r * m_sigma_t[ch];

            // Compute spline weights to interpolate BSSRDF on channel _ch_
            int rhoOffset, radiusOffset;
            float rhoWeights[4], radiusWeights[4];
            if (!CatmullRomWeights(m_table.m_nRhoSamples, m_table.m_rhoSamples.get(),
                m_rho[ch], &rhoOffset, rhoWeights) ||
                !CatmullRomWeights(m_table.m_nRadiusSamples, m_table.m_radiusSamples.get(),
                    rOptical, &radiusOffset, radiusWeights))
                continue;

            // Set BSSRDF value _Sr[ch]_ using tensor spline interpolation
            float sr = 0;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    float weight = rhoWeights[i] * radiusWeights[j];
                    if (weight != 0)
                        sr += weight *
                        m_table.EvalProfile(rhoOffset + i, radiusOffset + j);
                }
            }

            // Cancel marginal PDF factor from tabulated BSSRDF profile
            if (rOptical != 0) sr /= 2 * PI * rOptical;
            Sr[ch] = sr;
        }
        // Transform BSSRDF value into world space units
        Sr *= m_sigma_t * m_sigma_t;
        return Sr.Clamp();
    }

    float TabulatedBSSRDF::Pdf_Sr(int ch, float r) const
    {
        // Convert $r$ into unitless optical radius $r_{ \roman{optical} }$
        float rOptical = r * m_sigma_t[ch];

        // Compute spline weights to interpolate BSSRDF density on channel _ch_
        int rhoOffset, radiusOffset;
        float rhoWeights[4], radiusWeights[4];
        if (!CatmullRomWeights(m_table.m_nRhoSamples, m_table.m_rhoSamples.get(), m_rho[ch],
            &rhoOffset, rhoWeights) ||
            !CatmullRomWeights(m_table.m_nRadiusSamples, m_table.m_radiusSamples.get(),
                rOptical, &radiusOffset, radiusWeights))
            return 0.f;

        // Return BSSRDF profile density for channel _ch_
        float sr = 0, rhoEff = 0;
        for (int i = 0; i < 4; ++i) {
            if (rhoWeights[i] == 0) continue;
            rhoEff += m_table.m_rhoEff[rhoOffset + i] * rhoWeights[i];
            for (int j = 0; j < 4; ++j) {
                if (radiusWeights[j] == 0) continue;
                sr += m_table.EvalProfile(rhoOffset + i, radiusOffset + j) * rhoWeights[i] * radiusWeights[j];
            }
        }

        // Cancel marginal PDF factor from tabulated BSSRDF profile
        if (rOptical != 0) sr /= 2 * PI * rOptical;
        return std::max((float)0, sr * m_sigma_t[ch] * m_sigma_t[ch] / rhoEff);
    }

    float TabulatedBSSRDF::Sample_Sr(int ch, float u) const
    {
        if (m_sigma_t[ch] == 0) return -1;
        return SampleCatmullRom2D(m_table.m_nRhoSamples, m_table.m_nRadiusSamples,
            m_table.m_rhoSamples.get(), m_table.m_radiusSamples.get(),
            m_table.m_profile.get(), m_table.m_profileCDF.get(),
            m_rho[ch], u) /
            m_sigma_t[ch];
    }

    SeparableBSSRDF::SeparableBSSRDF(const SurfaceInteraction& po, float eta, const Material* material, eTransportMode mode) : BSSRDF(po, eta),
        m_ns(po.shading.m_n),
        m_ss(Normalize(m_po.shading.m_dpdu)),
        m_ts(Cross(m_ns, m_ss)),
        m_material(material),
        m_mode(mode)
    {

    }

    Spectrum SeparableBSSRDF::S(const SurfaceInteraction& pi, const Vector3f& wi)
    {
        float Ft = FrDielectric(CosTheta(m_po.m_wo), 1, m_eta);
        return (1 - Ft) * Sp(pi) * Sw(wi);
    }

    Spectrum SeparableBSSRDF::Sw(const Vector3f& w) const
    {
        float c = 1 - 2 * FresnelMoment1(1 / m_eta);
        return (1 - FrDielectric(CosTheta(w), 1, m_eta)) / (c * PI);
    }

    Spectrum SeparableBSSRDF::Sp(const SurfaceInteraction& pi) const
    {
        return Sr(Distance(m_po.m_p, pi.m_p));
    }

    Spectrum SeparableBSSRDF::Sample_S(const Scene& scene, float u1, const Vector2f& u2,
        MemoryArena& arena, SurfaceInteraction* si, float* pdf) const
    {
        Spectrum Sp = Sample_Sp(scene, u1, u2, arena, si, pdf);
        if (!Sp.IsBlack()) {
            // Initialize material model at sampled surface interaction
            si->m_bsdf = ARENA_ALLOC(arena, BSDF)(*si);
            si->m_bsdf->addBxDF(ARENA_ALLOC(arena, SeparableBSSRDFAdapter)(this));
            si->m_wo = Vector3f(si->shading.m_n);
        }
        return Sp;
    }

    Spectrum SeparableBSSRDF::Sample_Sp(const Scene& scene, float u1,
        const Vector2f& u2, MemoryArena& arena, SurfaceInteraction* si, float* pdf) const
    {
        // Choose projection axis for BSSRDF sampling
        Vector3f vx, vy, vz;
        if (u1 < .5f) {
            vx = m_ss;
            vy = m_ts;
            vz = Vector3f(m_ns);
            u1 *= 2;
        }
        else if (u1 < .75f) {
            // Prepare for sampling rays with respect to _ss_
            vx = m_ts;
            vy = Vector3f(m_ns);
            vz = m_ss;
            u1 = (u1 - .5f) * 4;
        }
        else {
            // Prepare for sampling rays with respect to _ts_
            vx = Vector3f(m_ns);
            vy = m_ss;
            vz = m_ts;
            u1 = (u1 - .75f) * 4;
        }

        // Choose spectral channel for BSSRDF sampling
        int max = Spectrum::nSamples;
        int ch = std::clamp((int)(u1 * Spectrum::nSamples), 0, Spectrum::nSamples - 1);
        u1 = u1 * Spectrum::nSamples - ch;

        // Sample BSSRDF profile in polar coordinates
        float r = Sample_Sr(ch, u2[0]);
        if (r < 0) return Spectrum(0.f);
        float phi = 2 * PI * u2[1];

        // Compute BSSRDF profile bounds and intersection height
        float rMax = Sample_Sr(ch, 0.999f);
        if (r >= rMax) return Spectrum(0.f);
        float l = 2 * std::sqrt(rMax * rMax - r * r);

        // Compute BSSRDF sampling ray segment
        Interaction base;
        base.m_p = m_po.m_p + r * (vx * std::cos(phi) + vy * std::sin(phi)) - l * vz * 0.5f;
        base.m_time = m_po.m_time;
        Vector3f pTarget = base.m_p + l * vz;

        // Intersect BSSRDF sampling ray against the scene geometry

        // Declare _IntersectionChain_ and linked list
        struct IntersectionChain {
            SurfaceInteraction si;
            IntersectionChain* next = nullptr;
        };
        IntersectionChain* chain = ARENA_ALLOC(arena, IntersectionChain)();

        // Accumulate chain of intersections along ray
        IntersectionChain* ptr = chain;
        int nFound = 0;
        while (true) {
            Ray ray = base.SpawnRayTo(pTarget);
            if (ray.m_dir == Vector3f(0, 0, 0) || !scene.intersect(ray, &ptr->si))
                break;

            base = ptr->si;
            // Append admissible intersection to _IntersectionChain_
            if (ptr->si.m_primitive->getMaterial() == this->m_material) {
                IntersectionChain* next = ARENA_ALLOC(arena, IntersectionChain)();
                ptr->next = next;
                ptr = next;
                nFound++;
            }
        }

        // Randomly choose one of several intersections during BSSRDF sampling
        if (nFound == 0) return Spectrum(0.0f);
        int selected = std::clamp((int)(u1 * nFound), 0, nFound - 1);
        while (selected-- > 0) chain = chain->next;
        *si = chain->si;

        // Compute sample PDF and return the spatial BSSRDF term $\Sp$
        *pdf = this->Pdf_Sp(*si) / nFound;
        return this->Sp(*si);
    }

    float SeparableBSSRDF::Pdf_Sp(const SurfaceInteraction& si) const
    {
        // Express $\pti-\pto$ and $\bold{n}_i$ with respect to local coordinates at
    // $\pto$
        Vector3f d = m_po.m_p - si.m_p;
        Vector3f dLocal(Dot(m_ss, d), Dot(m_ts, d), Dot(m_ns, d));
        Vector3f nLocal(Dot(m_ss, si.m_n), Dot(m_ts, si.m_n), Dot(m_ns, si.m_n));

        // Compute BSSRDF profile radius under projection along each axis
        float rProj[3] = { std::sqrt(dLocal.y * dLocal.y + dLocal.z * dLocal.z),
                           std::sqrt(dLocal.z * dLocal.z + dLocal.x * dLocal.x),
                           std::sqrt(dLocal.x * dLocal.x + dLocal.y * dLocal.y) };

        // Return combined probability from all BSSRDF sampling strategies
        float pdf = 0, axisProb[3] = { .25f, .25f, .5f };
        float chProb = 1 / (float)Spectrum::nSamples;
        for (int axis = 0; axis < 3; ++axis)
            for (int ch = 0; ch < Spectrum::nSamples; ++ch)
                pdf += Pdf_Sr(ch, rProj[axis]) * std::abs(nLocal[axis]) * chProb *
                axisProb[axis];
        return pdf;
    }

    BSSRDFTable::BSSRDFTable(int nRhoSamples, int nRadiusSamples)
        : m_nRhoSamples(nRhoSamples)
        , m_nRadiusSamples(nRadiusSamples)
        , m_rhoSamples(new float[nRhoSamples])
        , m_radiusSamples(new float[nRadiusSamples])
        , m_profile(new float[nRadiusSamples * nRhoSamples])
        , m_rhoEff(new float[nRhoSamples])
        , m_profileCDF(new float[nRadiusSamples * nRhoSamples])
    {

    }

#pragma endregion
	
	
    // BxDF Utility Functions
    float FrDielectric(float cosThetaI, float etaI, float etaT) {
        cosThetaI = std::clamp(cosThetaI, -1.0f, 1.0f);
        // Potentially swap indices of refraction
        bool entering = cosThetaI > 0.f;
        if (!entering) {
            std::swap(etaI, etaT);
            cosThetaI = std::abs(cosThetaI);
        }

        // Compute _cosThetaT_ using Snell's law
        float sinThetaI = std::sqrt(std::max((float)0, 1 - cosThetaI * cosThetaI));
        float sinThetaT = etaI / etaT * sinThetaI;

        // Handle total internal reflection
        if (sinThetaT >= 1) 
			return 1.0f;
        float cosThetaT = std::sqrt(std::max((float)0, 1 - sinThetaT * sinThetaT));
        float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) /
                      ((etaT * cosThetaI) + (etaI * cosThetaT));
        float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) /
                      ((etaI * cosThetaI) + (etaT * cosThetaT));
        return (Rparl * Rparl + Rperp * Rperp) / 2;
    }


    Spectrum FrConductor(float cosThetaI, const Spectrum& etai,
        const Spectrum& etat, const Spectrum& k) 
	{
        
            cosThetaI = std::clamp(cosThetaI, -1.0f, 1.0f);
            Spectrum eta = etat / etai;
            Spectrum etak = k / etai;

            float cosThetaI2 = cosThetaI * cosThetaI;
            float sinThetaI2 = 1. - cosThetaI2;
            Spectrum eta2 = eta * eta;
            Spectrum etak2 = etak * etak;

            Spectrum t0 = eta2 - etak2 - sinThetaI2;
            Spectrum a2plusb2 = Sqrt(t0 * t0 + 4 * eta2 * etak2);
            Spectrum t1 = a2plusb2 + cosThetaI2;
            Spectrum a = Sqrt(0.5f * (a2plusb2 + t0));
            Spectrum t2 = (float)2 * cosThetaI * a;
            Spectrum Rs = (t1 - t2) / (t1 + t2);

            Spectrum t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
            Spectrum t4 = t2 * sinThetaI2;
            Spectrum Rp = Rs * (t3 - t4) / (t3 + t4);

            return 0.5f * (Rp + Rs);
	}

#pragma region FourierMaterial
   

#pragma endregion

	Vector3f BRDFRemap(const Vector3f& wo, const Vector3f& wi)
	{
		float cosi = CosTheta(wi), coso = CosTheta(wo);
		float sini = SinTheta(wi), sino = SinTheta(wo);
		float phii = SphericalPhi(wi), phio = SphericalPhi(wo);

		float dphi = phii - phio;
		if (dphi < 0.) dphi += 2.f * PI;
		if (dphi > 2.f * PI) dphi -= 2.f * PI;
		if (dphi > PI) dphi = 2.f * PI - dphi;
		return Vector3f(sini * sino, dphi / PI, cosi * coso);
	}


	bool BxDF::matchesFlags(eBxDFType _flags) const
	{
		return (m_type & _flags) == m_type;
	}

	Spectrum BxDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* _pdf, eBxDFType* _type ) const
	{
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u);
		if (wo.z < 0.) wi->z *= -1.f;
		*_pdf = pdf(wo, *wi);
		return f(wo, *wi);
	}

	Spectrum BxDF::rho(const Vector3f& wo, int nSamples, const Vector2f* samples ) const
	{
		Spectrum r = 0.;
		for (int i = 0; i < nSamples; ++i) {
			// Estimate one term of $\rho_\roman{hd}$
			Vector3f wi;
			float pdf = 0.f;
			Spectrum f = sample_f(wo, &wi, samples[i], &pdf, nullptr);
			if (pdf > 0.) 
                r += f * AbsCosTheta(wi) / pdf;
		}
		return r / float(nSamples);
	}

	Spectrum BxDF::rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const
	{
		Spectrum r = 0.;
		for (int i = 0; i < nSamples; ++i) {
			// Estimate one term of $\rho_\roman{hh}$
			Vector3f wo, wi;
			wo = UniformSampleHemisphere(samples1[i]);
			float pdf_o = UniformHemispherePdf(),
                  pdf_i = 0.f;
			Spectrum f = sample_f(wo, &wi, samples2[i], &pdf_i, nullptr );
			if (pdf_i > 0.)
				r += f * AbsCosTheta(wi) * AbsCosTheta(wo) / (pdf_o * pdf_i);
		}
		return r / (PI * nSamples);
	}

	float BxDF::pdf(const Vector3f& wo, const Vector3f& wi) const
	{
		return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * INV_PI : 0.f;
	}


	BRDFToBTDF::BRDFToBTDF(BxDF* _brdf) 
		: BxDF(eBxDFType(_brdf->m_type ^ (BSDF_ALL_REFLECTION | BSDF_ALL_TRANSMISSION)))
		, m_brdf(_brdf)
	{

	}

	Spectrum BRDFToBTDF::f(const Vector3f& wo, const Vector3f& wi) const
	{
		return m_brdf->f(wo, GetOtherHemisphere(wi));
	}


	Spectrum BRDFToBTDF::sample_f(const Vector3f& wo, Vector3f* wi,  const Vector2f& u, float* pdf, eBxDFType* _type ) const
	{
        Spectrum f = m_brdf->sample_f(wo, wi, u, pdf, _type );
        *wi = GetOtherHemisphere(*wi);
        return f;
	}

	Spectrum BRDFToBTDF::rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const
	{
		return m_brdf->rho(GetOtherHemisphere(wo), nSamples, samples);
	}

	Spectrum BRDFToBTDF::rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const
	{
		return m_brdf->rho(nSamples, samples1, samples2);
	}

	float BRDFToBTDF::pdf(const Vector3f& wi, const Vector3f& wo) const
	{
		return m_brdf->pdf(wo, GetOtherHemisphere(wi));
	}


	ScaledBxDF::ScaledBxDF(BxDF* _brdf, const Spectrum& s) : BxDF(_brdf->m_type)
		, m_spectrum(s)
		, m_brdf(_brdf)
	{

	}

	Spectrum ScaledBxDF::f(const Vector3f& wo, const Vector3f& wi) const
	{
		return m_spectrum * m_brdf->f(wo, wi);
	}

	Spectrum ScaledBxDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const
	{
		return m_spectrum * m_brdf->sample_f(wo, wi, u, pdf, _type );
	}

	Spectrum ScaledBxDF::rho(const Vector3f& wo, int nSamples, const  Vector2f* samples) const
	{
		return m_spectrum * m_brdf->rho(wo, nSamples, samples);
	}

	Spectrum ScaledBxDF::rho(int nSamples, const  Vector2f* samples1, const  Vector2f* samples2) const
	{
		return m_spectrum * m_brdf->rho(nSamples, samples1, samples2);
	}

	FresnelConductor::FresnelConductor(const Spectrum& _etaI, const Spectrum& _etaT, const Spectrum& _k)
		 : m_etaI(_etaI)
		 , m_etaT(_etaT)		
		 , m_k(_k)
	{
	}

	Spectrum FresnelConductor::evaluate(float cosi) const
	{
		return FrConductor(std::abs(cosi), m_etaI, m_etaT, m_k);
	}

	FresnelDielectric::FresnelDielectric(float _etai, float _etat) : m_etai(_etai)
		, m_etat(_etat)
	{
	}

	Spectrum FresnelDielectric::evaluate(float cosi) const
	{
		
	    return FrDielectric(cosi, m_etai, m_etat );
	
	}

	SpecularReflection::SpecularReflection(const Spectrum& _spectrum, Fresnel* _pFresnel)
		: BxDF(eBxDFType(BSDF_REFLECTION | BSDF_SPECULAR))
		, m_spectrum(_spectrum)
		, m_pFresnel(_pFresnel)
	{

	}

	Spectrum SpecularReflection::f(const Vector3f& wo, const Vector3f& wi) const
	{
		return 0.0f;
	}

	Spectrum SpecularReflection::sample_f(const  Vector3f& wo, Vector3f* wi, const Vector2f&, float* pdf, eBxDFType* _type) const
	{
		*pdf = 1.0f;
		*wi = Vector3f(-wo.x, -wo.y, wo.z);
		return m_pFresnel->evaluate(CosTheta(wo)) * m_spectrum / AbsCosTheta(*wi);
	}

    float SpecularReflection::pdf(const Vector3f& wi, const Vector3f& wo) const
    {
        UNUSED(wi)
        UNUSED(wo)
        
        return 0.f;
    }

    SpecularTransmission::SpecularTransmission(const Spectrum& _spectrum, float _etai, float _etat, eTransportMode _mode)
		: BxDF(eBxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR))
		, m_spectrumTransmision( _spectrum )
		, m_etai( _etai )
		, m_etat( _etat )
		, m_fresnel( FresnelDielectric( _etai, _etat ) )
		, m_mode( _mode )
	{

	}

	Spectrum SpecularTransmission::f(const Vector3f& wo, const Vector3f& wi) const
	{
		return Spectrum(0.0f);
	}

	Spectrum SpecularTransmission::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type ) const
	{
		// Figure out which $\eta$ is incident and which is transmitted
         // Figure out which $\eta$ is incident and which is transmitted
        bool entering = CosTheta(wo) > 0;
        float etaI = entering ? m_etai : m_etat;
        float etaT = entering ? m_etat : m_etai;

        // Compute ray direction for specular transmission
        if (!Refract(wo, FaceForward(Vector3f(0, 0, 1), wo), etaI / etaT, wi))
            return 0.f;
        *pdf = 1;
        Spectrum ft = m_spectrumTransmision * (Spectrum(1.) - m_fresnel.evaluate(CosTheta(*wi)));
        // Account for non-symmetry with transmission to different medium
        if (m_mode == eTransportMode::TRANSPORTMODE_RADIANCE) 
			ft *= (etaI * etaI) / (etaT * etaT);
        return ft / AbsCosTheta(*wi);
	}

    float SpecularTransmission::pdf(const Vector3f& wi, const Vector3f& wo) const
    {
        return 0.f;
    }

    Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const
	{
		return m_spectrum * INV_PI;
	}

	Spectrum LambertianReflection::rho(const Vector3f& wo, int nSamples, const Vector2f* samples) const
	{
		return m_spectrum;
	}

	Spectrum LambertianReflection::rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2) const
	{
		return m_spectrum;
	}

	OrenNayer::OrenNayer(const Spectrum& _refl, float _sig) 
		: BxDF(eBxDFType(BSDF_REFLECTION | BSDF_DIFFUSE))
		, m_refl(_refl)
	{
		float sigma  = ToRadians(_sig);		
		float sigma2 = sigma * sigma;

		m_A = 1.f - (sigma2 / (2.f * (sigma2 + 0.33f)));
		m_B = (0.45f * sigma2) / (sigma2 + 0.09f);
	}

	Spectrum OrenNayer::f(const Vector3f& wo, const Vector3f& wi) const
	{
		float sinThetai = SinTheta(wi);
		float sinThetao = SinTheta(wo);

		float maxcos = 0.f;
		if (sinThetai > 1e-4 && sinThetao > 1e-4) {
			float sinphii = SinPhi(wi), cosphii = CosPhi(wi);
			float sinphio = SinPhi(wo), cosphio = CosPhi(wo);
			float dcos = cosphii * cosphio + sinphii * sinphio;
			maxcos = std::max(0.f, dcos);
		}

		// Compute sine and tangent terms of Oren-Nayar model
		float sinalpha, tanbeta;
		if (AbsCosTheta(wi) > AbsCosTheta(wo)) {
			sinalpha = sinThetao;
			tanbeta = sinThetai / AbsCosTheta(wi);
		}
		else {
			sinalpha = sinThetai;
			tanbeta = sinThetao / AbsCosTheta(wo);
		}
		return m_refl * INV_PI * (m_A + m_B * maxcos * sinalpha * tanbeta);
	}

	FresnelBlend::FresnelBlend(const Spectrum& _d, const Spectrum& _s, MicrofacetDistribution* _dist) : BxDF(eBxDFType(BSDF_REFLECTION | BSDF_GLOSSY))
		, m_diffuse(_d)
		, m_spec(_s)
		, m_pDist(_dist)
	{
	}

	Spectrum FresnelBlend::f(const Vector3f& wo, const Vector3f& wi) const
	{
		
        auto pow5 = [](float v) { return (v * v) * (v * v) * v; };
        Spectrum diffuse = (28.f / (23.f * PI)) * m_diffuse * (Spectrum(1.f) - m_spec) *
            (1 - pow5(1 - .5f * AbsCosTheta(wi))) *
            (1 - pow5(1 - .5f * AbsCosTheta(wo)));
        Vector3f wh = wi + wo;
        if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0);
        wh = Normalize(wh);
        Spectrum specular =
            m_pDist->D(wh) /
            (4 * AbsDot(wi, wh) * std::max(AbsCosTheta(wi), AbsCosTheta(wo))) *
            SchlickFresnel(Dot(wi, wh));
        return diffuse + specular;
	}

	Spectrum FresnelBlend::SchlickFresnel(float _cosTheta) const
	{
        auto pow5 = [](float v) { return (v * v) * (v * v) * v; };
        return m_spec + pow5(1 - _cosTheta) * (Spectrum(1.) - m_spec);
	}



    Spectrum FresnelBlend::sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& _u, float* pdf, eBxDFType* sampledType) const
    {
        Point2f u = _u;
        if (u[0] < .5) {
            u[0] = std::min(2 * u[0], OneMinusEpsilon);
            // Cosine-sample the hemisphere, flipping the direction if necessary
            *wi = CosineSampleHemisphere(u);
            if (wo.z < 0) wi->z *= -1;
        }
        else {
            u[0] = std::min(2 * (u[0] - .5f), OneMinusEpsilon);
            // Sample microfacet orientation $\wh$ and reflected direction $\wi$
            Vector3f wh = m_pDist->Sample_wh(wo, u);
            *wi = Reflect(wo, wh);
            if (!SameHemisphere(wo, *wi)) return Spectrum(0.f);
        }
        *pdf = this->pdf(wo, *wi);
        return f(wo, *wi);
    }

    float FresnelBlend::pdf(const Vector3f& wo, const Vector3f& wi) const
    {
        if (!SameHemisphere(wo, wi)) return 0;
        Vector3f wh = Normalize(wo + wi);
        float pdf_wh = m_pDist->Pdf(wo, wh);
        return .5f * (AbsCosTheta(wi) * INV_PI + pdf_wh / (4 * Dot(wo, wh)));
    }



    BSDF::BSDF(const SurfaceInteraction& si, float _eta)
        : m_eta(_eta)
        , m_normalShading(si.shading.m_n)
        , m_normalGeom(si.m_n)
        , m_sTang(Normalize(si.shading.m_dpdu))
        , m_tTang(Cross(m_normalShading, m_sTang )) 
		, m_numBxDFS(0)
    {

    }

    void BSDF::addBxDF(BxDF* _pBxDF)
	{
		assert(m_numBxDFS < MAX_BxDFS);
		m_pBxDFS[m_numBxDFS++] = _pBxDF;
	}


	int BSDF::numComponents(eBxDFType _type) const
	{
		int retVal = 0;
		for (int i = 0; i < m_numBxDFS; ++i)
		{
			if (m_pBxDFS[i]->matchesFlags(_type))
				retVal++;
		}
		return retVal;
	}

	Vector3f BSDF::worldToLocal(const Vector3f& _v) const
	{
		return Vector3f(   Dot( _v, m_sTang), Dot( _v, m_tTang), Dot( _v, m_normalGeom));
	}

	Vector3f BSDF::localToWorld(const Vector3f& _v) const
	{
		return Vector3f(m_sTang.x * _v.x + m_tTang.x * _v.y + m_normalGeom.x * _v.z,
					    m_sTang.y * _v.x + m_tTang.y * _v.y + m_normalGeom.y * _v.z,
					    m_sTang.z * _v.x + m_tTang.z * _v.y + m_normalGeom.z * _v.z);
	}


    Spectrum BSDF::sample_f(const Vector3f& woWorld, Vector3f* wiWorld, const Vector2f& u, float* pdf, eBxDFType type /*= BSDF_ALL*/, eBxDFType* sampledType /*= NULL */) const
    {
        // Choose which _BxDF_ to sample
        int matchingComps = numComponents(type);
        if (matchingComps == 0) {
            *pdf = 0;
            if (sampledType) 
				*sampledType = eBxDFType(0);
            return Spectrum(0.f);
        }
        int comp =
            std::min((int)std::floor(u[0] * matchingComps), matchingComps - 1);

        // Get _BxDF_ pointer for chosen component
        BxDF* bxdf = nullptr;
        int count = comp;
        for (int i = 0; i < m_numBxDFS; ++i)
            if (m_pBxDFS[i]->matchesFlags(type) && count-- == 0) {
                bxdf = m_pBxDFS[i];
                break;
            }
        assert(bxdf != nullptr);
       // VLOG(2) << "BSDF::Sample_f chose comp = " << comp << " / matching = " <<
        //    matchingComps << ", bxdf: " << bxdf->ToString();

        // Remap _BxDF_ sample _u_ to $[0,1)^2$
        Vector2f uRemapped(std::min(u[0] * matchingComps - comp, OneMinusEpsilon), u[1]);

        // Sample chosen _BxDF_
		Vector3f wi,
			wo = worldToLocal(woWorld);//WorldToLocal(woWorld);
        if (wo.z == 0) 
			return 0.;
        *pdf = 0;
        if (sampledType) 
			*sampledType = bxdf->m_type;
        Spectrum f = bxdf->sample_f(wo, &wi, uRemapped, pdf, sampledType);
        /* VLOG(2) << "For wo = " << wo << ", sampled f = " << f << ", pdf = "
             << *pdf << ", ratio = " << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.))
             << ", wi = " << wi;*/
        if (*pdf == 0) {
            if (sampledType) 
				*sampledType = eBxDFType(0);
			return Spectrum(0.f);
        }
        *wiWorld = localToWorld(wi);

        // Compute overall PDF with all matching _BxDF_s
        if (!(bxdf->m_type & BSDF_SPECULAR) && matchingComps > 1)
            for (int i = 0; i < m_numBxDFS; ++i)
                if (m_pBxDFS[i] != bxdf && m_pBxDFS[i]->matchesFlags(type))
                    *pdf += m_pBxDFS[i]->pdf(wo, wi);
        if (matchingComps > 1) *pdf /= matchingComps;

        // Compute value of BSDF for sampled direction
        if (!(bxdf->m_type & BSDF_SPECULAR)) {
            bool reflect = Dot(*wiWorld, m_normalGeom) * Dot(woWorld, m_normalGeom) > 0;
            f = 0.;
            for (int i = 0; i < m_numBxDFS; ++i)
                if (m_pBxDFS[i]->matchesFlags(type) &&
                    ((reflect && (m_pBxDFS[i]->m_type & BSDF_REFLECTION)) ||
                        (!reflect && (m_pBxDFS[i]->m_type & BSDF_TRANSMISSION))))
                    f += m_pBxDFS[i]->f(wo, wi);
        }
        /* VLOG(2) << "Overall f = " << f << ", pdf = " << *pdf << ", ratio = "
             << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.));*/
        return f;
    }

    float BSDF::Pdf(const Vector3f& _wo, const Vector3f& _wi, eBxDFType _flags /*= BSDF_ALL */) const
    {
		if (m_numBxDFS == 0) return 0.f;
       
        Vector3f wo = worldToLocal(_wo), 
			     wi = worldToLocal(_wi);
        float pdf = 0.f;
        int matchingComps = 0;
		for (int i = 0; i < m_numBxDFS; ++i) {
			if (m_pBxDFS[i]->matchesFlags(_flags)) {
				++matchingComps;
				pdf += m_pBxDFS[i]->pdf(wo, wi);
			}
		}
        float v = matchingComps > 0 ? pdf / matchingComps : 0.f;
        return v;
    }

    Spectrum BSDF::f(const Vector3f& woW, const Vector3f& wiW, eBxDFType flags /*= BSDF_ALL*/) const
    {
        Vector3f wi = worldToLocal(wiW), 
			     wo = worldToLocal(woW);
        if (wo.z == 0) return 0.;
        bool reflect = Dot(wiW, m_normalGeom) * Dot(woW, m_normalGeom) > 0;
        Spectrum f(0.f);
        for (int i = 0; i < m_numBxDFS; ++i)
            if (m_pBxDFS[i]->matchesFlags(flags) &&
                ((reflect && (m_pBxDFS[i]->m_type & BSDF_REFLECTION)) ||
                    (!reflect && (m_pBxDFS[i]->m_type & BSDF_TRANSMISSION))))
                f += m_pBxDFS[i]->f(wo, wi);
        return f;
    }

    Spectrum BSDF::rho(int nSamples, const Vector2f* samples1, const Vector2f* samples2, eBxDFType flags) const
    {
        Spectrum ret(0.f);
        for (int i = 0; i < m_numBxDFS; ++i)
            if (m_pBxDFS[i]->matchesFlags(flags))
                ret += m_pBxDFS[i]->rho( nSamples, samples1, samples2);
        return ret;
    }

    Spectrum BSDF::rho(const Vector3f& woWorld, int nSamples, const Vector2f* samples, eBxDFType flags) const
    {
        Vector3f wo = worldToLocal(woWorld);
        Spectrum ret(0.f);
        for (int i = 0; i < m_numBxDFS; ++i)
            if (m_pBxDFS[i]->matchesFlags(flags))
                ret += m_pBxDFS[i]->rho(wo, nSamples, samples);
        return ret;
    }


    MicrofacetTransmission::MicrofacetTransmission(const Spectrum& T, MicrofacetDistribution* distribution, float etaA, float etaB, eTransportMode mode) : BxDF(eBxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY)),
        m_T(T),
        m_distribution(distribution),
        m_etaA(etaA),
        m_etaB(etaB),
        m_fresnel(etaA, etaB),
        m_mode(mode)
    {

    }

    Spectrum MicrofacetTransmission::f(const Vector3f& wo, const Vector3f& wi) const
    {
        if (SameHemisphere(wo, wi)) return Spectrum(0.f); // transmission only

        float cosThetaO = CosTheta(wo);
        float cosThetaI = CosTheta(wi);
        if (cosThetaI == 0 || cosThetaO == 0) return Spectrum(0.f);

        // Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
        float eta = CosTheta(wo) > 0 ? (m_etaB / m_etaA) : (m_etaA / m_etaB);
        Vector3f wh = Normalize(wo + wi * eta);
        if (wh.z < 0) wh = -wh;

        // Same side?
        if (Dot(wo, wh) * Dot(wi, wh) > 0) return Spectrum(0.f);

        Spectrum F = m_fresnel.evaluate(Dot(wo, wh));

        float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
        float factor = (m_mode == eTransportMode::TRANSPORTMODE_RADIANCE) ? (1 / eta) : 1;

        return (Spectrum(1.f) - F) * m_T *
            std::abs(m_distribution->D(wh) * m_distribution->G(wo, wi) * eta * eta *
                AbsDot(wi, wh) * AbsDot(wo, wh) * factor * factor /
                (cosThetaI * cosThetaO * sqrtDenom * sqrtDenom));
    }

    Spectrum MicrofacetTransmission::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const
    {
        if (wo.z == 0) return 0.f;
        Vector3f wh = m_distribution->Sample_wh(wo, u);
        if (Dot(wo, wh) < 0) return 0.f;  // Should be rare

        float eta = CosTheta(wo) > 0 ? (m_etaA / m_etaB) : (m_etaB / m_etaA);
        if (!Refract(wo, wh, eta, wi)) 
			return 0.f;
        *pdf = this->pdf(wo, *wi);
        return f(wo, *wi);
    }
    

    float MicrofacetTransmission::pdf(const Vector3f& wo, const Vector3f& wi) const
    {
        if (SameHemisphere(wo, wi)) return 0;
        // Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
		float eta = CosTheta(wo) > 0 ? (m_etaA / m_etaB) : (m_etaB / m_etaA);
        Vector3f wh = Normalize(wo + wi * eta);

        if (Dot(wo, wh) * Dot(wi, wh) > 0) return 0;

        // Compute change of variables _dwh\_dwi_ for microfacet transmission
        float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
        float dwh_dwi =
            std::abs((eta * eta * Dot(wi, wh)) / (sqrtDenom * sqrtDenom));
        return m_distribution->Pdf(wo, wh) * dwh_dwi;
    }

    MicrofacetReflection::MicrofacetReflection(const Spectrum& R, MicrofacetDistribution* distribution, Fresnel* fresnel) 
		: BxDF(eBxDFType(BSDF_REFLECTION | BSDF_GLOSSY))
        , m_R(R)
        , m_distribution(distribution)
        , m_fresnel(fresnel)
    {

    }

    Spectrum MicrofacetReflection::f(const Vector3f& wo, const Vector3f& wi) const
    {
        float cosThetaO = AbsCosTheta(wo), 
			  cosThetaI = AbsCosTheta(wi);
        Vector3f wh = wi + wo;
        // Handle degenerate cases for microfacet reflection
        if (cosThetaI == 0 || cosThetaO == 0) 
			return Spectrum(0.f);
        if (wh.x == 0 && wh.y == 0 && wh.z == 0) 
			return Spectrum(0.f);
        wh = Normalize(wh);
        // For the Fresnel call, make sure that wh is in the same hemisphere
        // as the surface normal, so that TIR is handled correctly.
        Spectrum F = m_fresnel->evaluate(Dot(wi, FaceForward(wh, Vector3f(0, 0, 1))));
        return m_R * m_distribution->D(wh) * m_distribution->G(wo, wi) * F /
            (4 * cosThetaI * cosThetaO);
    }

    Spectrum MicrofacetReflection::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const
    {
        // Sample microfacet orientation $\wh$ and reflected direction $\wi$
        if (wo.z == 0) return 0.;
        Vector3f wh = m_distribution->Sample_wh(wo, u);
        if (Dot(wo, wh) < 0) return 0.;   // Should be rare
        *wi = Reflect(wo, wh);
        if (!SameHemisphere(wo, *wi)) return Spectrum(0.f);

        // Compute PDF of _wi_ for microfacet reflection
        *pdf = m_distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
        return f(wo, *wi);
    }

    float MicrofacetReflection::pdf(const Vector3f& wo, const Vector3f& wi) const
    {
        if (!SameHemisphere(wo, wi)) return 0;
        Vector3f wh = Normalize(wo + wi);
        return m_distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
    }


    Spectrum FresnelSpecular::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const
    {
        float F = FrDielectric(CosTheta(wo), m_etaA, m_etaB);
        if (u[0] < F) {
            // Compute specular reflection for _FresnelSpecular_

            // Compute perfect specular reflection direction
            *wi = Vector3f(-wo.x, -wo.y, wo.z);
            if (sampledType)
                *sampledType = eBxDFType(BSDF_SPECULAR | BSDF_REFLECTION);
            *pdf = F;
            return F * m_T / AbsCosTheta(*wi);
        }
        else {
            // Compute specular transmission for _FresnelSpecular_

            // Figure out which $\eta$ is incident and which is transmitted
            bool entering = CosTheta(wo) > 0;
            float etaI = entering ? m_etaA : m_etaB;
            float etaT = entering ? m_etaB : m_etaA;

            // Compute ray direction for specular transmission
            if (!Refract(wo, FaceForward(Vector3f(0, 0, 1), wo), etaI / etaT, wi))
                return 0.f;
            Spectrum ft = m_T * (1 - F);

            // Account for non-symmetry with transmission to different medium
            if (m_mode == eTransportMode::TRANSPORTMODE_RADIANCE)
                ft *= (etaI * etaI) / (etaT * etaT);
            if (sampledType)
                *sampledType = eBxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION);
            *pdf = 1 - F;
            return ft / AbsCosTheta(*wi);
        }
    }

    Spectrum LambertianTransmission::f(const Vector3f& wo, const Vector3f& wi) const
    {
		UNUSED(wo)
        UNUSED(wi)
        
        return m_T * INV_PI;
    }

    Spectrum LambertianTransmission::rho(const Vector3f&, int, const Vector2f* sample) const
    {
        UNUSED( sample )
        return m_T;
    }

    Spectrum LambertianTransmission::rho(int, const Vector2f* samples1, const Vector2f* samples2) const
    {
        return m_T;
    }

    Spectrum LambertianTransmission::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* sampledType) const
    {
        *wi = CosineSampleHemisphere(u);
        if (wo.z > 0) wi->z *= -1;
        *pdf = this->pdf(wo, *wi);
        return f(wo, *wi);
    }

    float LambertianTransmission::pdf(const Vector3f& wo, const Vector3f& wi) const
    {
		return !SameHemisphere(wo, wi) ? AbsCosTheta(wi) * INV_PI : 0;
    }

   

    FourierBSDF::FourierBSDF(const FourierBSDFTable& bsdfTable, eTransportMode mode) : BxDF(eBxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY)),
        m_bsdfTable(bsdfTable),
        m_mode(mode)
    {

    }

    Spectrum FourierBSDF::f(const Vector3f& wo, const Vector3f& wi) const
    {
        // Find the zenith angle cosines and azimuth difference angle
        float muI = CosTheta(-wi), muO = CosTheta(wo);
        float cosPhi = CosDPhi(-wi, wo);

        // Compute Fourier coefficients $a_k$ for $(\mui, \muo)$

        // Determine offsets and weights for $\mui$ and $\muo$
        int offsetI, offsetO;
        float weightsI[4], weightsO[4];
        if (!m_bsdfTable.GetWeightsAndOffset(muI, &offsetI, weightsI) ||
            !m_bsdfTable.GetWeightsAndOffset(muO, &offsetO, weightsO))
            return Spectrum(0.f);

        // Allocate storage to accumulate _ak_ coefficients
        float* ak = ALLOCA(float, m_bsdfTable.header.mMax * m_bsdfTable.header.nChannels);
        memset(ak, 0, m_bsdfTable.header.mMax * m_bsdfTable.header.nChannels * sizeof(float));

        // Accumulate weighted sums of nearby $a_k$ coefficients
        int mMax = 0;
        for (int b = 0; b < 4; ++b) {
            for (int a = 0; a < 4; ++a) {
                // Add contribution of _(a, b)_ to $a_k$ values
                float weight = weightsI[a] * weightsO[b];
                if (weight != 0) {
                    int m;
                    const float* ap = m_bsdfTable.GetAk(offsetI + a, offsetO + b, &m);
                    mMax = std::max(mMax, m);
                    for (int c = 0; c < m_bsdfTable.header.nChannels; ++c)
                        for (int k = 0; k < m; ++k)
                            ak[c * m_bsdfTable.header.mMax + k] += weight * ap[c * m + k];
                }
            }
        }

        // Evaluate Fourier expansion for angle $\phi$
        float Y = std::max((float)0, Fourier(ak, mMax, cosPhi));
        float scale = muI != 0 ? (1 / std::abs(muI)) : (float)0;

        // Update _scale_ to account for adjoint light transport
        if (m_mode == eTransportMode::TRANSPORTMODE_RADIANCE && muI * muO > 0) {
            float eta = muI > 0 ? 1 / m_bsdfTable.header.eta : m_bsdfTable.header.eta;
            scale *= eta * eta;
        }
        if (m_bsdfTable.header.nChannels == 1)
            return Spectrum(Y * scale);
        else {
            // Compute and return RGB colors for tabulated BSDF
            float R = Fourier(ak + 1 *  m_bsdfTable.header.mMax, mMax, cosPhi);
            float B = Fourier(ak + 2 *  m_bsdfTable.header.mMax, mMax, cosPhi);
            float G = 1.39829f * Y - 0.100913f * B - 0.297375f * R;
            float rgb[3] = { R * scale, G * scale, B * scale };
            return Spectrum::FromRGB(rgb).Clamp();
        }
    }

    Spectrum FourierBSDF::sample_f(const Vector3f& wo, Vector3f* wi, const Vector2f& u, float* pdf, eBxDFType* _type) const
    {
        float muO = CosTheta(wo);
        float pdfMu;
        float muI = SampleCatmullRom2D(
            m_bsdfTable.header.nMu, m_bsdfTable.header.nMu, m_bsdfTable.mu.data(),
            m_bsdfTable.mu.data(), m_bsdfTable.a0.data(), m_bsdfTable.cdf.data(),
            muO, u[1], nullptr, &pdfMu);

        // Compute Fourier coefficients $a_k$ for $(\mui, \muo)$

        // Determine offsets and weights for $\mui$ and $\muo$
        int offsetI, offsetO;
        float weightsI[4], weightsO[4];
        if (!m_bsdfTable.GetWeightsAndOffset(muI, &offsetI, weightsI) ||
            !m_bsdfTable.GetWeightsAndOffset(muO, &offsetO, weightsO))
            return Spectrum(0.f);

        // Allocate storage to accumulate _ak_ coefficients
        float* ak = ALLOCA(float, m_bsdfTable.header.mMax * m_bsdfTable.header.nChannels);
        memset(ak, 0, m_bsdfTable.header.mMax * m_bsdfTable.header.nChannels * sizeof(float));

        // Accumulate weighted sums of nearby $a_k$ coefficients
        int mMax = 0;
        for (int b = 0; b < 4; ++b) {
            for (int a = 0; a < 4; ++a) {
                // Add contribution of _(a, b)_ to $a_k$ values
                float weight = weightsI[a] * weightsO[b];
                if (weight != 0) {
                    int m;
                    const float* ap = m_bsdfTable.GetAk(offsetI + a, offsetO + b, &m);
                    mMax = std::max(mMax, m);
                    for (int c = 0; c < m_bsdfTable.header.nChannels; ++c)
                        for (int k = 0; k < m; ++k)
                            ak[c * m_bsdfTable.header.mMax + k] += weight * ap[c * m + k];
                }
            }
        }

        // Importance sample the luminance Fourier expansion
        float phi, pdfPhi;
        float Y = SampleFourier(ak, m_bsdfTable.recip.data(), mMax, u[0], &pdfPhi, &phi);
        *pdf = std::max((float)0, pdfPhi * pdfMu);

        // Compute the scattered direction for _FourierBSDF_
        float sin2ThetaI = std::max((float)0, 1 - muI * muI);
        float norm = std::sqrt(sin2ThetaI / Sin2Theta(wo));
        if (std::isinf(norm)) norm = 0;
        float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
        *wi = -Vector3f(norm * (cosPhi * wo.x - sinPhi * wo.y),
            norm * (sinPhi * wo.x + cosPhi * wo.y), muI);

        // Mathematically, wi will be normalized (if wo was). However, in
        // practice, floating-point rounding error can cause some error to
        // accumulate in the computed value of wi here. This can be
        // catastrophic: if the ray intersects an object with the FourierBSDF
        // again and the wo (based on such a wi) is nearly perpendicular to the
        // surface, then the wi computed at the next intersection can end up
        // being substantially (like 4x) longer than normalized, which leads to
        // all sorts of errors, including negative spectral values. Therefore,
        // we normalize again here.
        * wi = Normalize(*wi);

        // Evaluate remaining Fourier expansions for angle $\phi$
        float scale = muI != 0 ? (1 / std::abs(muI)) : (float)0;
        if (m_mode == eTransportMode::TRANSPORTMODE_RADIANCE && muI * muO > 0) {
            float eta = muI > 0 ? 1 / m_bsdfTable.header.eta : m_bsdfTable.header.eta;
            scale *= eta * eta;
        }

        if (m_bsdfTable.header.nChannels == 1) return Spectrum(Y * scale);
        float R = Fourier(ak + 1 * m_bsdfTable.header.mMax, mMax, cosPhi);
        float B = Fourier(ak + 2 * m_bsdfTable.header.mMax, mMax, cosPhi);
        float G = 1.39829f * Y - 0.100913f * B - 0.297375f * R;
        float rgb[3] = { R * scale, G * scale, B * scale };
        return Spectrum::FromRGB(rgb).Clamp();
    }

    float FourierBSDF::pdf(const Vector3f& wo, const Vector3f& wi) const
    {
        // Find the zenith angle cosines and azimuth difference angle
        float muI = CosTheta(-wi), muO = CosTheta(wo);
        float cosPhi = CosDPhi(-wi, wo);

        // Compute luminance Fourier coefficients $a_k$ for $(\mui, \muo)$
        int offsetI, offsetO;
        float weightsI[4], weightsO[4];
        if (!m_bsdfTable.GetWeightsAndOffset(muI, &offsetI, weightsI) ||
            !m_bsdfTable.GetWeightsAndOffset(muO, &offsetO, weightsO))
            return 0;
        float* ak = ALLOCA(float, m_bsdfTable.header.mMax);
        memset(ak, 0, m_bsdfTable.header.mMax * sizeof(float));
        int mMax = 0;
        for (int o = 0; o < 4; ++o) {
            for (int i = 0; i < 4; ++i) {
                float weight = weightsI[i] * weightsO[o];
                if (weight == 0) continue;

                int order;
                const float* coeffs =
                    m_bsdfTable.GetAk(offsetI + i, offsetO + o, &order);
                mMax = std::max(mMax, order);

                for (int k = 0; k < order; ++k) ak[k] += *coeffs++ * weight;
            }
        }

        // Evaluate probability of sampling _wi_
        float rho = 0;
        for (int o = 0; o < 4; ++o) {
            if (weightsO[o] == 0) continue;
            rho += weightsO[o] * m_bsdfTable.cdf[(offsetO + o) * m_bsdfTable.header.nMu + m_bsdfTable.header.nMu - 1] * (2 * PI);
        }
        float Y = Fourier(ak, mMax, cosPhi);
        return (rho > 0 && Y > 0) ? (Y / rho) : 0;
    }

}

