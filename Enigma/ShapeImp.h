#pragma once
#include "Shape.h"


namespace RayTrace
{
	class Sphere : public Shape
	{
	public:
		Sphere( const Transform* _o2w, const Transform* _w2o,
			    bool _reverseOrientation, 
			    float _radius, 
			    float _zMin, 
			    float _zMax,
			    float _phiMax );


        BBox3f	        objectBound() const override;
        bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const override;
        bool			        intersectP(const Ray& ray, bool testAlphaTexture) const override;
        float			        area() const override;
        Interaction		sample(const Vector2f& u, float* pdf) const override;
        Interaction		sample(const Interaction& ref, const Vector2f& u, float* pdf) const override;
        float			        pdf(const Interaction& ref, const Vector3f& wi) const override;
        float			        solidAngle(const Vector3f& p, int nSamples) const override;

	private:
		
		float m_radius;
		float m_phiMax;
		float m_zMin, m_zMax;
		float m_thetaMin, m_thetaMax;

	};

	class Cylinder : public Shape {
	public:

		Cylinder(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation,
				 float _radius, float _zMin, float _zMax, float _phiMax);

        BBox3f	        objectBound() const;
        bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
        bool			        intersectP(const Ray& ray, bool testAlphaTexture) const;
        float			        area() const;
        Interaction		sample(const Vector2f& u, float* pdf) const;   


	private:
		float m_radius, m_zMin, m_zMax, m_phiMax;

	};

	class Disk : public Shape {
	public:
		Disk(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation,
			 float _radius, float _innerRadius = 0.f, float _phiMax = 0.f, float _height = 0.f);

        BBox3f	        objectBound() const;
        bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
        bool			        intersectP(const Ray& ray, bool testAlphaTexture) const;
        float			        area() const;
        Interaction		sample(const Vector2f& u, float* pdf) const;
      
	private:
		
		float m_radius;
		float m_innerRadius;
		float m_phiMax;
		float m_height;
	};

    /*class Cone : public Shape {
    public:
        Cone(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation,
            float _radius, float _phiMax = 0.f, float _height = 0.f);

        BBox3f	        objectBound() const;
        bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
        bool			        intersectP(const Ray& ray, bool testAlphaTexture) const;
        float			        area() const;
        Interaction		sample(const Vector2f& u, float* pdf) const;

    private:
        const float m_radius,
                    m_height,
                    m_phiMax;
    };*/

    //class Paraboloid : public Shape {
    //public:
    //    BBox3f	        objectBound() const;
    //    bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
    //    bool			        intersectP(const Ray& ray, bool testAlphaTexture) const;
    //    float			        area() const;
    //    Interaction		sample(const Vector2f& u, float* pdf) const;

    //private:
    //    const float m_radius, 
    //                m_zMin, 
    //                m_zMax, 
    //                m_phiMax;
    //};

    //class Hyperboloid : public Shape {
    //public:
    //    BBox3f	        objectBound() const;
    //    bool			        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
    //    bool			        intersectP(const Ray& ray, bool testAlphaTexture) const;
    //    float			        area() const;
    //    Interaction		sample(const Vector2f& u, float* pdf) const;
    //private:

    //    Vector3f m_p1, m_p2;
    //    float m_zMin
    //        , m_zMax;
    //    float m_phiMax;
    //    float m_rMax;
    //    float m_ah, 
    //          m_ch;
    //};

  


#pragma region CreateShapes
   std::shared_ptr<Shape>			CreateSphereShape(const Transform* _o2w, const Transform* _w2o, 
										bool _reverseOrientation, const ParamSet& _set);

   std::shared_ptr<Shape>			CreateCylinderShape(const Transform* _o2w, const Transform* _w2o,
        bool _reverseOrientation, const ParamSet& _set);

   std::shared_ptr<Shape>			CreateDiskShape(const Transform* _o2w, const Transform* _w2o,
       bool _reverseOrientation, const ParamSet& _set);

   /*std::shared_ptr<Shape>			CreateConeShape(const Transform* _o2w, const Transform* _w2o,
       bool _reverseOrientation, const ParamSet& _set);

   std::shared_ptr<Shape>			CreateParaboloidShape(const Transform* _o2w, const Transform* _w2o,
       bool _reverseOrientation, const ParamSet& _set);

   std::shared_ptr<Shape>			CreateHyperboloidShape(const Transform* _o2w, const Transform* _w2o,
       bool _reverseOrientation, const ParamSet& _set);*/


#pragma endregion






}