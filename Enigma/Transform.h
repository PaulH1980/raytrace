#pragma once
#include <cstring>
#include <cmath>
#include <assert.h>
#include <algorithm>
#include "MathCommon.h"
#include "Ray.h"
#include "BBox.h"

namespace RayTrace
{
	class Transform
	{
	public:

		Transform() {}

		Transform(const Matrix4x4& _data);

		Transform(const Matrix4x4& _data, const Matrix4x4& _invData);

		Transform(const Transform& _rhs);


		bool	  isIdentity() const {		
			return ( m_mat[0][0] == 1.f && m_mat[0][1] == 0.f &&
					 m_mat[0][2] == 0.f && m_mat[0][3] == 0.f &&
					 m_mat[1][0] == 0.f && m_mat[1][1] == 1.f &&
					 m_mat[1][2] == 0.f && m_mat[1][3] == 0.f &&
					 m_mat[2][0] == 0.f && m_mat[2][1] == 0.f &&
					 m_mat[2][2] == 1.f && m_mat[2][3] == 0.f &&
					 m_mat[3][0] == 0.f && m_mat[3][1] == 0.f &&
					 m_mat[3][2] == 0.f && m_mat[3][3] == 1.f);			
		}

		Vector3f	  transformPoint(const Vector3f& _rhs) const noexcept {
			float x = _rhs.x, y = _rhs.y, z = _rhs.z;
			float xp = m_mat[0][0] * x + m_mat[0][1] * y + m_mat[0][2] * z + m_mat[0][3];
			float yp = m_mat[1][0] * x + m_mat[1][1] * y + m_mat[1][2] * z + m_mat[1][3];
			float zp = m_mat[2][0] * x + m_mat[2][1] * y + m_mat[2][2] * z + m_mat[2][3];
			float wp = m_mat[3][0] * x + m_mat[3][1] * y + m_mat[3][2] * z + m_mat[3][3];
			
			if (wp == 1.) return Vector3f(xp, yp, zp);
			else          return Vector3f(xp, yp, zp) / wp;
		}

		Vector3f	  transformVector(const Vector3f& _rhs) const noexcept {
			float x = _rhs.x, y = _rhs.y, z = _rhs.z;
			return Vector3f(m_mat[0][0] * x + m_mat[0][1] * y + m_mat[0][2] * z,
						   m_mat[1][0] * x + m_mat[1][1] * y + m_mat[1][2] * z,
						   m_mat[2][0] * x + m_mat[2][1] * y + m_mat[2][2] * z);
		}

		Vector3f   transformNormal(const Vector3f& _rhs) const noexcept {
			float x = _rhs.x, y = _rhs.y, z = _rhs.z;
			return Vector3f(m_invMat[0][0] * x + m_invMat[1][0] * y + m_invMat[2][0] * z,
						    m_invMat[0][1] * x + m_invMat[1][1] * y + m_invMat[2][1] * z,
						    m_invMat[0][2] * x + m_invMat[1][2] * y + m_invMat[2][2] * z);
		}

		Ray		  transformRay(const Ray& _ray) const noexcept {
			Ray retVal      = _ray;
			retVal.m_dir    = transformVector(retVal.m_dir);
			retVal.m_origin = transformPoint(retVal.m_origin);
			return retVal;
		}

		RayDifferential transformRayDifferential(const RayDifferential& _rayDiff)const noexcept
		{
			RayDifferential retVal = _rayDiff;
			retVal.m_dir    = transformVector(retVal.m_dir);
			retVal.m_origin = transformPoint(retVal.m_origin);

			retVal.m_rxOrigin = transformPoint(retVal.m_rxOrigin);
			retVal.m_ryOrigin = transformPoint(retVal.m_ryOrigin);

			retVal.m_rxDirection = transformVector(retVal.m_rxDirection);
			retVal.m_ryDirection = transformVector(retVal.m_ryDirection);
			return retVal;
		}

		BBox3f transformBounds(const BBox3f& _rhs) const noexcept {
				BBox3f retVal(transformPoint(Vector3f{ _rhs.m_min[0], _rhs.m_min[1], _rhs.m_min[2] }));

				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_min[0], _rhs.m_max[1], _rhs.m_min[2] }));
				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_max[0], _rhs.m_min[1], _rhs.m_min[2] }));
				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_max[0], _rhs.m_max[1], _rhs.m_min[2] }));				

				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_min[0], _rhs.m_max[1], _rhs.m_max[2] }));				
				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_min[0], _rhs.m_min[1], _rhs.m_max[2] }));
				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_max[0], _rhs.m_min[1], _rhs.m_max[2] }));
				retVal.addPoint(transformPoint(Vector3f{ _rhs.m_max[0], _rhs.m_max[1], _rhs.m_max[2] }));
				return retVal;
		}

		bool swapsHandedness() const noexcept {
			glm::mat3 m3x3 = glm::mat3(m_mat);
			return glm::determinant(m3x3) < 0.0f;	
		}

		bool hasScale() const noexcept
		{
			auto isOne = [](const Vector3f _v){
				static const float eps = 0.0001f;
				constexpr auto lngth = _v.length();
				return (lngth > (1.0f - eps)) && (lngth < (1.0f + eps));
			};
			
			auto x = transformVector(Vector3f(1, 0, 0));
			auto y = transformVector(Vector3f(0, 1, 0));
			auto z = transformVector(Vector3f(0, 0, 1));

			return (!isOne(x) && !isOne(y) && !isOne(z));

		}


		bool operator == (const Transform& _rhs) const noexcept
		{
			return m_mat == _rhs.m_mat &&
				   m_invMat == _rhs.m_invMat;
		}

		bool operator != (const Transform& _rhs) const noexcept
		{
			return !(*this == _rhs);
		}

		Transform operator *(const Transform& _rhs) const noexcept;


		Transform inverted() const noexcept {
			return Transform(m_invMat, m_mat);
		}


		SurfaceInteraction transformSurfInteraction(const SurfaceInteraction& si) const;

        template <typename T>
        inline Vector3<T> transformPoint(const Vector3<T>& pt, Vector3<T>* absError) const;
        
		template <typename T>
        inline Vector3<T> transformPoint(const Vector3<T>& p, const Vector3<T>& pError, Vector3<T>* pTransError) const;
        
		template <typename T>
        inline Vector3<T> transformVector(const Vector3<T>& v, Vector3<T>* vTransError) const;
       
		template <typename T>
        inline Vector3<T> transformVector(const Vector3<T>& v, const Vector3<T>& vError, Vector3<T>* vTransError) const;
        
		inline Ray transformRay(const Ray& r, Vector3f* oError, Vector3f* dError) const;
        inline Ray transformRay(const Ray& r, const Vector3f& oErrorIn, const Vector3f& dErrorIn, Vector3f* oErrorOut, Vector3f* dErrorOut) const;


        const Matrix4x4& GetMatrix() const { return m_mat; }
        const Matrix4x4& GetInverseMatrix() const { return m_invMat; }

		Matrix4x4 m_mat, m_invMat;
	};





	class AnimatedTransform 
	{
	public:
		
		AnimatedTransform( const Transform* _start, float _time1,
						   const Transform* _end, float _time2 );



		static void				Decompose(const Matrix4x4& m, Vector3f* T, Quatf* R, Vector3f* S);
		void					interpolate(float time, Transform* _tOut) const;
		Ray						interpolateRay(const Ray& r) const noexcept;
		RayDifferential			interpolateRayDifferential(const RayDifferential& r) const noexcept;
		Vector3f				interpolatePoint( const Vector3f& _rhs, float _time ) const noexcept;
		Vector3f				interpolateVector( const Vector3f& _rhs, float _time ) const noexcept;
		
		BBox3f					motionBounds(const BBox3f& b) const;
		

		const Transform* m_start = nullptr, 
			            *m_end = nullptr;
		const float      m_startTime = 0.0f, m_endTime = 1.0f;
		bool			 m_isAnimated = false;

		Vector3f		 m_trans[2];
		Quatf			 m_rot[2];
		Vector3f		 m_scale[2];
	};

	Transform Translate(const Vector3f& delta);
	Transform Scale(float x, float y, float z);
	Transform RotateX(float angle);
	Transform RotateY(float angle);
	Transform RotateZ(float angle);
	Transform Rotate(float angle, const Vector3f& axis);
	Transform LookAt(const Vector3f& pos, const Vector3f& look, const Vector3f& up);

	Transform Orthographic(float znear, float zfar);
	




	bool SolveLinearSystem2x2(const float A[2][2], const float B[2],
		float* x0, float* x1);


    template <typename T>
    inline Vector3<T> Transform::transformPoint(const Vector3<T>& p,
        Vector3<T>* pError) const {
        T x = p.x, y = p.y, z = p.z;
        // Compute transformed coordinates from point _pt_
        T xp = (m_mat[0][0] * x + m_mat[0][1] * y) + (m_mat[0][2] * z + m_mat[0][3]);
        T yp = (m_mat[1][0] * x + m_mat[1][1] * y) + (m_mat[1][2] * z + m_mat[1][3]);
        T zp = (m_mat[2][0] * x + m_mat[2][1] * y) + (m_mat[2][2] * z + m_mat[2][3]);
        T wp = (m_mat[3][0] * x + m_mat[3][1] * y) + (m_mat[3][2] * z + m_mat[3][3]);

        // Compute absolute error for transformed point
        T xAbsSum = (std::abs(m_mat[0][0] * x) + std::abs(m_mat[0][1] * y) +
            std::abs(m_mat[0][2] * z) + std::abs(m_mat[0][3]));
        T yAbsSum = (std::abs(m_mat[1][0] * x) + std::abs(m_mat[1][1] * y) +
            std::abs(m_mat[1][2] * z) + std::abs(m_mat[1][3]));
        T zAbsSum = (std::abs(m_mat[2][0] * x) + std::abs(m_mat[2][1] * y) +
            std::abs(m_mat[2][2] * z) + std::abs(m_mat[2][3]));
        *pError = gamma(3) * Vector3<T>(xAbsSum, yAbsSum, zAbsSum);
		assert(wp != 0.0);
        if (wp == 1)
            return Vector3<T>(xp, yp, zp);
        else
            return Vector3<T>(xp, yp, zp) / wp;
    }

    template <typename T>
    inline Vector3<T> Transform::transformPoint(const Vector3<T>& pt,
        const Vector3<T>& ptError,
        Vector3<T>* absError) const {
        T x = pt.x, y = pt.y, z = pt.z;
        T xp = (m_mat[0][0] * x + m_mat[0][1] * y) + (m_mat[0][2] * z + m_mat[0][3]);
        T yp = (m_mat[1][0] * x + m_mat[1][1] * y) + (m_mat[1][2] * z + m_mat[1][3]);
        T zp = (m_mat[2][0] * x + m_mat[2][1] * y) + (m_mat[2][2] * z + m_mat[2][3]);
        T wp = (m_mat[3][0] * x + m_mat[3][1] * y) + (m_mat[3][2] * z + m_mat[3][3]);
        absError->x =
            (gamma(3) + (T)1) *
				(std::abs(m_mat[0][0]) * ptError.x + std::abs(m_mat[0][1]) * ptError.y +
                 std::abs(m_mat[0][2]) * ptError.z) +
             gamma(3) * (std::abs(m_mat[0][0] * x) + std::abs(m_mat[0][1] * y) +
					    std::abs(m_mat[0][2] * z) + std::abs(m_mat[0][3]));
        absError->y =
            (gamma(3) + (T)1) *
				(std::abs(m_mat[1][0]) * ptError.x + std::abs(m_mat[1][1]) * ptError.y +
                std::abs(m_mat[1][2]) * ptError.z) +
            gamma(3) * (std::abs(m_mat[1][0] * x) + std::abs(m_mat[1][1] * y) +
                std::abs(m_mat[1][2] * z) + std::abs(m_mat[1][3]));
        absError->z =
            (gamma(3) + (T)1) *
				(std::abs(m_mat[2][0]) * ptError.x + std::abs(m_mat[2][1]) * ptError.y +
                 std::abs(m_mat[2][2]) * ptError.z) +
            gamma(3) * (std::abs(m_mat[2][0] * x) + std::abs(m_mat[2][1] * y) +
                std::abs(m_mat[2][2] * z) + std::abs(m_mat[2][3]));
		assert(wp != 0.0f);
        if (wp == 1.)
            return Vector3<T>(xp, yp, zp);
        else
            return Vector3<T>(xp, yp, zp) / wp;
    }

    template <typename T>
    inline Vector3<T> Transform::transformVector(const Vector3<T>& v,
        Vector3<T>* absError) const {
        T x = v.x, y = v.y, z = v.z;
        absError->x =
            gamma(3) * (std::abs(m_mat[0][0] * v.x) + std::abs(m_mat[0][1] * v.y) +
                std::abs(m_mat[0][2] * v.z));
        absError->y =
            gamma(3) * (std::abs(m_mat[1][0] * v.x) + std::abs(m_mat[1][1] * v.y) +
                std::abs(m_mat[1][2] * v.z));
        absError->z =
            gamma(3) * (std::abs(m_mat[2][0] * v.x) + std::abs(m_mat[2][1] * v.y) +
                std::abs(m_mat[2][2] * v.z));
        return Vector3<T>(m_mat[0][0] * x + m_mat[0][1] * y + m_mat[0][2] * z,
						  m_mat[1][0] * x + m_mat[1][1] * y + m_mat[1][2] * z,
						  m_mat[2][0] * x + m_mat[2][1] * y + m_mat[2][2] * z);
    }

    template <typename T>
    inline Vector3<T> Transform::transformVector(const Vector3<T>& v,
        const Vector3<T>& vError,
        Vector3<T>* absError) const {
        T x = v.x, y = v.y, z = v.z;
        absError->x =
            (gamma(3) + (T)1) *
				(std::abs(m_mat[0][0]) * vError.x + std::abs(m_mat[0][1]) * vError.y +
                 std::abs(m_mat[0][2]) * vError.z) +
            gamma(3) * (std::abs(m_mat[0][0] * v.x) + std::abs(m_mat[0][1] * v.y) +
					    std::abs(m_mat[0][2] * v.z));
        absError->y =
            (gamma(3) + (T)1) *
            (std::abs(m_mat[1][0]) * vError.x + std::abs(m_mat[1][1]) * vError.y +
                std::abs(m_mat[1][2]) * vError.z) +
            gamma(3) * (std::abs(m_mat[1][0] * v.x) + std::abs(m_mat[1][1] * v.y) +
                std::abs(m_mat[1][2] * v.z));
        absError->z =
            (gamma(3) + (T)1) *
            (std::abs(m_mat[2][0]) * vError.x + std::abs(m_mat[2][1]) * vError.y +
                std::abs(m_mat[2][2]) * vError.z) +
            gamma(3) * (std::abs(m_mat[2][0] * v.x) + std::abs(m_mat[2][1] * v.y) +
                std::abs(m_mat[2][2] * v.z));
        return Vector3<T>(m_mat[0][0] * x + m_mat[0][1] * y + m_mat[0][2] * z,
            m_mat[1][0] * x + m_mat[1][1] * y + m_mat[1][2] * z,
            m_mat[2][0] * x + m_mat[2][1] * y + m_mat[2][2] * z);
    }

    inline Ray Transform::transformRay(const Ray& r, Vector3f* oError,
        Vector3f* dError) const 
	{
		Vector3f o  = transformPoint(r.m_origin, oError);//(*this)(r.o, oError);
		Vector3f d = transformVector(r.m_dir, dError);//(*this)(r.d, dError);
		float tMax = r.m_maxT;
        float lengthSquared = LengthSqr(d);
        if (lengthSquared > 0) {
            float dt = Dot(Abs(d), *oError) / lengthSquared;
            o += d * dt;
            //        tMax -= dt;
        }
        return Ray(o, d, tMax, r.m_time, r.m_medium);
    }

    inline Ray Transform::transformRay(const Ray& r, const Vector3f& oErrorIn,
        const Vector3f& dErrorIn, Vector3f* oErrorOut,
        Vector3f* dErrorOut) const 
	{
        Vector3f o  = transformPoint(r.m_origin, oErrorIn, oErrorOut);//(*this)(r.o, oError);
        Vector3f d  = transformVector(r.m_dir, dErrorIn,dErrorOut);//(*this)(r.d, dError);


        float tMax = r.m_maxT;
        float lengthSquared = LengthSqr(d);
        if (lengthSquared > 0) {
            float dt = Dot(Abs(d), *oErrorOut) / lengthSquared;
            o += d * dt;
            //        tMax -= dt;
        }
        return Ray(o, d, tMax, r.m_time, r.m_medium);       
    }

	Transform Inverse(const Transform& _v);

	
	Matrix4x4 Inverse4x4(const Matrix4x4& _in);
	Matrix4x4 Transpose4x4(const Matrix4x4& _in);
	Matrix4x4 Translate4x4(const Vector3f& _v);
	Matrix4x4 Scale4x4(const Vector3f& _v);
    Matrix4x4 RotateX4x4(float angle);
    Matrix4x4 RotateY4x4(float angle);
    Matrix4x4 RotateZ4x4(float angle);
    Matrix4x4 Rotate4x4(float angle, const Vector3f& axis);
    Matrix4x4 LookAt4x4(const Vector3f& pos, const Vector3f& look, const Vector3f& up);

    Matrix4x4 Perspective4x4(float fov, float aspect, float znear, float zfar);
	Matrix4x4 Orthographic4x4(float _left, float _right, float _top, float _bottom, float _near, float _far);
	Matrix4x4 Orthographic4x4( const Vector2i& _screenDims );

	Matrix4x4 GetIdentity();

	Vector3f  GetForward (const Matrix4x4& _m);
	Vector3f  GetRight (const Matrix4x4& _m);
	Vector3f  GetUp   (const Matrix4x4& _m);
	Vector3f  GetPos  (const Matrix4x4& _m);


    void      SetForward(const Vector3f& _v, Matrix4x4& _m);
    void      SetRight(const Vector3f& _v, Matrix4x4& _m);
    void      SetUp(  const Vector3f& _v, Matrix4x4& _m);
    void      SetPos( const Vector3f& _v, Matrix4x4& _m);



		

}