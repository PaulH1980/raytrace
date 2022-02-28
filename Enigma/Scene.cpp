#include "Ray.h"
#include "Volume.h"
#include "Lights.h"
#include "Primitive.h"
#include "BVH.h"
#include "Scene.h"






namespace RayTrace
{

    

    
    Scene::Scene(const PrimitivePtr& _accel, const LightVector _lights)
        : m_accel(_accel)
        , m_lights(_lights)
    {
        m_bounds = m_accel->worldBound();
        for (const auto& light : m_lights) 
        {
            light->Preprocess(*this);
            if (light->m_flags & (int)eLightFlags::LIGHTFLAG_INFINITE)
                m_infiniteLights.push_back(light);
        }
    }


    bool Scene::intersect(const Ray& _ray, SurfaceInteraction* _isect) const
    {
        assert(LengthSqr(_ray.m_dir ) > 0.f);
        return m_accel->intersect(_ray, _isect);
    }

    bool Scene::intersectP(const Ray& _ray) const
    {
        assert(LengthSqr(_ray.m_dir) > 0.f);
        
        return  m_accel->intersectP(_ray);       
    }

    bool Scene::intersectTr(Ray _ray, Sampler& sampler, SurfaceInteraction* _isect, Spectrum* _transmittance) const
    {
        *_transmittance = Spectrum(1.f);
        while (true) {
            bool hitSurface = intersect(_ray, _isect);
            // Accumulate beam transmittance for ray segment
            if (_ray.m_medium) 
                *_transmittance *= _ray.m_medium->Tr(_ray, sampler);

            // Initialize next ray segment or terminate transmittance computation
            if (!hitSurface) 
                return false;

            if (_isect->m_primitive->getMaterial() != nullptr) 
                return true;
            _ray = _isect->SpawnRay(_ray.m_dir );
        }
        return false;
    }

    const BBox3f& Scene::worldBound() const
    {
        return m_bounds;
    }

    int Scene::getNumLights() const
    {
        return static_cast<int>(m_lights.size());
    }

    const LightPtr& Scene::getLight(int _idx) const
    {
        return m_lights[_idx];
    }

}

