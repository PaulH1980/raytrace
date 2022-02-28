#pragma once
#include <limits>
#include <random>
#include <memory>
#include "MathCommon.h"

namespace RayTrace
{
	//	
	
	class RNG 
	{
	public:

		RNG( float _minF32 = 0.0f, float _maxF32Val = OneMinusEpsilon,
			 uint32_t _minU32Val = 0, uint32_t _maxU32Val = std::numeric_limits<uint32_t>::max(), uint32_t _seed = 0);


		uint32_t randomUInt();
		uint32_t randomUInt(uint32_t v);

		float randomFloat();

		float randomFloat(float _min, float _max);

		void  setSeed(uint32_t _seed);;

		float uniformFloat();

		

		float				m_f32Min,
							m_f32Max;
		uint32_t			m_u32Min;
		uint32_t			m_u32Max;
		uint32_t			m_seed = 0;
		
	private:
		
		std::mt19937		m_generator;
		std::seed_seq		m_seq{ 1, 1, 1, 1, 1 };
		std::uniform_real_distribution<float>   m_f32Distribution;
		std::uniform_int_distribution<uint32_t> m_u32Distribution;
	};

	
}