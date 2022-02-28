#pragma once

#include "Defines.h"
#include "MathCommon.h"



namespace RayTrace
{
	class Ray
	{
	public:

		Ray();

		

        Ray(const Vector3f& _o,
            const Vector3f& _d,
			float _maxT = InfinityF32 );


		Ray(const Vector3f& _o, 
			const Vector3f& _d, 			
			float _maxT,
			float _time,
			const Medium *_pMedium = nullptr );     

		

        bool     hasNaNs() const noexcept;
		Vector3f scale(float _t) const noexcept;

		Vector3f                    m_origin;
		Vector3f                    m_dir;

		//mutable float				m_minT  = { 0.0f };
		mutable float				m_maxT  = InfinityF32;
		const  Medium*				m_medium = nullptr;
		float						m_time  = { 0.0f };		
	};

	class RayDifferential : public Ray
	{
	public:

        RayDifferential();
        RayDifferential(const Vector3f& _o, const Vector3f& _d, float _tMax = InfinityF32,
            float _time = 0.f, const Medium* _medium = nullptr);
        RayDifferential(const Ray& ray);


		void scaleDifferentials(float _s);
        bool hasNaNs() const noexcept;
   

		bool    m_hasDifferentials = { false };
		Vector3f m_rxOrigin, m_ryOrigin;
		Vector3f m_rxDirection, m_ryDirection;
	};
}