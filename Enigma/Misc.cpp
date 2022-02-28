#include "Misc.h"

namespace RayTrace
{

    float SampleGeneratorMatrix(const uint32_t* C, uint32_t a, uint32_t scramble /*= 0*/)
    {
        float val = (MultiplyGenerator(C, a) ^ scramble);
        return std::min(val * float(2.3283064365386963e-10), OneMinusEpsilon);
    }

    uint32_t FloatToBits(float f)
    {
        uint32_t ui;
        memcpy(&ui, &f, sizeof(float));
        return ui;
    }

    float BitsToFloat(uint32_t ui)
    {
        float f;
        memcpy(&f, &ui, sizeof(uint32_t));
        return f;
    }

    float NextFloatUp(float v)
    {
        // Handle infinity and negative zero for _NextFloatUp()_
        if (std::isinf(v) && v > 0.) return v;
        if (v == -0.f) v = 0.f;

        // Advance _v_ to next higher float
        uint32_t ui = FloatToBits(v);
        if (v >= 0)
            ++ui;
        else
            --ui;
        return BitsToFloat(ui);
    }

    float NextFloatDown(float v)
    {
        // Handle infinity and positive zero for _NextFloatDown()_
        if (std::isinf(v) && v < 0.) return v;
        if (v == 0.f) v = -0.f;
        uint32_t ui = FloatToBits(v);
        if (v > 0)
            --ui;
        else
            ++ui;
        return BitsToFloat(ui);
    }

    Vector3f OffsetRayOrigin(const Vector3f& p, const Vector3f& pError, const Vector3f& n, const Vector3f& w)
    {
        float d = Dot(Abs(n), pError);   
        Vector3f offset = d * Vector3f(n);
        if (Dot(w, n) < 0) offset = -offset;
        Vector3f po = p + offset;
        // Round offset point _po_ away from _p_
        for (int i = 0; i < 3; ++i) {
            if (offset[i] > 0)
                po[i] = NextFloatUp(po[i]);
            else if (offset[i] < 0)
                po[i] = NextFloatDown(po[i]);
        }
        return po;
    }

}

