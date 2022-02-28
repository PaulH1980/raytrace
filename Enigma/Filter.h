#pragma once
#include "MathCommon.h"

#include "Defines.h"

namespace RayTrace
{
	class Filter
	{
	public:
		// Filter Interface

		Filter(const Vector2f& _radius)
			: m_radius(_radius)
			, m_invRadius( Inverted( _radius ) )
		{
		}

		Filter(float xw, float yw)
			: Filter(Vector2f(xw, yw)) {

		}
		virtual ~Filter() {}
		virtual float Evaluate(const Vector2f& _xy) const = 0;

		// Filter Public Data
		const Vector2f m_radius;
		const Vector2f m_invRadius;
	};

	class BoxFilter : public Filter
	{
	public:
		BoxFilter(const Vector2f& _radius) : Filter(_radius) {}
		float Evaluate(const Vector2f& _xy) const override;
	};

	class MitchellFilter : public Filter
	{
	public:

		MitchellFilter( const Vector2f& _radius, float b, float c ) 
			: Filter(_radius)
			, m_b(b)
			, m_c(c) {}

		float Evaluate(const Vector2f& _xy) const override;


		float Mitchell1D(float x) const {
			x = fabsf(2.f * x);
			if (x > 1.f)
				return ((-m_b - 6 * m_c) * x * x * x + (6 * m_b + 30 * m_c) * x * x +
					   (-12 * m_b - 48 * m_c) * x + (8 * m_b + 24 * m_c)) * (1.f / 6.f);
			else
				return ((12 - 9 * m_b - 6 * m_c) * x * x * x +
					(-18 + 12 * m_b + 6 * m_c) * x * x +
					(6 - 2 * m_b)) * (1.f / 6.f);
		}
		float m_b, m_c;
	};

	class LanczosSincFilter : public Filter
	{
	public:
		LanczosSincFilter(const Vector2f& _radius, float tau)
			: Filter(_radius)
			, m_tau(tau) 
		{
		}

		float Evaluate(const Vector2f& _xy)const override;

		float Sinc1D(float x) const  {
			x = fabsf(x);
			if (x < 1e-5) return 1.f;
			if (x > 1.)   return 0.f;
			x *= PI;
			float sinc = sinf(x) / x;
			float lanczos = sinf(x * m_tau) / (x * m_tau);
			return sinc * lanczos; 
		}

		float m_tau;
	};

	class GaussianFilter : public Filter
	{
	public:
		GaussianFilter(const Vector2f& _radius, float a)
			: Filter(_radius)
			, m_alpha(a)
			, m_expX(expf(-m_alpha * _radius[0] * _radius[0]))
			, m_expY(expf(-m_alpha * _radius[1] * _radius[1])) { 
		}

		float Evaluate(const Vector2f& _xy) const override;

	private:
		// GaussianFilter Utility Functions
		float Gaussian(float d, float expv) const {
			return std::max(0.f, float(expf(-m_alpha * d * d) - expv));
		}

		const float m_alpha;
		const float m_expX, m_expY;
	};

    // Triangle Filter Declarations
    class TriangleFilter : public Filter {
    public:
        TriangleFilter(const Vector2f& _radius) 
			: Filter(_radius) {
		}

        float Evaluate(const Vector2f& _xy) const override;		
    };

	
   
    Filter* CreateBoxFilter(const ParamSet& _params);    
    Filter* CreateGaussianFilter(const ParamSet& _params);
    Filter* CreateMitchellFilter(const ParamSet& _params);
    Filter* CreateSincFilter(const ParamSet& _params);
    Filter* CreateTriangleFilter(const ParamSet& _params);

      
}