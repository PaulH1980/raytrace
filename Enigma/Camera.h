#pragma once
#include <memory>
#include "Defines.h"
#include "Transform.h"
#include "Ray.h"



namespace RayTrace
{

    class CameraSample {
    public:

        virtual ~CameraSample() {}

        Vector2f m_image;
        Vector2f m_lens;
        float		   m_time = 0;
    };



	class Camera
	{
	public:

		Camera(const AnimatedTransform& _trans, float _sOpen, float _sClose, std::unique_ptr<Film> _film, Medium* _medium = nullptr);
		virtual ~Camera();

		virtual float GenerateRay(const CameraSample& sample, Ray* ray) const = 0;
		virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const;

        virtual Spectrum We(const Ray& ray, Point2f* pRaster2 = nullptr) const;
        virtual void Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const;
        virtual Spectrum Sample_Wi(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf, Point2f* pRaster,  VisibilityTester* vis) const;

		const Film& GetFilm() const;

		AnimatedTransform    m_cameraToWorld;
		std::unique_ptr<Film> 	   m_film;
		Medium* m_medium;
		float m_shutterOpen, m_shutterClose;
	};

	class ProjectiveCamera : public Camera {
	public:
		// ProjectiveCamera Public Methods
		ProjectiveCamera(const AnimatedTransform& _cam2world,
			const Transform& _proj, const float _screenWindow[4],
			float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film);
	
		// ProjectiveCamera Protected Data
		Transform m_cameraToScreen, m_rasterToCamera;
		Transform m_screenToRaster, m_rasterToScreen;
		float m_lensRadius, m_focalDistance;
	};


	class PerspectiveCamera : public  ProjectiveCamera
	{
	public:
		PerspectiveCamera(const AnimatedTransform& _cam2world,
			const Transform& _proj, const float _screenWindow[4],
			float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film);



		virtual float		GenerateRay(const CameraSample& sample, Ray* ray) const;
		virtual float		GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const;
        virtual Spectrum	We(const Ray& ray, Point2f* pRaster2 = nullptr) const;
        virtual void		Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const;
        virtual Spectrum	Sample_Wi(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf, Point2f* pRaster, VisibilityTester* vis) const;


		Vector3f m_dxCamera, m_dyCamera;
		float    m_area;
	};

	class OrthoCamera : public ProjectiveCamera
	{
	public:
		// OrthoCamera Public Methods
		OrthoCamera(const AnimatedTransform& _cam2world, const Transform& _proj, const float _screenWindow[4],
			float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film);
		float GenerateRay(const CameraSample& sample, Ray*) const override;
		float GenerateRayDifferential(const CameraSample& sample, RayDifferential*) const override;
      

	private:
		// OrthoCamera Private Data
		Vector3f m_dxCamera, m_dyCamera;
	};


	class EnvironmentCamera : public Camera
	{
	public:
		// EnvironmentCamera Public Methods
		EnvironmentCamera(const AnimatedTransform& _cam2world, float _sopen, float _sclose, std::unique_ptr<Film> _film);
		float GenerateRay(const CameraSample& sample, Ray*) const;
	};

	class RealisticCamera : public Camera {

	public:

		float GenerateRay(const CameraSample& _sample, Ray* _ray) const override {
			UNUSED(_sample)
			UNUSED(_ray)
			return 0.0f;
		}
	};

	Camera* CreatePerspectiveCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld,
		Film* _pFilm, const Medium* _pMedium = nullptr);
    
	Camera* CreateOrthographicCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld,
        Film* _pFilm, const Medium* _pMedium = nullptr);

    Camera* CreateRealisticCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld,
        Film* _pFilm, const Medium* _pMedium = nullptr);
    
	Camera* CreateEnvironmentCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld,
        Film* _pFilm, const Medium* _pMedium = nullptr);


}