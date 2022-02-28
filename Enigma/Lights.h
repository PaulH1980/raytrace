#pragma once
#include <memory>
#include "Transform.h"
#include "Spectrum.h"
#include "MediumInterface.h"
#include "Defines.h"
#include "MipMap.h"
#include "Interaction.h"

namespace RayTrace
{
    class VisibilityTester;


    inline bool IsDeltaLight(int flags) {
        return flags & (int)eLightFlags::LIGHTFLAG_DELTADIRECTION ||
               flags & (int)eLightFlags::LIGHTFLAG_DELTAPOSITION;
    }
    
    using ImageDataRGBPtr = std::shared_ptr<MipMap<RGBSpectrum>>;

	class Light
	{
	public:
        Light(int flags, const Transform& LightToWorld,
            const MediumInterface& mediumInterface, int nSamples = 1);
       
        
        virtual void                        Preprocess(const Scene& scene) {}
        virtual Spectrum            Le(const RayDifferential& r) const;

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const = 0;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2, 
                                                        float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const = 0;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const = 0;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const = 0;

        virtual Spectrum            Power() const = 0;

        // Light Public Data
        const int m_flags;
        const int m_nSamples;
        const MediumInterface m_mediumInterface;
	protected:
		// Light Protected Data
		const Transform m_lightToWorld, 
							  m_worldToLight;
	};

	class PointLight : public Light
	{
	public:
       

		PointLight(const Transform& _light2world, const MediumInterface& mi, const Spectrum& _intensity);

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2, 
                                                        float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;



		Vector3f		m_pos;
		Spectrum	m_intensity;
	};

	class SpotLight : public Light
	{
	public:
		SpotLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity, float _width, float _fall);

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;


        float                               Falloff(const Vector3f& w) const;


        Vector3f		m_pos;
        Spectrum	m_intensity;
		float				m_cosTotalWidth, 
							m_cosFalloffStart;
	};

	

    class ProjectionLight : public Light
    {
    public:
		

		ProjectionLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity,
			const std::string& _texname, float _fov);

		Spectrum                    projection(const Vector3f& _w) const;

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;


		ImageDataRGBPtr		m_projMap;
        Vector3f		m_pos;
        Spectrum	m_intensity;
		Transform		m_lightProj;

        BBox2f        m_screenBounds;		
		float				m_hither, m_yon;
		float				m_cosTotalWidth;
    };
		
	

	class GonioPhotometricLight : public Light
	{
	public:
		GonioPhotometricLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity, const std::string& _texname);

        Spectrum                    scale(const Vector3f& _w) const;   

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;


        ImageDataRGBPtr		m_gonioMap;
        Vector3f		m_pos;
        Spectrum	m_intensity;       

	};



    // AreaLight Interface
    class AreaLight : public Light
    {
    public:
        AreaLight(const Transform& l2w, const MediumInterface& _mi,  int ns);
        virtual Spectrum L(const Interaction& intr, const Vector3f& w) const = 0;
       
    };

	class InfiniteAreaLight : public Light
	{
	public:
        // InfiniteAreaLight Public Methods
        InfiniteAreaLight(const Transform& light2world, const Spectrum& _Power, int _ns, const std::string& _texmap);

        ~InfiniteAreaLight();

        virtual Spectrum            Le(const RayDifferential& r) const override;
       
        virtual void                        Preprocess(const Scene& scene);

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;
       
        
        ImageDataRGBPtr                         m_radianceMap;
        std::unique_ptr<Distribution2D>   m_distribution;        
        Vector3f                          m_worldCenter;
        float                                   m_worldRadius;
      
	};


    class  DistantLight : public Light
    {
    public:
        DistantLight(const Transform& _light2world, const Spectrum& _radiance, const Vector3f& dir);

        virtual void                        Preprocess(const Scene& scene);

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;

    private:
        // DistantLight Private Data
        Vector3f           m_lightDir;
        Spectrum         m_L;
        Vector3f           m_worldCenter;
        float                    m_worldRadius;
    };



    class DiffuseAreaLight : public AreaLight
    {
    public:
        // DiffuseAreaLight Public Methods
        DiffuseAreaLight(const Transform& light2world, const MediumInterface& _mi,
            const Spectrum& Le, int ns, const std::shared_ptr<Shape>& _pShape, bool _twoSided );

        ~DiffuseAreaLight();

        Spectrum L(const  Interaction& intr, const Vector3f& w) const override;


        virtual void                        Preprocess(const Scene& scene);

        virtual float                       Pdf_Li(const Interaction& ref, const Vector3f& wi) const override;
        virtual Spectrum            Sample_Le(const Vector2f& u1, const  Vector2f& u2,
                                                      float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const override;
        virtual void                        Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const override;
        virtual Spectrum            Sample_Li(const Interaction& ref, const Vector2f& u,
                                                      Vector3f* wi, float* pdf, VisibilityTester* vis) const override;
        virtual Spectrum            Power() const override;
     

    protected:
        Spectrum    m_Lemission;
        ShapePtr  m_shape;   
        bool                m_twoSided;
        float               m_area;
    };		
		
   
 

    class VisibilityTester
    {
    public:
        VisibilityTester() {}
        // VisibilityTester Public Methods
        VisibilityTester(const Interaction& p0, const Interaction& p1)
            : p0(p0), p1(p1) {}
        const Interaction& P0() const { return p0; }
        const Interaction& P1() const { return p1; }
        bool  Unoccluded(const Scene& scene) const;
        Spectrum Tr(const Scene& scene, Sampler& sampler) const;

    private:
        Interaction p0, p1;
    };



    LightPtr CreatePointLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium = nullptr );
    LightPtr CreateSpotLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium = nullptr);
    LightPtr CreateGoniometricLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium = nullptr);
    LightPtr CreateProjectionLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium = nullptr);
    LightPtr CreateDistantLight(const Transform& _lightToWorld, const ParamSet& _param);
    LightPtr CreateInfiniteLight(const Transform& _lightToWorld, const ParamSet& _param);
    LightPtr CreateDiffuseAreaLight(const Transform& _lightToWorld, const ParamSet& _param, const ShapePtr& _shape, const Medium* _pMedium = nullptr);
    


}