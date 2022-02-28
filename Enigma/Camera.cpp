
#include "MonteCarlo.h"
#include "Sample.h"
#include "Error.h"
#include "Filter.h"
#include "ParameterSet.h"
#include "Film.h"
#include "Interaction.h"
#include "Lights.h"

#include "Camera.h"




namespace RayTrace
{


	Camera::Camera(const AnimatedTransform& _trans, float _sOpen, float _sClose, std::unique_ptr<Film>  _film, Medium* _medium)
		: m_cameraToWorld( _trans )
		, m_shutterOpen( _sOpen )
		, m_shutterClose( _sClose )
		, m_film( std::move(_film) )
		, m_medium(_medium)
	{

	}

    Camera::~Camera()
    {

    }

    float Camera::GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const
	{
		float wt = GenerateRay(sample, rd);
		// Find ray after shifting one pixel in the $x$ direction
		CameraSample sshift = sample;
		++(sshift.m_image.x);
		Ray rx;
		float wtx    = GenerateRay(sshift, &rx);
		rd->m_rxOrigin    = rx.m_origin;
		rd->m_rxDirection = rx.m_dir;

		// Find ray after shifting one pixel in the $y$ direction
		--(sshift.m_image.x);
		++(sshift.m_image.y);
		Ray ry;
		float wty = GenerateRay(sshift, &ry);
		rd->m_ryOrigin = ry.m_origin;
		rd->m_ryDirection = ry.m_dir;
		if (wtx == 0.f || wty == 0.f) 
			return 0.f;
		rd->m_hasDifferentials = true;
		return wt;
	}

    Spectrum Camera::We(const Ray& ray, Point2f* pRaster2 /*= nullptr*/) const
    {
		throw std::exception("Not Implemented");
    }

    void Camera::Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const
    {
		throw std::exception("Not Implemented");
    }

    Spectrum Camera::Sample_Wi(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf, Point2f* pRaster, VisibilityTester* vis) const
    {
		throw std::exception("Not Implemented");
    }

    const Film& Camera::GetFilm() const
    {
        return *m_film;
    }

#pragma region ProjectiveCamera
	ProjectiveCamera::ProjectiveCamera(
		const AnimatedTransform& _cam2world, const Transform& _proj, 
		const float _screenWindow[4], 
		float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film)
		: Camera(_cam2world, _sopen, _sclose, std::move(_film) ) 
		, m_lensRadius( _lensr )
		, m_focalDistance( _focald )
		, m_cameraToScreen( _proj )
	{

		auto scaleMat = Scale((_screenWindow[1] - _screenWindow[0]), (_screenWindow[3] - _screenWindow[2]), 1.f);

		// Compute projective camera screen transformations
		m_screenToRaster = Scale(float(m_film->m_fullResolution.x), float(m_film->m_fullResolution.y), 1.f) *
						   Scale(1.f / (_screenWindow[1] - _screenWindow[0]), 1.f / (_screenWindow[2] - _screenWindow[3]), 1.f) *
						   Translate(Vector3f(-_screenWindow[0], -_screenWindow[3], 0.f));
		m_rasterToScreen = m_screenToRaster.inverted();
		m_rasterToCamera = m_cameraToScreen.inverted() * m_rasterToScreen;
	}
#pragma endregion

#pragma region PerspectiveCamera
	PerspectiveCamera::PerspectiveCamera(const AnimatedTransform& _cam2world, 
		const Transform& _proj, const float _screenWindow[4], 
		float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film)
		: ProjectiveCamera(_cam2world, _proj, _screenWindow, _sopen, _sclose, _lensr, _focald, std::move(_film) )
	{
		m_dxCamera = m_rasterToCamera.transformPoint(Vector3f(1, 0, 0)) - m_rasterToCamera.transformPoint(Vector3f(0, 0, 0));
		m_dyCamera = m_rasterToCamera.transformPoint(Vector3f(0, 1, 0)) - m_rasterToCamera.transformPoint(Vector3f(0, 0, 0));


        // Compute image plane bounds at $z=1$ for _PerspectiveCamera_
        Point2i res =  m_film->m_fullResolution;
        Point3f pMin = m_rasterToCamera.transformPoint(Point3f(0, 0, 0));
        Point3f pMax = m_rasterToCamera.transformPoint(Point3f(res.x, res.y, 0));
        pMin /= pMin.z;
        pMax /= pMax.z;
        m_area = std::abs((pMax.x - pMin.x) * (pMax.y - pMin.y));
	}

	float PerspectiveCamera::GenerateRay(const CameraSample& sample, Ray* ray) const
	{
		// Generate raster and camera samples
		Vector3f Pras(sample.m_image, 0);
		Vector3f Pcamera = m_rasterToCamera.transformPoint( Pras );		
		*ray = Ray( Vector3f(0,0,0), Normalize(Pcamera), InfinityF32 );
		// Modify ray for depth of field
		if (m_lensRadius > 0.) {
			// Sample point on lens
            Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;  

			// Compute point on plane of focus
			float ft = m_focalDistance / ray->m_dir.z;
			Vector3f Pfocus = ray->scale(ft);

			// Update ray for effect of lens
			ray->m_origin = Vector3f(lens, 0.f);
			ray->m_dir    = Normalize(Pfocus - ray->m_origin);
		}
		ray->m_time = sample.m_time;
		*ray = m_cameraToWorld.interpolateRay(*ray);
		return 1.f;
	}

	float PerspectiveCamera::GenerateRayDifferential(const CameraSample& sample, RayDifferential* ray) const
	{
			// Generate raster and camera samples
		Vector3f Pras(sample.m_image, 0);
		Vector3f Pcamera = m_rasterToCamera.transformPoint(Pras);
		*ray = RayDifferential(Vector3f(0, 0, 0), Normalize(Pcamera), InfinityF32);
		// Modify ray for depth of field
		if (m_lensRadius > 0.) {
			// Sample point on lens			
			Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;

			// Compute point on plane of focus
			float ft = m_focalDistance / ray->m_dir.z;
			Vector3f Pfocus = ray->scale(ft);

			// Update ray for effect of lens
			ray->m_origin = Vector3f(lens, 0.f);
			ray->m_dir = Normalize(Pfocus - ray->m_origin);
		}
		// Compute offset rays for _PerspectiveCamera_ ray differentials
		if (m_lensRadius > 0.) {
			// Compute _PerspectiveCamera_ ray differentials with defocus blur

			// Sample point on lens
			Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;

			Vector3f dx = Normalize(Pcamera + m_dxCamera);
			float ft = m_focalDistance / dx.z;
			Vector3f pFocus = Vector3f(0, 0, 0) + (ft * dx);
			ray->m_rxOrigin = Vector3f(lens, 0.f);
			ray->m_rxDirection = Normalize(pFocus - ray->m_rxOrigin);

			Vector3f dy = Normalize(Pcamera + m_dyCamera);
			ft = m_focalDistance / dy.z;
			pFocus = Vector3f(0, 0, 0) + (ft * dy);
			ray->m_ryOrigin = Vector3f(lens, 0.f);
			ray->m_ryDirection = Normalize(pFocus - ray->m_ryOrigin);
		}
		else {
			ray->m_rxOrigin = ray->m_ryOrigin = ray->m_origin;
			ray->m_rxDirection =Normalize(Pcamera + m_dxCamera);
			ray->m_ryDirection =Normalize(Pcamera + m_dyCamera);
		}		

		ray->m_time = sample.m_time;
		*ray = m_cameraToWorld.interpolateRayDifferential(*ray);		
		ray->m_hasDifferentials = true;
		return 1.f;
	}

    Spectrum PerspectiveCamera::We(const Ray& ray, Point2f* pRaster2 /*= nullptr*/) const
    {
        // Interpolate camera matrix and check if $\w{}$ is forward-facing
        Transform c2w;
		m_cameraToWorld.interpolate(ray.m_time, &c2w);
        float cosTheta = Dot(ray.m_dir, c2w.transformVector(Vector3f(0, 0, 1)));
        if (cosTheta <= 0) return 0;

        // Map ray $(\p{}, \w{})$ onto the raster grid
        Point3f pFocus = ray.scale((m_lensRadius > 0 ? m_focalDistance : 1) / cosTheta);

		//Point3f pRaster = Inverse(RasterToCamera)(Inverse(c2w)(pFocus));

		Point3f pRaster = Inverse(m_rasterToCamera).transformPoint(Inverse(c2w).transformPoint(pFocus));

        // Return raster position if requested
        if (pRaster2) *pRaster2 = Point2f(pRaster.x, pRaster.y);

        // Return zero importance for out of bounds points
        BBox2i sampleBounds = m_film->GetSampleBounds();
        if (pRaster.x < sampleBounds.m_min.x || pRaster.x >= sampleBounds.m_max.x ||
            pRaster.y < sampleBounds.m_min.y || pRaster.y >= sampleBounds.m_max.y)
            return 0;

        // Compute lens area of perspective camera
        float lensArea = m_lensRadius != 0 ? (PI * m_lensRadius * m_lensRadius) : 1;

        // Return importance for point on image plane
        float cos2Theta = cosTheta * cosTheta;
        return Spectrum(1 / (m_area * lensArea * cos2Theta * cos2Theta));
    }

    void PerspectiveCamera::Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const
    {
        // Interpolate camera matrix and fail if $\w{}$ is not forward-facing
        Transform c2w;
        m_cameraToWorld.interpolate(ray.m_time, &c2w);
        float cosTheta = Dot(ray.m_dir, c2w.transformVector(Vector3f(0, 0, 1)));
        if (cosTheta <= 0) {
            *pdfPos = *pdfDir = 0;
            return;
        }

        // Map ray $(\p{}, \w{})$ onto the raster grid
        Point3f pFocus = ray.scale((m_lensRadius > 0 ? m_focalDistance : 1) / cosTheta);
        Point3f pRaster = Inverse(m_rasterToCamera).transformPoint(Inverse(c2w).transformPoint(pFocus));

        // Return zero importance for out of bounds points
        BBox2i sampleBounds = m_film->GetSampleBounds();
		if (pRaster.x < sampleBounds.m_min.x || pRaster.x >= sampleBounds.m_max.x ||
			pRaster.y < sampleBounds.m_min.y || pRaster.y >= sampleBounds.m_max.y)
			return;
			

            // Compute lens area of perspective camera
       float lensArea = m_lensRadius != 0 ? (PI * m_lensRadius * m_lensRadius) : 1;
       *pdfPos = 1 / lensArea;
       *pdfDir = 1 / (m_area * cosTheta * cosTheta * cosTheta);
        // Return importance for point on image plane
        
		
    }

    Spectrum PerspectiveCamera::Sample_Wi(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf, Point2f* pRaster, VisibilityTester* vis) const
    {
        // Uniformly sample a lens interaction _lensIntr_
        Point2f pLens = m_lensRadius * ConcentricSampleDisk(u);
        Point3f pLensWorld = m_cameraToWorld.interpolatePoint( Point3f(pLens.x, pLens.y, 0), ref.m_time );
        Interaction lensIntr(pLensWorld, ref.m_time, m_medium);
        lensIntr.m_n = Normal3f(m_cameraToWorld.interpolateVector( Vector3f( 0, 0, 1 ), ref.m_time) );

        // Populate arguments and compute the importance value
        *vis = VisibilityTester(ref, lensIntr);
        *wi = lensIntr.m_p - ref.m_p;
        float dist = wi->length();
        *wi /= dist;

        // Compute PDF for importance arriving at _ref_

        // Compute lens area of perspective camera
        float lensArea = m_lensRadius != 0 ? (PI * m_lensRadius * m_lensRadius) : 1;
        *pdf = (dist * dist) / (AbsDot(lensIntr.m_n, *wi) * lensArea);
        return We(lensIntr.SpawnRay(-*wi), pRaster);
    }

#pragma endregion
#pragma region OrthoCamera
	OrthoCamera::OrthoCamera(
		const AnimatedTransform& _cam2world, const Transform& _proj, const float _screenWindow[4],
		float _sopen, float _sclose, float _lensr, float _focald, std::unique_ptr<Film> _film)
		: ProjectiveCamera( _cam2world, _proj, _screenWindow, _sopen, _sclose, _lensr, _focald, std::move(_film)  )
	{
		m_dxCamera = m_rasterToCamera.transformPoint(Vector3f(1, 0, 0));
		m_dyCamera = m_rasterToCamera.transformPoint(Vector3f(0, 1, 0));
	}

	float OrthoCamera::GenerateRay(const CameraSample& sample, Ray* ray) const
	{
		// Generate raster and camera samples
		Vector3f Pras(sample.m_image, 0);
		Vector3f Pcamera = m_rasterToCamera.transformPoint(Pras);;
		
		*ray = Ray(Pcamera, Vector3f(0, 0, 1), 0.f, INFINITY);
		// Modify ray for depth of field
		if (m_lensRadius > 0.) {
            // Sample point on lens
			Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;

			// Compute point on plane of focus
			float ft = m_focalDistance / ray->m_dir.z;
			Vector3f Pfocus = ray->scale(ft);

			// Update ray for effect of lens
			ray->m_origin = Vector3f(lens, 0.f);
			ray->m_dir    = Normalize(Pfocus - ray->m_origin);
		}
		ray->m_time = sample.m_time;
		*ray = m_cameraToWorld.interpolateRay(*ray);
		return 1.f;
	}

	float OrthoCamera::GenerateRayDifferential(const CameraSample& sample, RayDifferential* ray) const
	{
		// Generate raster and camera samples
		Vector3f Pras(sample.m_image, 0);
		Vector3f Pcamera = m_rasterToCamera.transformPoint( Pras );		
		*ray = RayDifferential(Pcamera, Vector3f(0, 0, 1), 0., INFINITY);

		// Modify ray for depth of field
		if (m_lensRadius > 0.) {
            // Sample point on lens
			Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;

			// Compute point on plane of focus
			float ft = m_focalDistance / ray->m_dir.z;
			Vector3f Pfocus = ray->scale( ft );

			// Update ray for effect of lens
			ray->m_origin = Vector3f(lens, 0.f);
			ray->m_dir    = Normalize(Pfocus - ray->m_origin);
		}
		ray->m_time = sample.m_time;
		// Compute ray differentials for _OrthoCamera_
		if (m_lensRadius > 0) {
			// Compute _OrthoCamera_ ray differentials with defocus blur

              // Sample point on lens
			Vector2f lens = ConcentricSampleDisk(sample.m_lens) * m_lensRadius;

			float ft = m_focalDistance / ray->m_dir.z;

			Vector3f pFocus = Pcamera + m_dxCamera + (ft * Vector3f(0, 0, 1));
			ray->m_rxOrigin = Vector3f(lens, 0.f);
			ray->m_rxDirection = Normalize(pFocus - ray->m_rxOrigin);

			pFocus = Pcamera + m_dyCamera + (ft * Vector3f(0, 0, 1));
			ray->m_ryOrigin = Vector3f(lens, 0.f);
			ray->m_ryDirection = Normalize(pFocus - ray->m_ryOrigin);
		}
		else {
			ray->m_rxOrigin = ray->m_origin + m_dxCamera;
			ray->m_ryOrigin = ray->m_origin + m_dyCamera;
			ray->m_rxDirection = ray->m_ryDirection = ray->m_dir;
		}
		ray->m_hasDifferentials = true;
		*ray = m_cameraToWorld.interpolateRayDifferential(*ray);
		return 1.f;
	}
#pragma endregion
#pragma region EnvironmentCamera
	EnvironmentCamera::EnvironmentCamera(const AnimatedTransform& cam2world, float sopen, float sclose, std::unique_ptr<Film> _film)
		: Camera(cam2world, sopen, sclose, std::move(_film))
	{

	}

	float EnvironmentCamera::GenerateRay(const CameraSample& sample, Ray* _ray) const
	{
		auto  ComputeDirection = [](const CameraSample& _sample, const Film* _film)
		{
			float theta = PI * _sample.m_image.y     / _film->m_fullResolution.y;
			float phi   = TWO_PI * _sample.m_image.x / _film->m_fullResolution.x;
			return SphericalDirection(theta, phi);
		};

		float time = Lerp(m_shutterOpen, m_shutterClose, sample.m_time);
		const Ray ray = Ray(Vector3f(0, 0, 0), ComputeDirection( sample, m_film.get() ),  std::numeric_limits<float>::infinity(), time);
		*_ray = m_cameraToWorld.interpolateRay(ray);
		return 1.0f;
	}	

    Camera* CreatePerspectiveCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld, Film* _pFilm, const Medium* _pMedium /*= nullptr*/)
    {
        float shutteropen = _paramSet.FindOneFloat("shutteropen", 0.f);
        float shutterclose = _paramSet.FindOneFloat("shutterclose", 1.f);
        if (shutterclose < shutteropen) {
            Warning("Shutter close time [%f] < shutter open [%f].  Swapping them.",
                shutterclose, shutteropen);
            std::swap(shutterclose, shutteropen);
        }
        float lensradius = _paramSet.FindOneFloat("lensradius", 0.f);
        float focaldistance = _paramSet.FindOneFloat("focaldistance", 1e30f);
        float frame = _paramSet.FindOneFloat("frameaspectratio", float(_pFilm->m_fullResolution.x) / float(_pFilm->m_fullResolution.y ) );
        float screen[4];
        if (frame > 1.f) {
            screen[0] = -frame;
            screen[1] = frame;
            screen[2] = -1.f;
            screen[3] = 1.f;
        }
        else {
            screen[0] = -1.f;
            screen[1] = 1.f;
            screen[2] = -1.f / frame;
            screen[3] = 1.f / frame;
        }
        int swi;
        const float* sw = _paramSet.FindFloat("screenwindow", &swi);
        if (sw && swi == 4)
            memcpy(screen, sw, 4 * sizeof(float));
        float fov = _paramSet.FindOneFloat("fov", 90.);
        float halffov = _paramSet.FindOneFloat("halffov", -1.f);
        if (halffov > 0.f)
            // hack for structure synth, which exports half of the full fov
            fov = 2.f * halffov;       
	
		
		auto  Perspective = [](float fov, float n, float f)
		{
			// Perform projective divide for perspective projection
			Matrix4x4 persp(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, f / (f - n), -f * n / (f - n),
				0, 0, 1, 0);

			// Scale canonical perspective view to specified field of view
			float invTanAng = 1 / std::tan(ToRadians(fov) / 2);
			return Scale(invTanAng, invTanAng, 1) * Transform(persp);
		};

		return new PerspectiveCamera(_camToWorld, Perspective(fov, 1e-2f, 1000.f), screen, shutteropen,
            shutterclose, lensradius, focaldistance, std::unique_ptr<Film>( _pFilm ) );
    }

    Camera* CreateOrthographicCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld, Film* _pFilm, const Medium* _pMedium /*= nullptr*/)
    {
        // Extract common camera parameters from _ParamSet_
        float shutteropen = _paramSet.FindOneFloat("shutteropen", 0.f);
        float shutterclose = _paramSet.FindOneFloat("shutterclose", 1.f);
        if (shutterclose < shutteropen) {
            Warning("Shutter close time [%f] < shutter open [%f].  Swapping them.",
                shutterclose, shutteropen);
            std::swap(shutterclose, shutteropen);
        }
        float lensradius = _paramSet.FindOneFloat("lensradius", 0.f);
        float focaldistance = _paramSet.FindOneFloat("focaldistance", 1e30f);
        float frame = _paramSet.FindOneFloat("frameaspectratio",
            float(_pFilm->m_fullResolution.x) / float(_pFilm->m_fullResolution.y));
        float screen[4];
        if (frame > 1.f) {
            screen[0] = -frame;
            screen[1] = frame;
            screen[2] = -1.f;
            screen[3] = 1.f;
        }
        else {
            screen[0] = -1.f;
            screen[1] = 1.f;
            screen[2] = -1.f / frame;
            screen[3] = 1.f / frame;
        }
        int swi;
        const float* sw = _paramSet.FindFloat("screenwindow", &swi);
        if (sw && swi == 4)
            memcpy(screen, sw, 4 * sizeof(float));
        return new OrthoCamera(_camToWorld, Orthographic(0.f, 1.f), screen, shutteropen, shutterclose,
            lensradius, focaldistance, std::unique_ptr<Film>(_pFilm));
    }

    Camera* CreateRealisticCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld, Film* _pFilm, const Medium* _pMedium /*= nullptr*/)
    {
		return nullptr;
    }

    Camera* CreateEnvironmentCamera(const ParamSet& _paramSet, const AnimatedTransform& _camToWorld, Film* _pFilm, const Medium* _pMedium /*= nullptr*/)
    {
        float shutteropen = _paramSet.FindOneFloat("shutteropen", 0.f);
        float shutterclose = _paramSet.FindOneFloat("shutterclose", 1.f);
        if (shutterclose < shutteropen) {
            Warning("Shutter close time [%f] < shutter open [%f].  Swapping them.",
                shutterclose, shutteropen);
            std::swap(shutterclose, shutteropen);
        }
        float lensradius = _paramSet.FindOneFloat("lensradius", 0.f);
        float focaldistance = _paramSet.FindOneFloat("focaldistance", 1e30f);
        float frame = _paramSet.FindOneFloat("frameaspectratio",
            float(_pFilm->m_fullResolution.x) / float(_pFilm->m_fullResolution.y));
        float screen[4];
        if (frame > 1.f) {
            screen[0] = -frame;
            screen[1] = frame;
            screen[2] = -1.f;
            screen[3] = 1.f;
        }
        else {
            screen[0] = -1.f;
            screen[1] = 1.f;
            screen[2] = -1.f / frame;
            screen[3] = 1.f / frame;
        }
        int swi;
        const float* sw = _paramSet.FindFloat("screenwindow", &swi);
        if (sw && swi == 4)
            memcpy(screen, sw, 4 * sizeof(float));
        (void)lensradius; // don't need this
        (void)focaldistance; // don't need this
        return new EnvironmentCamera(_camToWorld, shutteropen, shutterclose, std::unique_ptr<Film>(_pFilm));
    }

#pragma  endregion

}

