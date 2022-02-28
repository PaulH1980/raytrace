#pragma once
#include <vector>
#include <cmath>
#include <assert.h>
#include <limits>
#include <algorithm>
#include "Defines.h"
#include "MathCommon.h"

namespace RayTrace
{
	
    // Spectrum Utility Declarations
    static const int sampledLambdaStart = 400;
    static const int sampledLambdaEnd = 700;
    static const int nSpectralSamples = 60;
    extern bool SpectrumSamplesSorted(const float* lambda, const float* vals,
        int n);
    extern void SortSpectrumSamples(float* lambda, float* vals, int n);
    extern float AverageSpectrumSamples(const float* lambda, const float* vals,
        int n, float lambdaStart, float lambdaEnd);
    inline void XYZToRGB(const float xyz[3], float rgb[3]) {
        rgb[0] = 3.240479f * xyz[0] - 1.537150f * xyz[1] - 0.498535f * xyz[2];
        rgb[1] = -0.969256f * xyz[0] + 1.875991f * xyz[1] + 0.041556f * xyz[2];
        rgb[2] = 0.055648f * xyz[0] - 0.204043f * xyz[1] + 1.057311f * xyz[2];
    }

    inline void RGBToXYZ(const float rgb[3], float xyz[3]) {
        xyz[0] = 0.412453f * rgb[0] + 0.357580f * rgb[1] + 0.180423f * rgb[2];
        xyz[1] = 0.212671f * rgb[0] + 0.715160f * rgb[1] + 0.072169f * rgb[2];
        xyz[2] = 0.019334f * rgb[0] + 0.119193f * rgb[1] + 0.950227f * rgb[2];
    }

    extern float InterpolateSpectrumSamples(const float* lambda, const float* vals,
        int n, float l);
    extern void Blackbody(const float* lambda, int n, float T, float* Le);
    extern void BlackbodyNormalized(const float* lambda, int n, float T,
        float* vals);

    // Spectral Data Declarations
    static const int nCIESamples = 471;
    extern const float CIE_X[nCIESamples];
    extern const float CIE_Y[nCIESamples];
    extern const float CIE_Z[nCIESamples];
    extern const float CIE_lambda[nCIESamples];
    static const float CIE_Y_integral = 106.856895;
    static const int nRGB2SpectSamples = 32;
    extern const float RGB2SpectLambda[nRGB2SpectSamples];
    extern const float RGBRefl2SpectWhite[nRGB2SpectSamples];
    extern const float RGBRefl2SpectCyan[nRGB2SpectSamples];
    extern const float RGBRefl2SpectMagenta[nRGB2SpectSamples];
    extern const float RGBRefl2SpectYellow[nRGB2SpectSamples];
    extern const float RGBRefl2SpectRed[nRGB2SpectSamples];
    extern const float RGBRefl2SpectGreen[nRGB2SpectSamples];
    extern const float RGBRefl2SpectBlue[nRGB2SpectSamples];
    extern const float RGBIllum2SpectWhite[nRGB2SpectSamples];
    extern const float RGBIllum2SpectCyan[nRGB2SpectSamples];
    extern const float RGBIllum2SpectMagenta[nRGB2SpectSamples];
    extern const float RGBIllum2SpectYellow[nRGB2SpectSamples];
    extern const float RGBIllum2SpectRed[nRGB2SpectSamples];
    extern const float RGBIllum2SpectGreen[nRGB2SpectSamples];
    extern const float RGBIllum2SpectBlue[nRGB2SpectSamples];

    // Spectrum Declarations
    template <int nSpectrumSamples>
    class CoefficientSpectrum {
    public:
        // CoefficientSpectrum Public Methods
        CoefficientSpectrum(float v = 0.f) {
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] = v;
            assert(!HasNaNs());
        }
#ifdef DEBUG
        CoefficientSpectrum(const CoefficientSpectrum& s) {
            DCHECK(!s.HasNaNs());
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] = s.c[i];
        }

        CoefficientSpectrum& operator=(const CoefficientSpectrum& s) {
            DCHECK(!s.HasNaNs());
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] = s.c[i];
            return *this;
        }
#endif  // DEBUG
        void Print(FILE* f) const {
            fprintf(f, "[ ");
            for (int i = 0; i < nSpectrumSamples; ++i) {
                fprintf(f, "%f", c[i]);
                if (i != nSpectrumSamples - 1) fprintf(f, ", ");
            }
            fprintf(f, "]");
        }
        CoefficientSpectrum& operator+=(const CoefficientSpectrum& s2) {
            assert(!s2.HasNaNs());
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] += s2.c[i];
            return *this;
        }
        CoefficientSpectrum operator+(const CoefficientSpectrum& s2) const {
            assert(!s2.HasNaNs());
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] += s2.c[i];
            return ret;
        }
        CoefficientSpectrum operator-(const CoefficientSpectrum& s2) const {
            assert(!s2.HasNaNs());
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] -= s2.c[i];
            return ret;
        }
        CoefficientSpectrum operator/(const CoefficientSpectrum& s2) const {
            assert(!s2.HasNaNs());
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) {
                assert(s2.c[i] != 0);
                ret.c[i] /= s2.c[i];
            }
            return ret;
        }
        CoefficientSpectrum operator*(const CoefficientSpectrum& sp) const {
            assert(!sp.HasNaNs());
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] *= sp.c[i];
            return ret;
        }
        CoefficientSpectrum& operator*=(const CoefficientSpectrum& sp) {
            assert(!sp.HasNaNs());
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] *= sp.c[i];
            return *this;
        }
        CoefficientSpectrum operator*(float a) const {
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] *= a;
            assert(!ret.HasNaNs());
            return ret;
        }
        CoefficientSpectrum& operator*=(float a) {
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] *= a;
            assert(!HasNaNs());
            return *this;
        }
        friend inline CoefficientSpectrum operator*(float a,
            const CoefficientSpectrum& s) {
            assert(!std::isnan(a) && !s.HasNaNs());
            return s * a;
        }
        CoefficientSpectrum operator/(float a) const {
            assert(a != 0);
            assert(!std::isnan(a));
            CoefficientSpectrum ret = *this;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] /= a;
            assert(!ret.HasNaNs());
            return ret;
        }
        CoefficientSpectrum& operator/=(float a) {
            assert(a != 0);
            assert(!std::isnan(a));
            for (int i = 0; i < nSpectrumSamples; ++i) c[i] /= a;
            return *this;
        }
        bool operator==(const CoefficientSpectrum& sp) const {
            for (int i = 0; i < nSpectrumSamples; ++i)
                if (c[i] != sp.c[i]) return false;
            return true;
        }
        bool operator!=(const CoefficientSpectrum& sp) const {
            return !(*this == sp);
        }
        bool IsBlack() const {
            for (int i = 0; i < nSpectrumSamples; ++i)
                if (c[i] != 0.) return false;
            return true;
        }
        friend CoefficientSpectrum Sqrt(const CoefficientSpectrum& s) {
            CoefficientSpectrum ret;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] = std::sqrt(s.c[i]);
            assert(!ret.HasNaNs());
            return ret;
        }
        template <int n>
        friend inline CoefficientSpectrum<n> Pow(const CoefficientSpectrum<n>& s,
            float e);
        CoefficientSpectrum operator-() const {
            CoefficientSpectrum ret;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] = -c[i];
            return ret;
        }
        friend CoefficientSpectrum Exp(const CoefficientSpectrum& s) {
            CoefficientSpectrum ret;
            for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] = std::exp(s.c[i]);
            assert(!ret.HasNaNs());
            return ret;
        }
        friend std::ostream& operator<<(std::ostream& os,
            const CoefficientSpectrum& s) {
            return os << s.ToString();
        }
        std::string ToString() const {
            std::string str = "[ ";
            for (int i = 0; i < nSpectrumSamples; ++i) {
                str += StringPrintf("%f", c[i]);
                if (i + 1 < nSpectrumSamples) str += ", ";
            }
            str += " ]";
            return str;
        }
        CoefficientSpectrum Clamp(float low = 0, float high = InfinityF32) const {
            CoefficientSpectrum ret;
            for (int i = 0; i < nSpectrumSamples; ++i)
                ret.c[i] = std::clamp(c[i], low, high);
            assert(!ret.HasNaNs());
            return ret;
        }
        
        bool HasInfs()
        {
            for (int i = 0; i < nSpectrumSamples; ++i)
                if (std::isinf(c[i])) return true;
            return false;
        }


        float MaxComponentValue() const {
            float m = c[0];
            for (int i = 1; i < nSpectrumSamples; ++i)
                m = std::max(m, c[i]);
            return m;
        }
        bool HasNaNs() const {
            for (int i = 0; i < nSpectrumSamples; ++i)
                if (std::isnan(c[i])) return true;
            return false;
        }
        bool Write(FILE* f) const {
            for (int i = 0; i < nSpectrumSamples; ++i)
                if (fprintf(f, "%f ", c[i]) < 0) return false;
            return true;
        }
        bool Read(FILE* f) {
            for (int i = 0; i < nSpectrumSamples; ++i) {
                double v;
                if (fscanf(f, "%lf ", &v) != 1) return false;
                c[i] = v;
            }
            return true;
        }
        float& operator[](int i) {
            assert(i >= 0 && i < nSpectrumSamples);
            return c[i];
        }
        float operator[](int i) const {
            assert(i >= 0 && i < nSpectrumSamples);
            return c[i];
        }

        // CoefficientSpectrum Public Data
        static const int nSamples = nSpectrumSamples;

    protected:
        // CoefficientSpectrum Protected Data
        float c[nSpectrumSamples];
    };

    class SampledSpectrum : public CoefficientSpectrum<nSpectralSamples> {
    public:
        // SampledSpectrum Public Methods
        SampledSpectrum(float v = 0.f);
        SampledSpectrum(const CoefficientSpectrum<nSpectralSamples>& v);
        static SampledSpectrum FromSampled(const float* lambda, const float* v,
            int n);
        static void Init();
        void ToXYZ(float xyz[3]) const;
        float y() const;
        void ToRGB(float rgb[3]) const;
        RGBSpectrum ToRGBSpectrum() const;
        static SampledSpectrum FromRGB(
            const float rgb[3], eSpectrumType type = eSpectrumType::SPECTRUM_ILLUMINANT);
        static SampledSpectrum FromXYZ(
            const float xyz[3], eSpectrumType type = eSpectrumType::SPECTRUM_REFLECTANCE);
        SampledSpectrum(const RGBSpectrum& r,
            eSpectrumType type = eSpectrumType::SPECTRUM_REFLECTANCE);

    private:
        // SampledSpectrum Private Data
        static SampledSpectrum X, Y, Z;
        static SampledSpectrum rgbRefl2SpectWhite, rgbRefl2SpectCyan;
        static SampledSpectrum rgbRefl2SpectMagenta, rgbRefl2SpectYellow;
        static SampledSpectrum rgbRefl2SpectRed, rgbRefl2SpectGreen;
        static SampledSpectrum rgbRefl2SpectBlue;
        static SampledSpectrum rgbIllum2SpectWhite, rgbIllum2SpectCyan;
        static SampledSpectrum rgbIllum2SpectMagenta, rgbIllum2SpectYellow;
        static SampledSpectrum rgbIllum2SpectRed, rgbIllum2SpectGreen;
        static SampledSpectrum rgbIllum2SpectBlue;
    };

    class RGBSpectrum : public CoefficientSpectrum<3> {
        using CoefficientSpectrum<3>::c;

    public:
        // RGBSpectrum Public Methods
        RGBSpectrum(float v = 0.f);
        RGBSpectrum(const CoefficientSpectrum<3>& v);
        RGBSpectrum(const RGBSpectrum& s,
            eSpectrumType type = eSpectrumType::SPECTRUM_REFLECTANCE);
        static RGBSpectrum FromRGB(const float rgb[3],
            eSpectrumType type = eSpectrumType::SPECTRUM_REFLECTANCE);
        void ToRGB(float* rgb) const;
        const RGBSpectrum& ToRGBSpectrum() const;
        void ToXYZ(float xyz[3]) const;
        static RGBSpectrum FromXYZ(const float xyz[3],
            eSpectrumType type = eSpectrumType::SPECTRUM_REFLECTANCE);
        float y() const;
        static RGBSpectrum FromSampled(const float* lambda, const float* v, int n);
    };

    // Spectrum Inline Functions
    template <int nSpectrumSamples>
    inline CoefficientSpectrum<nSpectrumSamples> Pow(
        const CoefficientSpectrum<nSpectrumSamples>& s, float e) {
        CoefficientSpectrum<nSpectrumSamples> ret;
        for (int i = 0; i < nSpectrumSamples; ++i) 
            ret.c[i] = std::pow(s.c[i], e);
        assert(!ret.HasNaNs());
        return ret;
    }

    inline RGBSpectrum Lerp(float t, const RGBSpectrum& s1, const RGBSpectrum& s2) {
        return (1 - t) * s1 + t * s2;
    }

    inline SampledSpectrum Lerp(float t, const SampledSpectrum& s1,
        const SampledSpectrum& s2) {
        return (1 - t) * s1 + t * s2;
    }

    void ResampleLinearSpectrum(const float* lambdaIn, const float* vIn, int nIn,
        float lambdaMin, float lambdaMax, int nOut,
        float* vOut);

#pragma endregion

	
}

