#pragma once
#include "Defines.h"
namespace RayTrace
{

    template<typename T>
    class SphereT
    {

    public:

        SphereT() : SphereT(Vector3<T>(0.0f), 0.0f) {}
        SphereT(const Vector3<T>& _center, T _radius = 1.0f)
            : m_center(_center)
            , m_radius(_radius)
        {

        }

        T   getRadius() const { return m_radius; }
        T   getRadiusSqr() const { return m_radius * m_radius; }

        Vector3<T>       m_center;
        T                m_radius;
    };
}