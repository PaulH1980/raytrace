#pragma once

#include "Defines.h"
#include "Ray.h"
#include "MathCommon.h"

namespace RayTrace
{

	//////////////////////////////////////////////////////////////////////////
	//class BBox2
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	class BBox2
	{
	public:

	
        BBox2()  {
            constexpr T minNum = std::numeric_limits<T>::lowest();
            constexpr T maxNum = std::numeric_limits<T>::max();

			m_min = { maxNum };
			m_max = { minNum };
		
		
		};

        BBox2(const Vector2<T>& _minMax)
            : m_min(_minMax)
            , m_max(_minMax)
        {
        }

        BBox2(const Vector2<T>& _p1, const Vector2<T>& _p2)
            :m_min( Min( _p1, _p2 ) )
            ,m_max( Max( _p1, _p2 ) )
        {
        }

		template <typename U>
		explicit BBox2(const BBox2<U>& _rhs)
			: m_min( Vector2<U>( _rhs.m_min ) )
			, m_max( Vector2<U>( _rhs.m_max ) )
		{			
		}

        template <typename U>
        explicit operator BBox2<U>() const {
            return BBox2<U>((Vector2<U>)m_min, (Vector2<U>)m_max);
        }

		Vector2<T> diagonal() const  noexcept {
			return m_max - m_min; 
		}

        T area() const noexcept {
			Vector2<T> d = diagonal();
            return (d.x * d.y);
        }

		eAxis maximumExtent() const noexcept {
            Vector2 diag = diagonal();
            if (diag.x > diag.y)
                return eAxis::AXIS_X;
            else
                return eAxis::AXIS_Y;
        }

        inline const Vector2<T>& operator[](int i) const noexcept {
            return (i == 0) ? m_min: m_max;
        }

        inline Vector2<T> operator[](int i) noexcept {
			return (i == 0) ? m_min : m_max;
        }

        bool operator==(const  BBox2<T>& b) const noexcept {
            return b.m_min == m_min && 
				   b.m_max == m_max;
        }
		 
        bool operator!=(const BBox2<T>& b) const noexcept {
			return !(*this == b);
        }

		Vector2<T> lerp(const Vector2<T>& t) const noexcept {
            return Vector2<T>(Lerp( m_min.x, m_max.x, t.x),
						      Lerp( m_min.y, m_max.y, t.y));
        }
		Vector2<T> offset(const Vector2<T>& p) const noexcept {
			Vector2<T> o = p - m_min;
            if (m_max.x > m_min.x) o.x /= m_max.x - m_min.x;
            if (m_max.y > m_min.y) o.y /= m_max.y - m_min.y;
            return o;
        }

        void boundingSphere(Vector2<T>* c, float* rad) const noexcept {
            *c = (m_min + m_max) / 2;
            *rad = inside(*c ) ? (*c - m_max ).length() : 0;
        }
        
        bool inside(const Vector2<T>& pt) const noexcept {
            return (pt.x >= m_min.x && 
				    pt.x <= m_max.x &&
				    pt.y >= m_min.y &&
					pt.y <= m_max.y);
        }

		bool insideExclusive(const Vector2<T>& pt) const {
			return (pt.x >= m_min.x &&
					pt.x <  m_max.x &&
					pt.y >= m_min.y &&
					pt.y <  m_max.y);
		}

      
        Vector2<T>  m_min;
        Vector2<T>  m_max;
	};


	//////////////////////////////////////////////////////////////////////////
	//class BBox3
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	class BBox3
	{
	public:
	
		BBox3() {
            constexpr T minNum = std::numeric_limits<T>::lowest();
            constexpr T maxNum = std::numeric_limits<T>::max();

            m_min = { maxNum };
            m_max = { minNum };

		
		};

		BBox3(const Vector3<T>& _minMax)
			: m_min(_minMax)
			, m_max(_minMax)
		{
		}

		BBox3(const Vector3<T>& _p1, const Vector3<T>& _p2)
			: m_min(Min(_p1, _p2))
			, m_max(Max(_p1, _p2))
		{

		}




		void expand(T _delta) noexcept {
			for (int i = 0; i < 3; ++i) {
				m_min[i] -= _delta;
				m_max[i] += _delta;
			}
		}

		void addPoint( const Vector3<T>& _point ) noexcept {
			for (int i = 0; i < 3; ++i){
				m_min[i] = std::min(m_min[i], _point[i]);
				m_max[i] = std::max(m_max[i], _point[i]);
			}
		}

		void addBounds(const BBox3<T>& _b) noexcept
		{
			addPoint(_b.m_min);
			addPoint(_b.m_max);
		}

		BBox3<T> added(const Vector3<T>& _point) const noexcept {
			BBox3<T> retVal = *this;
			retVal.addPoint(_point);
			return retVal;
		}

		BBox3<T> added(const BBox3<T>& _b) const noexcept
		{
			BBox3<T> retVal = *this;
			retVal.addBounds(_b);
			return retVal;
		}

		bool intersects(const BBox3<T>& _rhs) const noexcept {
			for (int i = 0; i < 3; ++i) {
				if (_rhs.m_min[i] > m_max[i] || _rhs.m_max[i] < m_min[i])
					return false;
			}
			return true;
		}


		Vector3<T> diagonal() const noexcept
		{
			return m_max - m_min;
		}


		eAxis maximumExtent() const  noexcept {
			Vector3<T> diag = diagonal();
			if (diag.x > diag.y && diag.x > diag.z)
				return eAxis::AXIS_X;
			else if (diag.y > diag.z)
				return eAxis::AXIS_Y;
			else
				return eAxis::AXIS_Z;
		}


		bool inside(const Vector3<T>& _point) const noexcept  {
            return (_point.x >= m_min.x &&
					_point.x <= m_max.x &&
					_point.y >= m_min.y &&
					_point.y <= m_max.y &&   
					_point.z >= m_min.z &&
					_point.z <= m_max.z );
		}

        bool insideExlusive(const Vector3<T>& _point) const noexcept {
            return (_point.x >= m_min.x &&
					_point.x <  m_max.x &&
					_point.y >= m_min.y &&
					_point.y <  m_max.y &&
					_point.z >= m_min.z &&
					_point.z <  m_max.z);
        }


       Vector3<T> offset(const Vector3<T>& p) const  noexcept {
		   Vector3<T> o = p - m_min;
			if (m_max.x > m_min.x) o.x /= m_max.x - m_min.x;
			if (m_max.y > m_min.y) o.y /= m_max.y - m_min.y;
			if (m_max.z > m_min.z) o.z /= m_max.z - m_min.z;				
			return o;
        }

		T surfaceArea() const noexcept {
			const auto d = diagonal();
			return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
		}

		T volume() const noexcept {
			const auto d = diagonal();
			return d.x * d.y * d.z;
		}

		Vector3<T> lerp(T _tx, T _ty,T _tz) const noexcept {
			return Vector3<T>( Lerp( m_min[0], m_max[0], _tx),
						       Lerp( m_min[1], m_max[1], _ty), 
				               Lerp( m_min[2], m_max[2], _tz));
		}

		Vector3<T>	lerp(const Vector3<T>& v) const noexcept {
			return lerp(v[0], v[1], v[2]);
		}

		Vector3<T> getCenter() const noexcept {
			return  (m_min * .5f) + (m_max * .5f);
		}
       
		inline bool intersectP(const Ray& ray, const Vector3f& invDir, const int dirIsNeg[3]) const
		{
            const BBox3& bounds = *this;
            // Check for ray intersection against $x$ and $y$ slabs
            float tMin  = (bounds[dirIsNeg[0]].x - ray.m_origin.x) * invDir.x;
            float tMax  = (bounds[1 - dirIsNeg[0]].x - ray.m_origin.x) * invDir.x;

            float tyMin = (bounds[dirIsNeg[1]].y - ray.m_origin.y) * invDir.y;
            float tyMax = (bounds[1 - dirIsNeg[1]].y - ray.m_origin.y) * invDir.y;

            // Update _tMax_ and _tyMax_ to ensure robust bounds intersection
            tMax  *= 1 + 2 * gamma(3);
            tyMax *= 1 + 2 * gamma(3);
            if (tMin > tyMax || tyMin > tMax) return false;
            if (tyMin > tMin) tMin = tyMin;
            if (tyMax < tMax) tMax = tyMax;

            // Check for ray intersection against $z$ slab
            float tzMin = (bounds[dirIsNeg[2]].z - ray.m_origin.z) * invDir.z;
            float tzMax = (bounds[1 - dirIsNeg[2]].z - ray.m_origin.z) * invDir.z;

            // Update _tzMax_ to ensure robust bounds intersection
            tzMax *= 1 + 2 * gamma(3);
            if (tMin > tzMax || tzMin > tMax) 
				return false;
            if (tzMin > tMin) tMin = tzMin;
            if (tzMax < tMax) tMax = tzMax;
            return (tMin < ray.m_maxT) && (tMax > 0);
		}


        bool intersectP(const Ray& ray, float* hitt0 /*= NULL*/, float* hitt1 /*= NULL*/) const
        {
            float t0 = 0,
				  t1 = ray.m_maxT;
            for (int i = 0; i < 3; ++i) {
                // Update interval for _i_th bounding box slab
                float invRayDir = 1 / ray.m_dir[i];
                float tNear = (m_min[i] - ray.m_origin[i]) * invRayDir;
                float tFar  = (m_max[i] - ray.m_origin[i]) * invRayDir;

                // Update parametric interval from slab intersection $t$ values
                if (tNear > tFar) std::swap(tNear, tFar);

                // Update _tFar_ to ensure robust ray--bounds intersection
                tFar *= 1 + 2 * gamma(3);
                t0 = tNear > t0 ? tNear : t0;
                t1 = tFar < t1 ? tFar : t1;
                if (t0 > t1) return false;
            }
            if (hitt0) *hitt0 = t0;
            if (hitt1) *hitt1 = t1;
            return true;
        }

        void getBoundingSphere(Vector3<T>& center, T& radius) const noexcept
        {
            center = getCenter();
            radius = inside(center) ? (center - m_max).length() : 0.0f;           
        }

        const Vector3<T>& operator[](int i) const noexcept
        {
            assert(i == 0 || i == 1);
            if (i == 0)
                return m_min;
            return m_max;
        }

		Vector3<T>& operator[](int i) noexcept
        {
            assert(i == 0 || i == 1);
            if (i == 0)
                return m_min;
            return m_max;
        }

		Vector3<T>  m_min;
		Vector3<T>  m_max;
	};




	template<typename T>
	BBox2<T> Intersection(const BBox2<T>& _a, const BBox2<T>& _b) {
        BBox2<T> ret;
		ret.m_min = Max(_a.m_min, _b.m_min);    
		ret.m_max = Min(_a.m_max, _b.m_max);
        return ret;
	}

    template<typename T>
    BBox3<T> Intersection(const BBox3<T>& _a, const BBox3<T>& _b) {
		BBox3<T> ret;
        ret.m_min = Max(_a.m_min, _b.m_min);
        ret.m_max = Min(_a.m_max, _b.m_max);
        return ret;
    }

    template<typename T>
    BBox3<T> Union(const BBox3<T>& _a, const BBox3<T>& _b) {
		BBox3<T> ret;
		ret.m_min = Min(_a.m_min, _b.m_min);
		ret.m_max = Max(_a.m_max, _b.m_max);		
		return ret;      
    }

    template<typename T>
    BBox3<T> Union(const BBox3<T>& _a, const Vector3<T>& _b) {
        BBox3<T> ret;
        ret.m_min = Min(_a.m_min, _b);
        ret.m_max = Max(_a.m_max, _b);
        return ret;
    }

    template<typename T>
    BBox3<T> Union(const BBox3<T>& _a, const Vector3<T>* _pVerts, uint32_t _numVerts) 
	{
		BBox3<T> ret = _a;
		for (uint32_t i = 0; i < _numVerts; ++i) {
            ret.m_min = Min(ret.m_min, _pVerts[i]);
            ret.m_max = Max(ret.m_max, _pVerts[i]);
		}
		return ret;
	}

    template<typename T>
    BBox3<T> GetBounds( const Vector3<T>* _pVerts, uint32_t _numVerts)
    {
        BBox3<T> ret;
        for (uint32_t i = 0; i < _numVerts; ++i) {
            ret.m_min = Min(ret.m_min, _pVerts[i]);
            ret.m_max = Max(ret.m_max, _pVerts[i]);
        }
        return ret;
    }
}