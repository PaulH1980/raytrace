#include <glm/gtx/matrix_decompose.hpp>
#include "Transform.h"
#include "Interaction.h"
#include "BBox.h"

namespace RayTrace {

	Transform Translate(const Vector3f& delta)
	{
		Matrix4x4 data = { 1, 0, 0, delta.x,
						   0, 1, 0, delta.y,
						   0, 0, 1, delta.z,
						   0, 0, 0, 1
		};
		Matrix4x4 invData = { 1, 0, 0, -delta.x,
							  0, 1, 0, -delta.y,
							  0, 0, 1, -delta.z,
							  0, 0, 0, 1
		};
		return Transform(data, invData);
	}

	Transform Scale(float x, float y, float z)
	{
		Matrix4x4 data = { x, 0, 0, 0,
						   0, y, 0, 0,
						   0, 0, z, 0,
						   0, 0, 0, 1
		};
		Matrix4x4 invData = { 1 / x, 0, 0, 0,
							  0, 1 / y, 0, 0,
							  0, 0, 1 / z, 0,
							  0, 0, 0,   1
		};

		return Transform(data, invData);
	}

	Transform RotateX(float angle)
	{
		auto mat = Rotate4x4(angle, { 1.f, 0.f, 0.f });
		return Transform(mat, glm::transpose(mat ));
	}

	Transform RotateY(float angle)
	{
		auto mat = Rotate4x4(angle, { 0.f, 1.f, 0.f });
		return Transform(mat, glm::transpose(mat));
	}

	Transform RotateZ(float angle)
	{
        auto mat = Rotate4x4(angle, { 0.f, 0.f, 1.f });
        return Transform(mat, glm::transpose(mat));
	}

	Transform Rotate(float angle, const Vector3f& axis)
	{
		Vector3f a = Normalize( axis );
		float s = sinf(ToRadians(angle));
		float c = cosf(ToRadians(angle));
		float m[4][4];

		m[0][0] = a.x * a.x + (1.f - a.x * a.x) * c;
		m[0][1] = a.x * a.y * (1.f - c) - a.z * s;
		m[0][2] = a.x * a.z * (1.f - c) + a.y * s;
		m[0][3] = 0;

		m[1][0] = a.x * a.y * (1.f - c) + a.z * s;
		m[1][1] = a.y * a.y + (1.f - a.y * a.y) * c;
		m[1][2] = a.y * a.z * (1.f - c) - a.x * s;
		m[1][3] = 0;

		m[2][0] = a.x * a.z * (1.f - c) - a.y * s;
		m[2][1] = a.y * a.z * (1.f - c) + a.x * s;
		m[2][2] = a.z * a.z + (1.f - a.z * a.z) * c;
		m[2][3] = 0;

		m[3][0] = 0;
		m[3][1] = 0;
		m[3][2] = 0;
		m[3][3] = 1;

		Matrix4x4 mat = glm::make_mat4((const float*)&m);
		return Transform(mat, glm::transpose( mat ));
	}

	Transform LookAt(const Vector3f& pos, const Vector3f& look, const Vector3f& up)
	{
       auto mat = glm::lookAt(pos, look, up);        
	   return Transform(mat, inverse(mat));
	}

	Transform Orthographic(float znear, float zfar)
	{
		return Scale(1.f, 1.f, 1.f / (zfar - znear)) *
			   Translate(Vector3f(0.f, 0.f, -znear));
	}

   

    Matrix4x4 Orthographic4x4(const Vector2i& _screenDims)
    {
       
		float w = _screenDims.x;
		float h = _screenDims.y;

		return glm::ortho(0.f, w, 0.f, h, -1.0f, 1.0f);

    }

    Matrix4x4 GetIdentity()
    {
		Matrix4x4 identity(1.0f);
		return identity;
    }

    Vector3f GetForward(const Matrix4x4& _m)
    {
        Vector3f ret = {
             _m[0][2],
             _m[1][2],
             _m[2][2]
        };
        return ret;
    }

    Vector3f GetRight(const Matrix4x4& _m)
    {
        Vector3f ret = {
             _m[0][0],
             _m[1][0],
             _m[2][0]
		};
        return ret;
    }

    Vector3f GetUp(const Matrix4x4& _m)
    {
        Vector3f ret = {
          _m[0][1],
          _m[1][1],
          _m[2][1]
        };
		return ret;
    }

    Vector3f GetPos(const Matrix4x4& _m)
    {
        Vector3f ret = {
          _m[0][3],
          _m[1][3],
          _m[2][3]
        };
        return ret;
    }

   
    void SetRight(const Vector3f& _v, Matrix4x4& _m)
    {
        _m[0][0] = _v[0];
        _m[1][0] = _v[1];
        _m[2][0] = _v[2];       
    }

    void SetUp(const Vector3f& _v, Matrix4x4& _m)
    {
        _m[0][1] = _v[0];
        _m[1][1] = _v[1];
        _m[2][1] = _v[2];     
    }

    void SetForward(const Vector3f& _v, Matrix4x4& _m)
    {
        _m[0][2] = _v[0];
        _m[1][2] = _v[1];
        _m[2][2] = _v[2];
    }


    void SetPos(const Vector3f& _v, Matrix4x4& _m)
    {
        _m[0][3] = _v[0];
        _m[1][3] = _v[1];
        _m[2][3] = _v[2];
        _m[3][3] = 1.0f;
    }


	bool SolveLinearSystem2x2(const float A[2][2], const float B[2], float* x0, float* x1)
	{
		float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
		if (std::fabs(det) < 1e-10f)
			return false;
		*x0 = (A[1][1] * B[0] - A[0][1] * B[1]) / det;
		*x1 = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
		if (std::isnan(*x0) || std::isnan(*x1))
			return false;
		return true;
	}

    Transform Inverse(const Transform& _v)
    {
        return _v.inverted();
    }

    Matrix4x4 Inverse4x4(const Matrix4x4& _in)
    {
		return glm::inverse(_in);
    }

    Matrix4x4 Transpose4x4(const Matrix4x4& _in)
    {
		return glm::transpose(_in);
    }

    Matrix4x4 Translate4x4(const Vector3f& _v)
    {
       /* Matrix4x4 data = { 1, 0, 0, _v.x,
                           0, 1, 0, _v.y,
                           0, 0, 1, _v.z,
                           0, 0, 0, 1 };
        return data;*/

		return glm::translate(Matrix4x4(1.0f), _v);
    }

    Matrix4x4 Scale4x4(const Vector3f& _v)
    {
		return glm::scale(_v);
    }

    Matrix4x4 RotateX4x4(float angle)
    {
		return glm::rotate(ToRadians(angle), Vector3f(1.f, 0.f, 0.f));
    }

    Matrix4x4 RotateY4x4(float angle)
    {
		return glm::rotate(ToRadians(angle), Vector3f(0.f, 1.f, 0.f));
    }

    Matrix4x4 RotateZ4x4(float angle)
    {
		return glm::rotate(ToRadians(angle), Vector3f(0.f, 0.f, 1.f));	
    }

    Matrix4x4 Rotate4x4(float angle, const Vector3f& axis)
    {
		Vector3f a = Normalize(axis);
        float s = sinf(ToRadians(angle));
        float c = cosf(ToRadians(angle));
        float m[4][4];

        m[0][0] = a.x * a.x + (1.f - a.x * a.x) * c;
        m[0][1] = a.x * a.y * (1.f - c) - a.z * s;
        m[0][2] = a.x * a.z * (1.f - c) + a.y * s;
        m[0][3] = 0;

        m[1][0] = a.x * a.y * (1.f - c) + a.z * s;
        m[1][1] = a.y * a.y + (1.f - a.y * a.y) * c;
        m[1][2] = a.y * a.z * (1.f - c) - a.x * s;
        m[1][3] = 0;

        m[2][0] = a.x * a.z * (1.f - c) - a.y * s;
        m[2][1] = a.y * a.z * (1.f - c) + a.x * s;
        m[2][2] = a.z * a.z + (1.f - a.z * a.z) * c;
        m[2][3] = 0;

        m[3][0] = 0;
        m[3][1] = 0;
        m[3][2] = 0;
        m[3][3] = 1;

        Matrix4x4 mat  = glm::make_mat4((const float*) m);
		return mat;
    }

    Matrix4x4 LookAt4x4(const Vector3f& pos, const Vector3f& look, const Vector3f& up)
    {
		return glm::lookAt(pos, look, up);
    }


    Matrix4x4 Perspective4x4(float fov, float aspect, float znear, float zfar)
    {
		return glm::perspective(fov, aspect, znear, zfar);
    }

    Matrix4x4 Orthographic4x4(float _left, float _right, float _top, float _bottom, float _near, float _far)
    {
		return glm::ortho(_left, _right, _bottom, _top, _near, _far);
    }

    Transform Transform::operator*(const Transform& _rhs) const noexcept
	{
		Matrix4x4 m1 = m_mat *_rhs.m_mat; 
		Matrix4x4 m2 = _rhs.m_invMat * (m_invMat);
		return Transform(m1, m2);
	}

    Transform::Transform(const Matrix4x4& _data) : m_mat(_data)
        , m_invMat(glm::inverse(_data))
    {
		m_invMat = Transpose4x4(m_invMat );
		m_mat    = Transpose4x4(m_mat );
    }

    Transform::Transform(const Matrix4x4& _data, const Matrix4x4& _invData) : m_mat(_data)
        , m_invMat(_invData)
    {
        m_invMat = Transpose4x4(m_invMat);
        m_mat = Transpose4x4(m_mat);
    }

    Transform::Transform(const Transform& _rhs)
    {
        m_mat = _rhs.m_mat;
        m_invMat = _rhs.m_invMat;
    }

    SurfaceInteraction Transform::transformSurfInteraction(const SurfaceInteraction& si) const
    {
		SurfaceInteraction ret;
        // Transform _p_ and _pError_ in _SurfaceInteraction_
		ret.m_p = transformPoint(si.m_p, si.m_pError, &ret.m_pError);//(*this)(si.p, si.pError, &ret.pError);

        // Transform remaining members of _SurfaceInteraction_
        ret.m_n  = Normalize(transformNormal(si.m_n));
        ret.m_wo = transformVector(si.m_wo);
        ret.m_time = si.m_time;
        ret.m_mediumInterface = si.m_mediumInterface;
        ret.m_uv = si.m_uv;
        ret.m_shape = si.m_shape;
        ret.m_dpdu = transformVector(si.m_dpdu);
        ret.m_dpdv = transformVector(si.m_dpdv);
        ret.m_dndu = transformNormal(si.m_dndu);
        ret.m_dndv = transformNormal(si.m_dndv);
		ret.shading.m_n =  Normalize(transformNormal(si.shading.m_n));//Normalize(t(si.shading.n));
        ret.shading.m_dpdu = transformVector(si.shading.m_dpdu);
        ret.shading.m_dpdv = transformVector(si.shading.m_dpdv);
        ret.shading.m_dndu = transformNormal(si.shading.m_dndu);
        ret.shading.m_dndv = transformNormal(si.shading.m_dndv);
        ret.m_dudx = si.m_dudx;
        ret.m_dvdx = si.m_dvdx;
        ret.m_dudy = si.m_dudy;
        ret.m_dvdy = si.m_dvdy;
        ret.m_dpdx = transformVector(si.m_dpdx);
        ret.m_dpdy = transformVector(si.m_dpdy);
        ret.m_bsdf = si.m_bsdf;
        ret.m_bssrdf = si.m_bssrdf;
        ret.m_primitive = si.m_primitive;
        ret.shading.m_n = FaceForward(ret.shading.m_n, ret.m_n);
        ret.m_faceIndex = si.m_faceIndex;
        return ret;
    }
	

	AnimatedTransform::AnimatedTransform(const Transform* _start, float _time1, const Transform* _end, float _time2)
		: m_start( _start )
		, m_startTime( _time1 )
		, m_end( _end )
		, m_endTime( _time2 )
	{
		m_isAnimated = *m_start != *m_end;
		Decompose(m_start->m_mat, &m_trans[0], &m_rot[0], &m_scale[0]);
		Decompose(m_end->m_mat,   &m_trans[1], &m_rot[1], &m_scale[1]);
	}

	void AnimatedTransform::Decompose(const Matrix4x4& m, Vector3f* _transOut, Quatf* _rotOut, Vector3f* _scaleOut)
	{
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
		glm::decompose(m, scale, rotation, translation, skew, perspective);

		*_transOut = translation;
		*_rotOut = rotation;
		*_scaleOut = scale;	
	}

	void AnimatedTransform::interpolate(float time, Transform* _tOut) const
	{
		if (time <= m_startTime || !m_isAnimated) {
			*_tOut = *m_start;
			return;
		}
		if (time >= m_endTime) {
			*_tOut = *m_end;
			return;
		}
		//else
		{
			float dt = (time - m_startTime) / (m_endTime - m_startTime);		
		
			Matrix4x4 scaleMat = Scale4x4(Lerp(m_scale[0], m_scale[1], dt));
			Matrix4x4 rotMat   = glm::toMat4(glm::mix(m_rot[0], m_rot[1], dt));
			Matrix4x4 transMat = Translate4x4(Lerp(m_trans[0], m_trans[1], dt));

			*_tOut = Transform(transMat * rotMat * scaleMat);		
		}
	}

	Ray AnimatedTransform::interpolateRay(const Ray& r) const noexcept
	{
		if (r.m_time <= m_startTime || !m_isAnimated)
			return m_start->transformRay(r);
		if (r.m_time >= m_endTime)
			return m_end->transformRay(r);
		Transform transform;
		interpolate(r.m_time, &transform);
		return transform.transformRay(r);
	}

	RayDifferential AnimatedTransform::interpolateRayDifferential(const RayDifferential& r) const noexcept
	{
		if (r.m_time <= m_startTime || !m_isAnimated)
			return m_start->transformRayDifferential(r);
		if (r.m_time >= m_endTime)
			return m_end->transformRayDifferential(r);
		Transform transform;
		interpolate(r.m_time, &transform);
		return transform.transformRayDifferential(r);
	}

	Vector3f AnimatedTransform::interpolatePoint(const Vector3f& _rhs, float _time) const noexcept
	{
		if (_time <= m_startTime || !m_isAnimated)
			return m_start->transformPoint(_rhs);
		if (_time >= m_endTime)
			return m_end->transformPoint(_rhs);
		Transform transform;
		interpolate(_time, &transform);
		return transform.transformPoint(_rhs);
	}

	Vector3f AnimatedTransform::interpolateVector(const Vector3f& _rhs, float _time) const noexcept
	{
		if (_time <= m_startTime || !m_isAnimated)
			return m_start->transformVector(_rhs);
		if (_time >= m_endTime)
			return m_end->transformVector(_rhs);
		Transform transform;
		interpolate(_time, &transform);
		return transform.transformVector(_rhs);
	}

	BBox3f AnimatedTransform::motionBounds(const BBox3f& b) const
	{
        if (!m_isAnimated)
            return m_start->transformBounds(b);
        BBox3f ret;
		const int nSteps = 128;
		for (int i = 0; i < nSteps; ++i) {
			Transform t;
			float time = Lerp( m_startTime, m_endTime, float(i) / float(nSteps - 1) );
			interpolate(time, &t);			
			ret.addBounds(t.transformBounds(b));
		}
		return ret;
		
	}

}
