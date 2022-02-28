#pragma once
#include <vector>
#include <memory>
#include "Defines.h"


namespace RayTrace
{


    bool  AddShapeToVector(const ShapePtr& _shape, ShapesVector& _shapeVector);


    class Shape
    {
    public:

        Shape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation);
        virtual ~Shape() {}



        virtual BBox3f objectBound() const = 0;
        virtual float        area() const = 0;
        virtual Interaction sample(const Vector2f& u, float* pdf) const = 0;

        virtual BBox3f worldBounds() const;
        virtual bool         canIntersect() const { return true; }
        virtual bool         canRefine() const { return false; }
        virtual void         refine(ShapesVector& _refined) {}


        virtual bool        intersect(const Ray& ray,  float* tHit, SurfaceInteraction* isect, bool testAlphaTexture = true) const = 0;
        virtual bool        intersectP(const Ray& ray, bool testAlphaTexture = true) const;

       
        // Sample a point on the shape given a reference point |ref| and
        // return the PDF with respect to solid angle from |ref|.
        virtual Interaction sample(const Interaction& ref, const Vector2f& u, float* pdf) const;
      

        virtual float pdf(const Interaction&) const { return 1 / area(); }
        virtual float pdf(const Interaction& ref, const Vector3f& wi) const;

        // Returns the solid angle subtended by the shape w.r.t. the reference
        // point p, given in world space. Some shapes compute this value in
        // closed-form, while the default implementation uses Monte Carlo
        // integration; the nSamples parameter determines how many samples are
        // used in this case.
        virtual float solidAngle(const Vector3f& p, int nSamples) const;





        /*	virtual bool intersect(const Ray& ray, float* tHit, float* rayEpsilon, DifferentialGeometry* dg) const;
            virtual bool intersectP(const Ray& ray) const;
            virtual void getShadingGeometry(const Transform& obj2world, const DifferentialGeometry& dg, DifferentialGeometry* dgShading) const;
            virtual float area() const;
            virtual Vector3f sample(float u1, float u2, Vector3f* _normal) const;
            virtual Vector3f sample(const Vector3f& _point, float u1, float u2, Vector3f* _normal) const;

            virtual float pdf(const Vector3f& _point, const Vector3f& wi) const;
            virtual float pdf(const Vector3f& Pshape) const;

            virtual void computeGradients(float u, float v, const Vector3f& phit,
                const Vector3f& dpdu, const Vector3f& dpdv,
                const Vector3f& d2Pduu, const Vector3f& d2Pduv, const Vector3f& d2Pdvv,
                DifferentialGeometry* _dg) const;*/

        const Transform *m_objectToWorld,
                              *m_worldToObject;
        const bool m_reverseOrientation,
                   m_transformSwapsHandedness;
        uint32_t		 m_shapeId;
        static uint32_t  s_nextShapeId;
    };





    //// ShapeSet Declarations
    //class ShapeSet {
    //public:
    //    // ShapeSet Public Methods
    //    ShapeSet(const ShapePtr& _shape);
    //    float Area() const { return m_sumArea; }
    //    virtual ~ShapeSet();
    //    Vector3f Sample(const Vector3f& _point, const LightSample& _ls, Vector3f* _Ns) const;
    //    Vector3f Sample(const LightSample& _ls, Vector3f* _Ns) const;
    //    float Pdf(const Vector3f& _point, const Vector3f& wi) const;
    //    float Pdf(const Vector3f& _point) const;



    //private:
    //    // ShapeSet Private Data
    //    std::vector<ShapePtr> m_shapes;
    //    std::vector<float>    m_areas;
    //    std::unique_ptr<Distribution1D> m_areaDistribution;
    //    float                 m_sumArea;
    //};


}