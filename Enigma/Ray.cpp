#include "Medium.h"
#include "Ray.h"

namespace RayTrace
{
    Ray::Ray()        
        : m_maxT(InfinityF32)
        , m_time(0.0f)
        , m_medium(nullptr)
    {

    }

   

    Ray::Ray(const Vector3f& _o, const Vector3f& _d, float _maxT, float _time /*= 0.0f*/, const Medium* _pMedium /*= nullptr*/)
        : m_origin( _o )
        , m_dir( _d )        
        , m_maxT(_maxT )
        , m_time(_time )
        , m_medium(_pMedium)
    {

    }

    
    Ray::Ray(const Vector3f& _o, const Vector3f& _d, float _maxT)
        : Ray( _o, _d, _maxT, 0.0f)

    {

    }

    bool Ray::hasNaNs() const noexcept
    {
        return ( HasNans( m_origin )  || HasNans( m_dir ) || isnan(m_maxT));
    }

    Vector3f Ray::scale(float _t) const noexcept
    {
        return m_origin + m_dir * _t;
    }

    RayDifferential::RayDifferential(const Ray& ray) : Ray(ray)
    {
        m_hasDifferentials = false;
    }

    RayDifferential::RayDifferential(const Vector3f& _o, const Vector3f& _d, float _tMax /*= InfinityF32*/, float _time /*= 0.f*/, const Medium* _medium /*= nullptr*/)
        : Ray(_o, _d, _tMax, _time, _medium)
    {
        m_hasDifferentials = false;
    }

    RayDifferential::RayDifferential()
    {
        m_hasDifferentials = false;
    }

    void RayDifferential::scaleDifferentials(float _s)
    {
        m_rxOrigin = m_origin + (m_rxOrigin - m_origin) * _s;
        m_ryOrigin = m_origin + (m_ryOrigin - m_origin) * _s;
        m_rxDirection = m_dir + (m_rxDirection - m_dir) * _s;
        m_ryDirection = m_dir + (m_ryDirection - m_dir) * _s;
    }

    bool RayDifferential::hasNaNs() const noexcept
    {
        return Ray::hasNaNs() ||
            (m_hasDifferentials && ( HasNans(m_rxOrigin) || HasNans( m_ryOrigin) ||
                HasNans( m_rxDirection ) || HasNans( m_ryDirection )));
    }

}

