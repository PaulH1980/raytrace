#pragma once
#include <thread>
#include "Defines.h"

#include "MathCommon.h"
namespace RayTrace
{
    inline int     NumSystemCores() {
#if _DEBUG
        return 1;
#else
        return std::thread::hardware_concurrency();
#endif
    }

    inline int CountTrailingZeros(uint32_t v) {
#if defined(IS_MSVC)
        unsigned long index;
        if (_BitScanForward(&index, v))
            return index;
        else
            return 32;
#else
        return __builtin_ctz(v);
#endif
    }

    static constexpr int PrimeTableSize = 1000;
    extern const int PrimeSums[PrimeTableSize];
    extern const int Primes[PrimeTableSize];

    extern uint32_t CMaxMinDist[17][32];

    uint64_t SobolIntervalToIndex(const uint32_t m, uint64_t frame, const Vector2i& p);
    float SobolSample(int64_t index, int dimension, uint64_t scramble = 0);
    float SobolSampleFloat(int64_t a, int dimension, uint32_t scramble);

    inline uint32_t MultiplyGenerator(const uint32_t* C, uint32_t a) {
        uint32_t v = 0;
        for (int i = 0; a != 0; ++i, a >>= 1)
            if (a & 1) v ^= C[i];
        return v;
    }
    float    SampleGeneratorMatrix(const uint32_t* C, uint32_t a, uint32_t scramble = 0);

    uint32_t FloatToBits(float f);
    float    BitsToFloat(uint32_t ui);
    float    NextFloatUp(float v);
    float    NextFloatDown(float v);


    Vector3f OffsetRayOrigin(const Vector3f& p, const Vector3f& pError,
        const Vector3f& n, const Vector3f& w);

    template <int base>
    inline uint64_t InverseRadicalInverse(uint64_t inverse, int nDigits) {
        uint64_t index = 0;
        for (int i = 0; i < nDigits; ++i) {
            uint64_t digit = inverse % base;
            inverse /= base;
            index = index * base + digit;
        }
        return index;
    }

    std::vector<uint16_t> ComputeRadicalInversePermutations(RNG& rng);
    float ScrambledRadicalInverse(int baseIndex, uint64_t a, const uint16_t* perm);

    template <typename Predicate>
    int FindInterval(int size, const Predicate& pred) {
        int first = 0, len = size;
        while (len > 0) {
            int half = len >> 1, middle = first + half;
            // Bisect range based on value of _pred_ at _middle_
            if (pred(middle)) {
                first = middle + 1;
                len -= half + 1;
            }
            else
                len = half;
        }
        return std::clamp(first - 1, 0, size - 2);
    }

    
}