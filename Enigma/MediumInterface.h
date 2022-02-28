#pragma once
#include "Defines.h"
#include "Transform.h"
#include "Spectrum.h"
#include "BBox.h"

namespace RayTrace
{
    
    bool  GetMediumScatteringProperties(const std::string& name, Spectrum* sigma_a, Spectrum* sigma_s);
    float PhaseHG(float cosTheta, float g);
    
    // Media Declarations
    class PhaseFunction {
    public:
        // PhaseFunction Interface
        virtual ~PhaseFunction();
        virtual float p(const Vector3f& wo, const Vector3f& wi) const = 0;
        virtual float Sample_p(const Vector3f& wo, Vector3f* wi, const Vector2f& u) const = 0;
       
    };

    // HenyeyGreenstein Declarations
    class HenyeyGreenstein : public PhaseFunction {
    public:
        // HenyeyGreenstein Public Methods
        HenyeyGreenstein(float _g) : m_g(_g) {}
        float p(const Vector3f& wo, const Vector3f& wi) const override;
        float Sample_p(const Vector3f& wo, Vector3f* wi, const Vector2f& sample) const override;
    private:
        const float m_g;
    };

    // MediumInterface Declarations
    struct MediumInterface {
        MediumInterface() : inside(nullptr), outside(nullptr) {}
        // MediumInterface Public Methods
        MediumInterface(const Medium* medium) : inside(medium), outside(medium) {}
        MediumInterface(const Medium* inside, const Medium* outside)
            : inside(inside), outside(outside) {}
        bool IsMediumTransition() const { return inside != outside; }
        const Medium* inside, * outside;
    };






   

}