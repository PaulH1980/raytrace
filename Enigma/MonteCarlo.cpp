#include <cmath>
#include "MathCommon.h"
#include "Sample.h"
#include "Memory.h"
#include "MonteCarlo.h"



namespace RayTrace
{

	void LatinHypercube( float* samples, uint32_t nSamples, uint32_t nDim, RNG& rng)
	{
		// Generate LHS samples along diagonal
		float delta = 1.f / nSamples;
		for (uint32_t i = 0; i < nSamples; ++i)
			for (uint32_t j = 0; j < nDim; ++j)
				samples[nDim * i + j] = std::min((i + (rng.randomFloat())) * delta, OneMinusEpsilon);

		// Permute LHS samples in each dimension
		for (uint32_t i = 0; i < nDim; ++i) {
			for (uint32_t j = 0; j < nSamples; ++j) {
				uint32_t other = j + (rng.randomUInt() % (nSamples - j));
				std::swap(samples[nDim * j + i], samples[nDim * other + i]);
			}
		}
	}

	void LDShuffleScrambled1D(int nSamples, int nPixel, float* samples, RNG& rng)
	{
		uint32_t scramble = rng.randomUInt();
		for (int i = 0; i < nSamples * nPixel; ++i)
			samples[i] = VanDerCorput(i, scramble);
		for (int i = 0; i < nPixel; ++i)
			Shuffle(samples + i * nSamples, nSamples, 1, rng);
		Shuffle(samples, nPixel, nSamples, rng);
	}

	void LDShuffleScrambled2D(int nSamples, int nPixel, float* samples, RNG& rng)
	{
		uint32_t scramble[2] = { rng.randomUInt(), rng.randomUInt() };
		for (int i = 0; i < nSamples * nPixel; ++i)
			Sample02(i, scramble, &samples[2 * i]);
		for (int i = 0; i < nPixel; ++i)
			Shuffle(samples + 2 * i * nSamples, nSamples, 2, rng);
		Shuffle(samples, nPixel, 2 * nSamples, rng);
	}
		

    Point2f ConcentricSampleDisk(const Point2f& u) {
        // Map uniform random numbers to $[-1,1]^2$
        Point2f uOffset = 2.f * u - Vector2f(1, 1);

        // Handle degeneracy at the origin
        if (uOffset.x == 0 && uOffset.y == 0) return Point2f(0, 0);

        // Apply concentric mapping to point
        float theta, r;
        if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
            r = uOffset.x;
            theta = PI_OVER_FOUR * (uOffset.y / uOffset.x);
        }
        else {
            r = uOffset.y;
            theta = PI_OVER_TWO - PI_OVER_FOUR * (uOffset.x / uOffset.y);
        }
        return r * Point2f(std::cos(theta), std::sin(theta));
    }

		

	Vector3f UniformSampleHemisphere(float u1, float u2)
	{
		float z = u1;
		float r = sqrtf(std::max(0.f, 1.f - z * z));
		float phi = 2 * PI * u2;
		float x = r * cosf(phi);
		float y = r * sinf(phi);
		return Vector3f(x, y, z);
	}

    Vector3f UniformSampleHemisphere(const Vector2f& u)
    {
		return UniformSampleHemisphere(u[0], u[1]);
    }

    float UniformHemispherePdf()
	{
		return 1.0f / (PI * 2.0f);
	}

    float UniformSpherePdf()
    {
		return 1.0f / (PI * 4.0f);
    }

    Vector3f UniformSampleSphere(float u1, float u2)
	{
		float z = 1.f - 2.f * u1;
		float r = sqrtf(std::max(0.f, 1.f - z * z));
		float phi = 2.f * PI * u2;
		float x = r * cosf(phi);
		float y = r * sinf(phi);
		return Vector3f(x, y, z);
	}

    Vector3f UniformSampleSphere(const Vector2f& u)
    {
		return UniformSampleSphere(u.x, u.y);
    }

    float UniformConePdf(float cosThetaMax)
	{
		return 1.f / (2.f * PI * (1.f - cosThetaMax));
	}

	

	void UniformSampleTriangle(float u1, float u2, float* u, float* v)
	{
		float su1 = sqrtf(u1);
		*u = 1.f - su1;
		*v = u2 * su1;
	}

    Vector2f UniformSampleTriangle(const Vector2f& u)
    {
		Vector2f ret;
		UniformSampleTriangle(u.x, u.y, &ret.x, &ret.y);
		return ret;
    }

    Vector3f UniformSampleCone(float u1, float u2, float costhetamax)
	{
		float costheta = (1.f - u1) + u1 * costhetamax;
		float sintheta = sqrtf(1.f - costheta * costheta);
		float phi = u2 * 2.f * PI;
		return Vector3f(cosf(phi) * sintheta, sinf(phi) * sintheta, costheta);
	}

	Vector3f UniformSampleCone(float u1, float u2, float costhetamax, const Vector3f& x, const Vector3f& y, const Vector3f& z)
	{
		float costheta = Lerp(u1, costhetamax, 1.f);
		float sintheta = sqrtf(1.f - costheta * costheta);
		float phi = u2 * 2.f * PI;
		return cosf(phi) * sintheta * x + sinf(phi) * sintheta * y + costheta * z;
	}

    Vector3f UniformSampleCone(const Vector2f& u, float costhetamax)
    {
		return UniformSampleCone(u.x, u.y, costhetamax);
    }



    Vector3f CosineSampleHemisphere(const Vector2f& u)
    {
		auto d  = ConcentricSampleDisk(u);
        float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
        return Vector3f(d.x, d.y, z);
    }

    float CosineHemispherePdf(float cosTheta)
    {
        return cosTheta * INV_PI;
    }

    float PowerHeuristic(int nf, float fPdf, int ng, float gPdf)
    {
        float f = nf * fPdf, g = ng * gPdf;
        return (f * f) / (f * f + g * g);
    }

    void StratifiedSample1D(float* samp, int nSamples, RNG& rng, bool jitter)
	{
		float invTot = 1.f / nSamples;
		for (int i = 0; i < nSamples; ++i) {
			float delta = jitter ? rng.randomFloat() : 0.5f;
			*samp++ = std::min((i + delta) * invTot, OneMinusEpsilon);
		}
	}

	void StratifiedSample2D(float* samp, int nx, int ny, RNG& rng, bool jitter )
	{
		float dx = 1.f / nx, dy = 1.f / ny;
		for (int y = 0; y < ny; ++y)
			for (int x = 0; x < nx; ++x) {
				float jx = jitter ? rng.randomFloat() : 0.5f;
				float jy = jitter ? rng.randomFloat() : 0.5f;
				*samp++ = std::min((x + jx) * dx, OneMinusEpsilon);
				*samp++ = std::min((y + jy) * dy, OneMinusEpsilon);
			}
	}

    void StratifiedSample2D(Vector2f* samp, int nx, int ny, RNG& rng, bool jitter)
    {
        float dx = (float)1 / nx, dy = (float)1 / ny;
		for (int y = 0; y < ny; ++y) {
			for (int x = 0; x < nx; ++x) {
				float jx = jitter ? rng.randomFloat(.0f, OneMinusEpsilon) : 0.5f;
				float jy = jitter ? rng.randomFloat(.0f, OneMinusEpsilon) : 0.5f;
				samp->x = std::min((x + jx) * dx, OneMinusEpsilon);
				samp->y = std::min((y + jy) * dy, OneMinusEpsilon);
				++samp;
			}
		}
    }

    float Sobol2(uint32_t n, uint32_t scramble)
	{
		for (uint32_t v = 1 << 31; n != 0; n >>= 1, v ^= v >> 1)
			if (n & 0x1) scramble ^= v;
		return std::min(((scramble >> 8) & 0xffffff) / float(1 << 24), OneMinusEpsilon);
	}

	void Sample02(uint32_t n, const uint32_t scramble[2], float sample[2])
	{
		sample[0] = VanDerCorput(n, scramble[0]);
		sample[1] = Sobol2(n, scramble[1]);
	}

	float VanDerCorput(uint32_t n, uint32_t scramble)
	{
		// Reverse bits of _n_
		n = (n << 16) | (n >> 16);
		n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
		n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
		n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
		n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
		n ^= scramble;
		return std::min(((n >> 8) & 0xffffff) / float(1 << 24), OneMinusEpsilon);
	}

	

    Distribution2D::Distribution2D(const float* data, int nu, int nv)
    {
        pConditionalV.reserve(nv);
        for (int v = 0; v < nv; ++v) {
            // Compute conditional sampling distribution for $\tilde{v}$
            pConditionalV.push_back(new Distribution1D(&data[v * nu], nu));
        }
        // Compute marginal sampling distribution $p[\tilde{v}]$
        std::vector<float> marginalFunc;
        marginalFunc.reserve(nv);
        for (int v = 0; v < nv; ++v)
            marginalFunc.push_back(pConditionalV[v]->funcInt);
        pMarginal = new Distribution1D(&marginalFunc[0], nv);
    }

    Distribution2D::~Distribution2D()
    {
        delete pMarginal;
        for (uint32_t i = 0; i < pConditionalV.size(); ++i)
            delete pConditionalV[i];
    }

    Vector2f Distribution2D::SampleContinuous(const Vector2f& u, float* pdf) const
    {
        float pdfs[2];
        int v;
        float d1 = pMarginal->SampleContinuous(u[1], &pdfs[1], &v);
        float d0 = pConditionalV[v]->SampleContinuous(u[0], &pdfs[0]);
        *pdf = pdfs[0] * pdfs[1];
        return Vector2f(d0, d1);
    }

    float Distribution2D::Pdf(const Vector2f& uv) const
    {
        return Pdf(uv.x, uv.y);
    }

}


