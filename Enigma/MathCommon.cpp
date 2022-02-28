
#include "RNG.h"
#include "MathCommon.h"

namespace RayTrace
{
	



  

	Vector3f SphericalDirection(float sinTheta, float cosTheta, float phi)
	{
		return Vector3f(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
	}

	Vector3f SphericalDirection(float sinTheta, float cosTheta, float phi, 
		const Vector3f& x, const Vector3f& y, const Vector3f& z)	{
		return ( sinTheta * cosf(phi) * x ) +
			   ( sinTheta * sinf(phi) * y ) + 
			   ( cosTheta * z );
	}
	
	Vector3f SphericalDirection(float theta, float phi)
	{
		return SphericalDirection(sinf(theta), cosf(theta), phi);
	}

	float SphericalTheta(const Vector3f& _n)
	{
		return  std::acosf(std::clamp(_n.z, -1.0f, 1.0f));
	}

	float SphericalPhi(const Vector3f& _n)
	{
		const float p = atan2f(_n.y, _n.x);
		return (p < 0.0f) ? p + TWO_PI
			: p;
	}

	Vector2f SphericalThetaPhi(const Vector3f& _n)
	{
		return Vector2f(SphericalTheta(_n), SphericalPhi(_n));
	}

    int Clamp(int _v, int _lo, int _hi)
    {
        if (_v < _lo)
            return _lo;
        if (_v > _hi)
            return _hi;
        return _v;
    }

    uint32_t Clamp(uint32_t _v, uint32_t _lo, uint32_t _hi)
    {
        if (_v < _lo)
            return _lo;
        if (_v > _hi)
            return _hi;
        return _v;
    }

    float Clamp(float _v, float _lo, float _hi)
    {

        if (_v < _lo)
            return _lo;
        if (_v > _hi)
            return _hi;
        return _v;
    }

    double Clamp(double _v, double _lo, double _hi)
    {
        if (_v < _lo)
            return _lo;
        if (_v > _hi)
            return _hi;
        return _v;
    }

	Matrix4x4 Interpolate(const Matrix4x4& _to, const Matrix4x4& _from, float _time)
	{
	    
		return glm::interpolate(_to, _from, _time);
	}

}

