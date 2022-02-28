#include "MathCommon.h"
#include "RNG.h"


namespace RayTrace
{
	RNG::RNG(float _minF32 /*= 0.0f*/, float _maxF32Val /*= FloatOneMinusEpsilon*/, 
        uint32_t _minU32Val /*= 0*/, uint32_t _maxU32Val,  uint32_t _seed)
        
        : m_f32Distribution(_minF32, _maxF32Val)
        , m_u32Distribution(_minU32Val, _maxU32Val)
        , m_u32Min(_minU32Val)
        , m_u32Max(_maxU32Val)
        , m_f32Min(_minF32)
        , m_f32Max(_maxF32Val)
        , m_seed( _seed )
    {
       // m_generator.seed(m_seed);
      //  m_generator.seed(_seed);
    }

  
    uint32_t RNG::randomUInt()
    {
        return m_u32Distribution(m_generator);
    }



    uint32_t RNG::randomUInt(uint32_t b)
    {

        uint32_t threshold = (~b + 1u) % b;
        while (true) {
            uint32_t r = randomUInt();
            if (r >= threshold) return r % b;

        }
    }

    float RNG::randomFloat(float _min, float _max)
    {
        auto distRng    =  m_f32Max - m_f32Min;
        float offsetRng =  randomFloat() - m_f32Min;
        float delta = offsetRng / distRng;
        return Lerp(_min, _max, delta);

    }

    float RNG::randomFloat()
    {
        return m_f32Distribution(m_generator);
    }

    void RNG::setSeed(uint32_t _seed)
    {
        m_generator.seed(_seed);
    }

    float RNG::uniformFloat() {
        float val = std::min(OneMinusEpsilon, float(randomUInt() * 0x1p-32f));
        assert(val >= 0.0f && val < 1.0f);
        return val;
    }

}

