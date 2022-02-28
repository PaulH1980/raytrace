#pragma once
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>

#include "Defines.h"


namespace RayTrace
{
	constexpr float	PI			= 3.1415926535897932384f;
	constexpr float HALF_PI		= PI * 0.5f;
	constexpr float	TWO_PI		= PI * 2.0f;
	constexpr float	FOUR_PI		= PI * 4.0f;
	constexpr float RADIANS		= PI / 180.0f;
	constexpr float DEGREES		= 180.0f / PI;
	constexpr float INV_PI		= 1.0f / PI;
	constexpr float INV_TWO_PI	= 1.0f / TWO_PI;
	constexpr float INV_FOUR_PI = 1.0f / FOUR_PI;
	constexpr float INV_HALF_PI = 1.0f / HALF_PI;
	constexpr float PI_OVER_TWO  = PI / 2.0f;
	constexpr float PI_OVER_FOUR = PI / 4.0f;

	static constexpr float OneMinusEpsilon = 0.9999999403953552f;
	static constexpr float InfinityF32 	   = std::numeric_limits<float>::infinity();
	static constexpr float LowestF32       = std::numeric_limits<float>::lowest();
	static constexpr float MaxF32		   = std::numeric_limits<float>::max();
	static constexpr float MachineEpsilon  = (std::numeric_limits<float>::epsilon() * 0.5);

	template<typename T>
	T Lerp(const T& _start, const T& _end, float _t) {
		if (_t < 0.f)
			return _start;
		if (_t > 1.f)
			return _end;
		const T diff = (_end - _start) * _t;
		return _start + diff;
	}


	static inline float ToRadians(float angle)
	{
		return angle * RADIANS;
	}

	template<typename T>
	inline int SolveQuadratic(const T& a, const T& b, const T& c, T& x0, T& x1)
	{
		T discr = b * b - 4 * a * c;
		if (discr < T(0))
			return 0;
		else if (discr == T(0))
		{
			x0 = x1 = T(-0.5) * b / a;
			return 1; //1st solution
		}
		else
		{
			T q = b > T(0) ? T(-0.5) * (b + sqrt(discr)) :
				T(-0.5) * (b - sqrt(discr));
			x0 = q / a;
			x1 = c / q;
		}
		if (x0 > x1)
			std::swap(x0, x1);
		return 2; //2nd solution
	}


	
	Vector3f		SphericalDirection(float theta, float phi);
	Vector3f		SphericalDirection(float sinTheta, float cosTheta, float phi);
	Vector3f		SphericalDirection(float sinTheta, float cosTheta, float phi,
								   const Vector3f& x, const Vector3f& y, const Vector3f& z);

	float			SphericalTheta(const Vector3f& _n);
	float			SphericalPhi(const Vector3f& _n);
	Vector2f		SphericalThetaPhi(const Vector3f& _n);

	int				Clamp(int _v, int _lo, int _hi);
    double			Clamp(double _v, double _lo, double _hi);
	float			Clamp(float _v, float _lo, float _hi);
	uint32_t	    Clamp(uint32_t _v, uint32_t _lo, uint32_t _hi);

    template <typename T>
    inline constexpr bool IsPowerOf2(T v) {
        return v && !(v & (v - 1));
    }


	inline uint32_t RoundUpPow2(uint32_t v) {
		v--;
		v |= v >> 1;    v |= v >> 2;
		v |= v >> 4;    v |= v >> 8;
		v |= v >> 16;
		return v + 1;
	}

    inline int Ceil2Int(float val) {
        return (int)ceilf(val);
    }

	inline int Floor2Int(float val) {
		return (int)floorf(val);
	}

	inline int Round2Int(float val) {
		return Floor2Int(val + 0.5f);
	}

	inline int Float2Int(float val) {
		return (int)val;
	}


    inline float Log2(float x) {
        static const float invLog2 = 1.f / std::logf(2.f);
        return logf(x) * invLog2;
    }

    inline int Log2Int(float v) {
        return Floor2Int(Log2(v));
    }

    inline int Mod(int a, int b) {
        int n = int(a / b);
        a -= n * b;
        if (a < 0) a += b;
        return a;
    }

    inline float gamma(int n) {
        return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
    }

    inline uint32_t ReverseBits32(uint32_t n) {
        n = (n << 16) | (n >> 16);
        n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
        n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
        n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
        n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
        return n;
    }

    inline uint64_t ReverseBits64(uint64_t n) {
        uint64_t n0 = ReverseBits32((uint32_t)n);
        uint64_t n1 = ReverseBits32((uint32_t)(n >> 32));
        return (n0 << 32) | n1;
    }

    template<typename T>
    inline bool HasNans(const Vector3<T>& _v)
    {
        return std::isnan(_v[0]) || std::isnan(_v[1]) || std::isnan(_v[2]);
    }

    template<typename T>
    inline bool HasInfinities(const Vector3<T>& _v)
    {
        return std::isinf(_v[0]) || std::isinf(_v[1]) || std::isinf(_v[2]);
    }

    template<typename T>
    inline bool HasNegatives(const Vector3<T>& _v)
    {
        return (_v[0] < 0.0f) ||(_v[1] < 0.0f) || (_v[2] < 0.0f);
    }

    template<typename T>
    inline Vector3<T> operator*(T f, const Vector3<T>& v) { return v * f; }

    template<typename T>
    inline Vector2<T> operator*(T f, const Vector2<T>& v) { return v * f; }
    
    template<typename T>
    inline Vector3<T> ToVector3(const Vector2<T>& _v)
    {
        return Vector3<T>(_v.x, _v.y, (T)0);
    }


    template<typename T>
    inline Vector3<T> ToVector3(const Vector4<T>& _v)
    {
        return Vector3<T>(_v.x, _v.y, _v.z);
    }


    template<typename T>
    inline T Dot(const Vector2<T>& v1, const Vector2<T>& v2)
    {
        return glm::dot(v1, v2);
    }


    template<typename T>
    inline T Dot(const Vector3<T>& v1, const Vector3<T>& v2)
    {
        return glm::dot(v1, v2);
    }


    template<typename Vec>
    inline float LengthSqr(const Vec& _v) {
        return Dot(_v, _v);
    }

    template<typename Vec>
    inline float Length(const Vec& _v) {

        return glm::length(_v);
    }

    template<typename Vec>
    inline float Distance(const Vec& _a, const Vec& _b) {
        return Length(_a - _b);
    }

    template<typename Vec>
    inline float DistanceSqr(const Vec& _a, const Vec& _b) {
        return LengthSqr(_a - _b);
    }

    template<typename T>
    Vector3<T> Inverted(const Vector3<T>& _v)
    {
        Vector3<T> result((T)1.0 / _v.x, (T)1.0 / _v.y, (T)1.0 / _v.z);
        return result;
    }

    template<typename T>
    Vector2<T> Inverted(const Vector2<T>& _v)
    {
        Vector2<T> result((T)1.0 / _v.x, (T)1.0 / _v.y);
        return result;
    }

    template<typename T>
    inline T AbsDot(const Vector3<T>& v1, const Vector3<T>& v2)
    {
        return glm::abs(Dot(v1, v2));
    }

    template<typename T>
    inline Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2)
    {
        return  glm::cross(v1, v2);
    }

    template<typename T>
    inline Vector3<T> Abs(const Vector3<T>& v) {
        return glm::abs(v);
    }

    template<typename T>
    inline Vector3<T> Normalize(const Vector3<T>& v) {
        return  glm::normalize(v);
    }

    template<typename T>
    inline Vector2<T> Normalize(const Vector2<T>& v) {
        return  glm::normalize(v);
    }

    template<typename T>
    inline Vector3<T> FaceForward(const Vector3<T>& n1, const Vector3<T>& n2)
    {
        if (glm::dot(n1, n2) < (T)0.0f)
            return -n1;
        return n1;
    }

    template <typename T>
    T MaxComponent(const Vector3<T>& v) {
        return std::max(v.x, std::max(v.y, v.z));
    }

    

    template <typename T>
    Vector2<T> Min(const Vector2<T>& _a, const Vector2<T>& _b)
    {
        return glm::min(_a, _b);
    }

    template <typename T>
    Vector2<T> Max(const Vector2<T>& _a, const Vector2<T>& _b)
    {
        return glm::max(_a, _b);
    }

    template <typename T>
    Vector3<T> Min(const Vector3<T>& _a, const Vector3<T>& _b)
    {
        // return _a.minVal(_b);
        return glm::min(_a, _b);
    }

    template <typename T>
    Vector3<T> Max(const Vector3<T>& _a, const Vector3<T>& _b)
    {
        return glm::max(_a, _b);
    }


    template <typename T>
    Vector4<T> Min(const Vector4<T>& _a, const Vector4<T>& _b)
    {
        return glm::min(_a, _b);
    }

    template <typename T>
    Vector4<T> Max(const Vector4<T>& _a, const Vector4<T>& _b)
    {
        return glm::max(_a, _b);
    }



    template <typename T>
    Vector2<T> Clamp(const Vector2<T>& _v, const Vector2<T>& _lo, const Vector2<T>& _hi) {
        const auto tmp = Max(_v, _lo);
        return Min(tmp, _hi);
    }

    template <typename T>
    Vector3<T> Clamp(const Vector3<T>& _v, const Vector3<T>& _lo, const Vector3<T>& _hi) {
        const auto tmp = Max(_v, _lo);
        return Min(tmp, _hi);
    }

    template <typename T>
    Vector4<T> Clamp(const Vector4<T>& _v, const Vector4<T>& _lo, const Vector4<T>& _hi) {
        const auto tmp = Max(_v, _lo);
        return Min(tmp, _hi);
    }




    template <typename T>
    eAxis MaxDimension(const Vector3<T>& v)
    {
        if (v.x > v.y) {
            if (v.x > v.z)
                return eAxis::AXIS_X;
            return eAxis::AXIS_Z;
        }
        if (v.y > v.z)
            return eAxis::AXIS_Y;
        return eAxis::AXIS_Z;
    }

    template <typename T>
    Vector3<T> Permute(const Vector3<T>& v, int x, int y, int z) {
        return Vector3<T>(v[x], v[y], v[z]);
    }

    template <typename T>
    Vector3<T> MultiplyXYZ(const Vector3<T>& _in, const Vector3<T>& _x, const Vector3<T>& _y, const Vector3<T>& _z)
    {
        return Vector3<T>(_in[0] * _x, _in[1] * _y, _in[2] * _z);
    }

	Matrix4x4  Interpolate(const Matrix4x4& _to, const Matrix4x4& _from, float _time);




    template<typename T>
    inline void CoordinateSystem(const Vector3<T>& v1, Vector3<T>* v2, Vector3<T>* v3) {
        if (fabsf(v1.x) > fabsf(v1.y)) {
            float invLen = 1.f / sqrtf(v1.x * v1.x + v1.z * v1.z);
            *v2 = Vector3<T>(-v1.z * invLen, 0.f, v1.x * invLen);
        }
        else {
            float invLen = 1.f / sqrtf(v1.y * v1.y + v1.z * v1.z);
            *v2 = Vector3<T>(0.f, v1.z * invLen, -v1.y * invLen);
        }
        *v3 = Cross(v1, *v2);
    }

    template<typename T>
    std::string ToString(const Vector2<T>& _v) {
        return fmt::format("vec2[ {}, {} ]", _v.x, _v.y);
    }


    template<typename T>
    std::string ToString(const Vector3<T>& _v) {
        return fmt::format("vec3[ {}, {}, {} ]", _v.x, _v.y, _v.z);
    }


    static const auto Vec2fZero = Vector2f(0.0f);
    static const auto Vec2fHalf = Vector2f(0.5f);
    static const auto Vec2fOne = Vector2f(1.0f);

    static const auto Vec3fZero = Vector3f(0.0f);
    static const auto Vec3fHalf = Vector3f(0.5f);
    static const auto Vec3fOne = Vector3f(1.0f);

    static const auto Identity4x4 = Matrix4x4(1.0f);


    static const Matrix4x4 ToLeftHand = {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  -1.0f, 0.0f,
        0.0f, 0.0f,  0.0f, 1.0f
    };

}