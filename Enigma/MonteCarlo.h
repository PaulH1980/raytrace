#pragma once
#include <algorithm>
#include <memory>
#include <vector>
#include <assert.h>
#include "RNG.h"
#include "MathCommon.h"



namespace RayTrace
{

    // Monte Carlo Utility Declarations
    struct Distribution1D {
        // Distribution1D Public Methods
        Distribution1D(const float* f, int n) {
            count = n;
            func.resize(count);
            cdf.resize(count + 1);
            memcpy(func.data(), f, sizeof(float) * count);
          
            // Compute integral of step function at $x_i$
            cdf[0] = 0.;
            for (int i = 1; i < count + 1; ++i)
                cdf[i] = cdf[i - 1] + func[i - 1] / n;

            // Transform step function integral into CDF
            funcInt = cdf[count];
            if (funcInt == 0.f) {
                for (int i = 1; i < n + 1; ++i)
                    cdf[i] = float(i) / float(n);
            }
            else {
                for (int i = 1; i < n + 1; ++i)
                    cdf[i] /= funcInt;
            }
        }
        ~Distribution1D() {
        
        }

        float SampleContinuous(float u, float* pdf, int* off = NULL) const {
            // Find surrounding CDF segments and _offset_
            auto iter = std::upper_bound(cdf.begin(), cdf.begin() + count + 1, u);
            int offset = std::max(0, int((iter - cdf.begin()) - 1));
            if (off) *off = offset;
            assert(offset < count);
            assert(u >= cdf[offset] && u < cdf[offset + 1]);

            // Compute offset along CDF segment
            float du = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
            assert(!isnan(du));

            // Compute PDF for sampled offset
            if (pdf) *pdf = func[offset] / funcInt;

            // Return $x\in{}[0,1)$ corresponding to sample
            return (offset + du) / count;
        }
        int SampleDiscrete(float u, float* pdf) const {
            // Find surrounding CDF segments and _offset_
            auto iter = std::upper_bound(cdf.begin(), cdf.begin() + count + 1, u);
            int offset = std::max(0, int((iter - cdf.begin()) - 1));
            assert(offset < count);
            assert(u >= cdf[offset] && u < cdf[offset + 1]);
            if (pdf) *pdf = func[offset] / (funcInt * count);
            return offset;
        }

        int Count() const { return func.size(); }

        float DiscretePDF(int index) const {
            assert(index >= 0 && index < Count());
            return func[index] / (funcInt * Count());
        }
    
        friend struct Distribution2D;
        // Distribution1D Private Data
        std::vector<float> func,
                           cdf;       
        float funcInt;
        int count;
    };

    struct Distribution2D {
        // Distribution2D Public Methods
        Distribution2D(const float* data, int nu, int nv);
        ~Distribution2D();
        Vector2f SampleContinuous(const Vector2f& u, float* pdf ) const;

        float Pdf(const Vector2f& uv) const;

        float Pdf(float u, float v) const {
            int iu = std::clamp(Float2Int(u * pConditionalV[0]->count), 0,
                pConditionalV[0]->count - 1);
            int iv = std::clamp(Float2Int(v * pMarginal->count), 0,
                pMarginal->count - 1);
            if (pConditionalV[iv]->funcInt * pMarginal->funcInt == 0.f) return 0.f;
            return (pConditionalV[iv]->func[iu] * pMarginal->func[iv]) /
                (pConditionalV[iv]->funcInt * pMarginal->funcInt);
        }
    private:
        // Distribution2D Private Data
        std::vector<Distribution1D*> pConditionalV;
        Distribution1D* pMarginal;
    };



    Vector2f    ConcentricSampleDisk(const Vector2f& u);
	Vector3f	UniformSampleHemisphere(float u1, float u2);
    Vector3f	UniformSampleHemisphere(const Vector2f& u);
	float		UniformHemispherePdf();
	float		UniformSpherePdf();
    Vector3f	UniformSampleSphere(const Vector2f& u);
	Vector3f	UniformSampleSphere(float u1, float u2);
	float		UniformConePdf(float cosThetaMax);
	void		UniformSampleTriangle(float u1, float u2, float* u, float* v);
    Vector2f    UniformSampleTriangle(const Vector2f& u);
	Vector3f	UniformSampleCone(float u1, float u2, float costhetamax);
	Vector3f	UniformSampleCone(float u1, float u2, float costhetamax,
		const Vector3f& x, const Vector3f& y, const Vector3f& z);

    Vector3f    UniformSampleCone(const Vector2f& u, float costhetamax);


    Vector3f CosineSampleHemisphere(const Vector2f& u);
    float    CosineHemispherePdf(float cosTheta);

    float PowerHeuristic(int nf, float fPdf, int ng, float gPdf);

	// Sampling Function Definitions
	void StratifiedSample1D(float* samp, int nSamples, RNG& rng, bool jitter = true);

	void StratifiedSample2D(float* samp, int nx, int ny, RNG& rng, bool jitter = true);

    void StratifiedSample2D(Vector2f* samp, int nx, int ny, RNG& rng, bool jitter = true);

	template <typename T>
	void Shuffle(T* samp, uint32_t count, uint32_t dims, RNG& rng) {
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t other = i + (rng.randomUInt(count - i) );
			for (uint32_t j = 0; j < dims; ++j)
				std::swap(samp[dims * i + j], samp[dims * other + j]);
		}
	}


	float Sobol2(uint32_t n, uint32_t scramble);


	// Sampling Inline Functions
	void Sample02(uint32_t n, const uint32_t scramble[2],
		float sample[2]);


	float VanDerCorput(uint32_t n, uint32_t scramble);

	//int  LDPixelSampleFloatsNeeded(const Sample* sample, int nPixelSamples);
	//void LDPixelSample(int xPos, int yPos, float shutterOpen, float shutterClose, int nPixelSamples, Sample* samples, float* buf, RNG& rng);
	void LatinHypercube(float* samples, uint32_t nSamples, uint32_t nDim, RNG& rng);

	void LDShuffleScrambled1D(int nSamples, int nPixel,
		float* samples, RNG& rng);

	void LDShuffleScrambled2D(int nSamples, int nPixel,
		float* samples, RNG& rng);

	//double RadicalInverse(int _n, int _base);

}