#include "RNG.h"
#include "Ray.h"
#include "Memory.h"
#include "Scene.h"
#include "Sample.h"
#include "Renderer.h"
#include "Sphere.h"
#include "IO.h"
#include "MonteCarlo.h"
#include "Shape.h"
#include "Spectrum.h"
#include "Medium.h"
#include "BxDF.h"
#include "Primitive.h"
#include "ParameterSet.h"
#include "Concurrency.h"
#include "MipMap.h"
#include "Api.h"
#include "Misc.h"
#include "Integrator.h"
#include "Scene.h"
#include "Lights.h"





namespace RayTrace
{

  /*  LightSampleOffsets::LightSampleOffsets(int _count, Sample* _sample)
        : m_nSamples( _count )
        , m_componentOffset(_sample->Add1D(_count))
        , m_posOffset(_sample->Add2D(_count))
    {   
    }


    LightSample::LightSample(float up0, float up1, float ucomp)
    {
        assert(up0 >= 0.f && up0 < 1.f);
        assert(up1 >= 0.f && up1 < 1.f);
        assert(ucomp >= 0.f && ucomp < 1.f);
        m_uPos[0] = up0;
        m_uPos[1] = up1;
        m_uComponent = ucomp;
    }
 
    LightSample::LightSample(const Sample* _sample, const LightSampleOffsets& _offsets, uint32_t _num)
    {
        assert(_num < _sample->n2D[_offsets.m_posOffset]);
        assert(_num < _sample->n1D[_offsets.m_componentOffset]);
        m_uPos[0] = _sample->twoD[_offsets.m_posOffset][2 * _num];
        m_uPos[1] = _sample->twoD[_offsets.m_posOffset][2 * _num + 1];
        m_uComponent = _sample->oneD[_offsets.m_componentOffset][_num];
        assert(m_uPos[0] >= 0.f && m_uPos[0] < 1.f);
        assert(m_uPos[1] >= 0.f && m_uPos[1] < 1.f);
        assert(m_uComponent >= 0.f && m_uComponent < 1.f);
    }

    LightSample::LightSample(RNG& _rng) 
    {
        m_uPos[0] = _rng.randomFloat();
        m_uPos[1] = _rng.randomFloat();
        m_uComponent = _rng.randomFloat();
    }*/

#pragma region Light
    Light::Light(int flags, const Transform& LightToWorld, const MediumInterface& mediumInterface, int nSamples /*= 1*/)
        : m_lightToWorld( LightToWorld )
        , m_worldToLight( LightToWorld.inverted() )
        , m_nSamples( nSamples )
        , m_flags(flags)
        , m_mediumInterface( mediumInterface )

    {

    }

    Spectrum Light::Le(const RayDifferential& r) const
    {
        return Spectrum(0.f);
    }

#pragma endregion

#pragma region PointLight   

    PointLight::PointLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity)
        : Light( (int)eLightFlags::LIGHTFLAG_DELTAPOSITION, _light2world, _mi )
        , m_pos( _light2world.transformPoint(Vec3fZero))
        , m_intensity(_intensity)
    {

    }

    float PointLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return 0;
    }

    Spectrum PointLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        *ray = Ray(m_pos, UniformSampleSphere(u1), INFINITY, time, m_mediumInterface.inside);
        *nLight = ray->m_dir;
        *pdfPos = 1;
        *pdfDir = UniformSpherePdf();
        return m_intensity;
    }

    void PointLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        *pdfPos = 0;
        *pdfDir = UniformSpherePdf();
    }

    Spectrum PointLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        *wi = Normalize(m_pos - ref.m_p);
        *pdf = 1.f;
        *vis = VisibilityTester(ref, Interaction(m_pos, ref.m_time, m_mediumInterface));

        return m_intensity / DistanceSqr(m_pos, ref.m_p);
    }

    Spectrum PointLight::Power() const
    {
        return 4.0f * PI * m_intensity;
    }

#pragma endregion
#pragma region SpotLight
    SpotLight::SpotLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity, float _width, float _fall)
        : Light((int)eLightFlags::LIGHTFLAG_DELTAPOSITION, _light2world, _mi )
        , m_intensity(_intensity)
        , m_pos(_light2world.transformPoint(Vector3f(0.0f)))
    {
        m_cosTotalWidth   = cosf(ToRadians(_width));
        m_cosFalloffStart = cosf(ToRadians(_fall));
    }

    float SpotLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return 0.f;
    }

    Spectrum SpotLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        Vector3f w = UniformSampleCone(u1, m_cosTotalWidth);
        *ray = Ray(m_pos, m_lightToWorld.transformVector(w), InfinityF32, time, m_mediumInterface.inside);
        *nLight = ray->m_dir;
        *pdfPos = 1;
        *pdfDir = UniformConePdf(m_cosTotalWidth);
        return m_intensity * Falloff(ray->m_dir);
    }

    void SpotLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        *pdfPos = 0;
        *pdfDir = (CosTheta(m_worldToLight.transformVector(ray.m_dir)) >= m_cosTotalWidth)
            ? UniformConePdf(m_cosTotalWidth)
            : 0;
    }

    Spectrum SpotLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        *wi = Normalize(m_pos - ref.m_p);
        *pdf = 1.f;
        *vis =
            VisibilityTester(ref, Interaction(m_pos, ref.m_time, m_mediumInterface));
        return m_intensity * Falloff(-*wi) / DistanceSqr(m_pos, ref.m_p);
    }

    Spectrum SpotLight::Power() const
    {
        return m_intensity * 2.f * PI *
            (1.f - .5f * (m_cosFalloffStart + m_cosTotalWidth));
    }

    float SpotLight::Falloff(const Vector3f& w) const
    {
        Vector3f wl = Normalize(m_worldToLight.transformVector(w));
        float cosTheta = wl.z;
        if (cosTheta < m_cosTotalWidth) return 0;
        if (cosTheta >= m_cosFalloffStart) return 1;
        // Compute falloff inside spotlight cone
        float delta =
            (cosTheta - m_cosTotalWidth) / (m_cosFalloffStart - m_cosTotalWidth);
        return (delta * delta) * (delta * delta);
    }
#pragma endregion

#pragma region ProjectionLight
    ProjectionLight::ProjectionLight(const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity, const std::string& _texname, float _fov)
        : Light((int)eLightFlags::LIGHTFLAG_DELTAPOSITION, _light2world, _mi)
        , m_intensity(_intensity)
        , m_pos(_light2world.transformPoint(Vector3f(0.0f)))
        , m_hither(1e-3f)
        , m_yon(1e30f)
    {
        auto  Perspective = [](float fov, float n, float f)
        {
            // Perform projective divide for perspective projection
            Matrix4x4 persp(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, f / (f - n), -f * n / (f - n),
                0, 0, 1, 0);

            // Scale canonical perspective view to specified field of view
            float invTanAng = 1 / std::tan(ToRadians(fov) / 2);
            return Scale(invTanAng, invTanAng, 1) * Transform(persp);
        };


        m_lightProj = Perspective(_fov,m_hither, m_yon );

        Vector2i resolution;
        int numChannels = 0;
        eTextureType type;
        auto colors = ReadImage(_texname.c_str(), resolution, numChannels, type);
        auto texels = PixelDataToSpectrum(colors, numChannels);
        colors.clear();
        if (!colors.empty())
            m_projMap = std::make_shared<MipMap<RGBSpectrum>>(resolution.x, resolution.y, texels);
     
        // Initialize _ProjectionLight_ projection matrix
        float aspect = m_projMap ? float(resolution.x) / float(resolution.y) : 1.f;
        // Initialize _ProjectionLight_ projection matrix        
        if (aspect > 1)
            m_screenBounds = BBox2f(Vector2f(-aspect, -1), Vector2f(aspect, 1));
        else
            m_screenBounds = BBox2f(Vector2f(-1, -1 / aspect), Vector2f(1, 1 / aspect));
        // Compute cosine of cone surrounding projection directions
        float opposite = tanf(ToRadians(_fov) / 2.f);
        float tanDiag = opposite * sqrtf(1.f + 1.f / (aspect * aspect));
        m_cosTotalWidth = cosf(atanf(tanDiag));
    }


    Spectrum ProjectionLight::projection(const Vector3f& _w) const
    {
        if (!m_projMap) 
            return 1.0f;
        
        Vector3f wl = m_worldToLight.transformVector(_w);
        // Discard directions behind projection light
        if (wl.z < m_hither) return 0.f;

        // Project point onto projection plane and compute light
        auto Pl = m_lightProj.transformPoint(wl);//lightProjection(Point(wl.x, wl.y, wl.z));
        if (!m_screenBounds.inside(Vector2f(Pl.x, Pl.y)))
            return 0.f;

        if (!m_projMap) 
            return 1;

        Vector2f st = m_screenBounds.offset(Vector2f(Pl.x, Pl.y));
        return Spectrum(m_projMap->lookup(st), SPECTRUM_ILLUMINANT);
    }

    float ProjectionLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return 0.f;
    }

    Spectrum ProjectionLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        Vector3f w = UniformSampleCone(u1, m_cosTotalWidth);
        *ray = Ray(m_pos, m_lightToWorld.transformVector(w), InfinityF32, time, m_mediumInterface.inside);
        *nLight = ray->m_dir;
        *pdfPos = 1;
        *pdfDir = UniformConePdf(m_cosTotalWidth);
        return m_intensity * projection(ray->m_dir );
    }

    void ProjectionLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        *pdfPos = 0.f;
        *pdfDir = (CosTheta(m_worldToLight.transformVector(ray.m_dir)) >= m_cosTotalWidth)
            ? UniformConePdf(m_cosTotalWidth)
            : 0;
    }

    Spectrum ProjectionLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        *wi = Normalize(m_pos - ref.m_p);
        *pdf = 1;
        *vis =
            VisibilityTester(ref, Interaction(m_pos, ref.m_time, m_mediumInterface));
        return m_intensity * projection(-*wi) / DistanceSqr(m_pos, ref.m_p);
    }

    Spectrum ProjectionLight::Power() const
    {
        return (m_projMap
            ? Spectrum(m_projMap->lookup(Vector2f(.5f, .5f), .5f), SPECTRUM_ILLUMINANT)
            : Spectrum(1.f)) * m_intensity * 2.0f * PI * (1.f - m_cosTotalWidth);
    }

#pragma endregion

#pragma region GonioPhotometricLight 
    GonioPhotometricLight::GonioPhotometricLight(
        const Transform& _light2world, const MediumInterface& _mi, const Spectrum& _intensity, const std::string& _texname)
        : Light((int)eLightFlags::LIGHTFLAG_DELTAPOSITION, _light2world, _mi)
        , m_intensity(_intensity)
        , m_pos(_light2world.transformPoint(Vector3f(0.0f)))
    {
        
       
        Vector2i resolution;
        int numChannels = 0;
        eTextureType type;
        auto colors = ReadImage(_texname.c_str(), resolution, numChannels, type );
        auto texels = PixelDataToSpectrum(colors, numChannels);
        colors.clear();
        if (!texels.empty())
            m_gonioMap = std::make_shared<MipMap<RGBSpectrum>>(resolution.x, resolution.y, texels);       
    }

    Spectrum GonioPhotometricLight::scale(const Vector3f& _w) const
    {
        Vector3f wp= m_worldToLight.transformVector(_w);
        std::swap(wp.y, wp.z);
        float theta = SphericalTheta(wp);
        float phi   = SphericalPhi(wp);
        float s = phi * INV_TWO_PI,
              t = theta * INV_PI;
        return (m_gonioMap == nullptr) ? 1.f :
            Spectrum(m_gonioMap->lookup(Vector2f( s, t ), SPECTRUM_ILLUMINANT));
    }

    float GonioPhotometricLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return 0.f;
    }

    Spectrum GonioPhotometricLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        *ray = Ray(m_pos, UniformSampleSphere(u1), InfinityF32, time, m_mediumInterface.inside);
        *nLight = ray->m_dir;
        *pdfPos = 1.f;
        *pdfDir = UniformSpherePdf();
        return m_intensity * scale(ray->m_dir);
    }

    void GonioPhotometricLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        *pdfPos = 0.f;
        *pdfDir = UniformSpherePdf();
    }

    Spectrum GonioPhotometricLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        *wi = Normalize(m_pos - ref.m_p);
        *pdf = 1.f;
        *vis = VisibilityTester(ref, Interaction(m_pos, ref.m_time, m_mediumInterface));
        return m_intensity * scale(-*wi) / DistanceSqr(m_pos, ref.m_p);
    }

    Spectrum GonioPhotometricLight::Power() const
    {
        return 4.f * PI * PI *
            Spectrum(m_gonioMap ? m_gonioMap->lookup(Vector2f{ .5f, .5f }, .5f) : 1.f, SPECTRUM_ILLUMINANT);
    }

#pragma endregion

#pragma region IniniteAreaLight

    struct Task
    {

        void operator()()
        {

            float filter = 0.5f / std::min(m_width, m_height);
            float vp = (m_yStart+0.5f) / (float)m_height;
            float sinTheta = sinf(PI * float(m_yStart + .5f) / float(m_height));

            for (int u = 0; u < m_width; ++u) {
                float up = float(u + .5f) / (float)m_width;
                std::size_t idx = u + m_yStart * m_width;
                (*m_img)[idx] = m_radiance->lookup(Vector2f(up, vp), filter).y();
                (*m_img)[idx] *= sinTheta;
            }
        }
        FloatVector* m_img;
        ImageDataRGBPtr     m_radiance;
        int                 m_yStart;
        int                 m_width,
            m_height;
    };



    InfiniteAreaLight::InfiniteAreaLight(const Transform& _light2world, 
        const Spectrum& _L, int _ns, const std::string& _texmap)
        : Light((int)eLightFlags::LIGHTFLAG_INFINITE, _light2world, MediumInterface(), _ns)
        , m_worldRadius(0.f)
    {
        int width = 0, height = 0;
     
        int numChannels = 0;
        eTextureType type;
        auto colors = ReadImage(_texmap.c_str(), &width, &height, numChannels, type);
        auto texels = PixelDataToSpectrum(colors, numChannels);
        colors.clear();

        // Read texel data from _texmap_ into _texels_     
        if( !texels.empty() ) {
            for (int i = 0; i < width * height; ++i)
               texels[i] *= _L.ToRGBSpectrum();            
        }
        else 
        {
            texels.resize(1);
            width = height = 1;
            texels[0] = _L.ToRGBSpectrum();
        }

       
        m_radianceMap = std::make_shared<MipMap<RGBSpectrum>>(width, height, texels);     
        width = m_radianceMap->width() * 2;
        height = m_radianceMap->height() * 2;      

        // Compute scalar-valued image _img_ from environment map
        //float filter = 1.f / std::max(width, height);
        std::vector<float> img( static_cast<std::size_t>(width) * static_cast<std::size_t>(height));       

        TaskQueue<Task> worker( NumSystemCores() );

        for (int v = 0; v < height; ++v) {

            Task t = { &img, m_radianceMap, v, width, height };
            worker.Enqueue(t);
        }

        worker.Join();

        // Compute sampling distributions for rows and columns of image
        m_distribution = std::make_unique<Distribution2D>(img.data(), width, height);
       
    }


    InfiniteAreaLight::~InfiniteAreaLight()
    {

    }

 

   Spectrum InfiniteAreaLight::Le(const RayDifferential& r) const
    {
        Vector3f w = Normalize( m_worldToLight.transformVector (r.m_dir) );
        Vector2f st(SphericalPhi(w) * INV_TWO_PI, SphericalTheta(w) * INV_PI);
        return Spectrum(m_radianceMap->lookup(st), eSpectrumType::SPECTRUM_ILLUMINANT);
    }

    void InfiniteAreaLight::Preprocess(const Scene& scene)
    {
        scene.worldBound().getBoundingSphere(m_worldCenter, m_worldRadius);
    }

    float InfiniteAreaLight::Pdf_Li(const Interaction& ref, const Vector3f& w) const
    {
        Vector3f wi = m_worldToLight.transformVector(w);
        float theta = SphericalTheta(wi), 
                phi = SphericalPhi(wi);
        float sinTheta = std::sin(theta);
        if (sinTheta == 0) return 0;
        return m_distribution->Pdf(Vector2f(phi * INV_TWO_PI, theta * INV_TWO_PI)) / (2 * PI * PI * sinTheta);
    }

    Spectrum InfiniteAreaLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
       
        Vector2f u = u1;
        // Find $(u,v)$ sample coordinates in infinite light texture
        float mapPdf;
        Vector2f uv = m_distribution->SampleContinuous(u, &mapPdf);
        if (mapPdf == 0) 
            return Spectrum(0.f);
        float theta = uv[1] * PI, 
              phi = uv[0] * 2.f * PI;
        float cosTheta = std::cos(theta), 
              sinTheta = std::sin(theta);
        float sinPhi = std::sin(phi), 
              cosPhi = std::cos(phi);
        Vector3f d =
            -m_lightToWorld.transformVector(Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));
        *nLight = d;

        // Compute origin for infinite light sample ray
        Vector3f v1, v2;
        CoordinateSystem(-d, &v1, &v2);
        Vector2f cd = ConcentricSampleDisk(u2);
        Vector3f pDisk = m_worldCenter + m_worldRadius * (cd.x * v1 + cd.y * v2);
        *ray = Ray(pDisk + m_worldRadius * -d, d, InfinityF32, time);

        // Compute _InfiniteAreaLight_ ray PDFs
        *pdfDir = sinTheta == 0 ? 0 : mapPdf / (2 * PI * PI * sinTheta);
        *pdfPos = 1 / (PI * m_worldRadius * m_worldRadius);
        return Spectrum(m_radianceMap->lookup(uv), SPECTRUM_ILLUMINANT);
    }

    void InfiniteAreaLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        Vector3f d = -m_worldToLight.transformVector(ray.m_dir);
        float theta = SphericalTheta(d), phi = SphericalPhi(d);
        Vector2f uv(phi * INV_TWO_PI, theta * INV_TWO_PI);
        float mapPdf = m_distribution->Pdf(uv);
        *pdfDir = mapPdf / (2 * PI * PI * std::sin(theta));
        *pdfPos = 1 / (PI *m_worldRadius * m_worldRadius);
    }

    Spectrum InfiniteAreaLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        // Find $(u,v)$ sample coordinates in infinite light texture
        float mapPdf;
        auto uv = m_distribution->SampleContinuous(u, &mapPdf);
        if (mapPdf == 0) return Spectrum(0.f);

        // Convert infinite light sample point to direction
        float theta = uv[1] * PI, 
                phi = uv[0] * 2 * PI;
        float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
        float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
        *wi =
            m_lightToWorld.transformVector(Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));

        // Compute PDF for sampled infinite light direction
        *pdf = mapPdf / (2 * PI * PI * sinTheta);
        if (sinTheta == 0) *pdf = 0;

        // Return radiance value for infinite light direction
        *vis = VisibilityTester(ref, Interaction(ref.m_p + *wi * (2 * m_worldRadius),
            ref.m_time, m_mediumInterface));
        return Spectrum(m_radianceMap->lookup(uv), SPECTRUM_ILLUMINANT);
    }

    Spectrum InfiniteAreaLight::Power() const
    {
        Vector2f half(0.5f, 0.5f);
        
        return PI * m_worldRadius * m_worldRadius * Spectrum(m_radianceMap->lookup(half, half, half), SPECTRUM_ILLUMINANT);
    }    
#pragma endregion

#pragma region AreaLight

    AreaLight::AreaLight(const Transform& l2w, const MediumInterface& _mi, int ns)
        : Light((int)eLightFlags::LIGHTFLAG_AREA, l2w, _mi, ns)
    {

    }
#pragma endregion

#pragma region DiffuseAreaLight  

    DiffuseAreaLight::DiffuseAreaLight(const Transform& _light2World, const MediumInterface& _mi, const Spectrum& Le, int ns, const std::shared_ptr<Shape>& _pShape, bool _twoSided)
        : AreaLight( _light2World, _mi, ns )
        , m_Lemission(Le)
        , m_shape(_pShape)
        , m_twoSided( _twoSided )

    {
        m_area = m_shape->area();
    }

    DiffuseAreaLight::~DiffuseAreaLight()
    {

    }

    Spectrum DiffuseAreaLight::L(const Interaction& intr, const Vector3f& w) const
    {
        return (m_twoSided || Dot(intr.m_n, w) > 0) ? m_Lemission : Spectrum(0.f);
    }

    void DiffuseAreaLight::Preprocess(const Scene& scene)
    {

    }

    float DiffuseAreaLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return m_shape->pdf(ref, wi);
    }

    Spectrum DiffuseAreaLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        // Sample a point on the area light's _Shape_, _pShape_
        Interaction pShape = m_shape->sample(u1, pdfPos);
        pShape.m_mediumInterface = m_mediumInterface;
        *nLight = pShape.m_n;

        // Sample a cosine-weighted outgoing direction _w_ for area light
        Vector3f w;
        if (m_twoSided) {
            Vector2f u = u2;
            // Choose a side to sample and then remap u[0] to [0,1] before
            // applying cosine-weighted hemisphere sampling for the chosen side.
            if (u[0] < .5) {
                u[0] = std::min(u[0] * 2, OneMinusEpsilon);
                w = CosineSampleHemisphere(u);
            }
            else {
                u[0] = std::min((u[0] - .5f) * 2, OneMinusEpsilon);
                w = CosineSampleHemisphere(u);
                w.z *= -1;
            }
            *pdfDir = 0.5f * CosineHemispherePdf(std::abs(w.z));
        }
        else {
            w = CosineSampleHemisphere(u2);
            *pdfDir = CosineHemispherePdf(w.z);
        }

        Vector3f v1, v2, n(pShape.m_n);
        CoordinateSystem(n, &v1, &v2);
        w = w.x * v1 + w.y * v2 + w.z * n;
        *ray = pShape.SpawnRay(w);
        return L(pShape, w);
    }

    void DiffuseAreaLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        Interaction it(ray.m_origin, nLight, Vector3f(), Vector3f(nLight), ray.m_time, m_mediumInterface);
        *pdfPos = m_shape->pdf(it);
        *pdfDir = m_twoSided ? (.5 * CosineHemispherePdf(AbsDot(nLight, ray.m_dir)))
            : CosineHemispherePdf(Dot(nLight, ray.m_dir));
    }

    Spectrum DiffuseAreaLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        Interaction pShape = m_shape->sample(ref, u, pdf);
        pShape.m_mediumInterface = m_mediumInterface;
        if (*pdf == 0 || LengthSqr (pShape.m_p - ref.m_p) == 0) {
            *pdf = 0;
            return 0.f;
        }
        *wi = Normalize(pShape.m_p - ref.m_p);
        *vis = VisibilityTester(ref, pShape);
        return L(pShape, -*wi);
    }       

    Spectrum DiffuseAreaLight::Power() const
    {
        float scale = m_twoSided ? 2.0f : 1.0f;
        return m_Lemission * (m_area * PI * scale);
    }
   
#pragma endregion

#pragma region DistantLight
    DistantLight::DistantLight(const Transform& _light2world, const Spectrum& _radiance, const Vector3f& dir)
        : Light(eLightFlags::LIGHTFLAG_DELTADIRECTION, _light2world, MediumInterface() )
        , m_L( _radiance )
        , m_lightDir( _light2world.transformVector( dir ))
        , m_worldRadius(0.f)
    {

    }

    void DistantLight::Preprocess(const Scene& scene)
    {
        scene.worldBound().getBoundingSphere(m_worldCenter, m_worldRadius);
    }

    float DistantLight::Pdf_Li(const Interaction& ref, const Vector3f& wi) const
    {
        return 0.f;
    }

    Spectrum DistantLight::Sample_Le(const Vector2f& u1, const Vector2f& u2, float time, Ray* ray, Vector3f* nLight, float* pdfPos, float* pdfDir) const
    {
        Vector3f v1, v2;
        CoordinateSystem(m_lightDir, &v1, &v2);
        Vector2f cd = ConcentricSampleDisk(u1);
        Vector3f pDisk = m_worldCenter + m_worldRadius * (cd.x * v1 + cd.y * v2);

        // Set ray origin and direction for infinite light ray
        *ray = Ray(pDisk + m_worldRadius * m_lightDir, -m_lightDir, InfinityF32, time);
        *nLight = ray->m_dir;
        *pdfPos = 1 / (PI * m_worldRadius * m_worldRadius);
        *pdfDir = 1;
        return m_L;
    }

    void DistantLight::Pdf_Le(const Ray& ray, const Vector3f& nLight, float* pdfPos, float* pdfDir) const
    {
        *pdfPos = 1 / (PI * m_worldRadius * m_worldRadius);
        *pdfDir = 0;
    }

    Spectrum DistantLight::Sample_Li(const Interaction& ref, const Vector2f& u, Vector3f* wi, float* pdf, VisibilityTester* vis) const
    {
        *wi = m_lightDir;
        *pdf = 1;
        Vector3f pOutside = ref.m_p + m_lightDir * (2 * m_worldRadius);
        *vis = VisibilityTester(ref, Interaction(pOutside, ref.m_time, m_mediumInterface));
        return m_L;
    }       

    Spectrum DistantLight::Power() const
    {
        return m_L * PI * m_worldRadius * m_worldRadius;
    }


#pragma endregion 

    bool VisibilityTester::Unoccluded(const Scene& scene) const
    {
        return !scene.intersectP(p0.SpawnRayTo(p1));
    }

    Spectrum VisibilityTester::Tr(const Scene& scene, Sampler& sampler) const
    {
        Ray ray(p0.SpawnRayTo(p1));
        Spectrum Tr(1.f);
        while (true) {
            SurfaceInteraction isect;
            bool hitSurface = scene.intersect(ray, &isect);
            // Handle opaque surface along ray's path
            if (hitSurface && isect.m_primitive->getMaterial() != nullptr)
                return Spectrum(0.0f);

            // Update transmittance for current ray segment
            if (ray.m_medium) 
                Tr *= ray.m_medium->Tr(ray, sampler);

            // Generate next ray segment or return final transmittance
            if (!hitSurface) 
                break;

            ray = isect.SpawnRayTo(p1);
        }
        return Tr;
    }

    LightPtr CreatePointLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium /*= nullptr */)
    {
        Spectrum I  = _param.FindOneSpectrum("I", Spectrum(1.0));
        Spectrum sc = _param.FindOneSpectrum("scale", Spectrum(1.0));
        Vector3f P  = _param.FindOnePoint3f("from", Vector3f(0, 0, 0));
        Transform l2w = Translate(Vector3f(P.x, P.y, P.z)) * _lightToWorld;
        return std::make_shared<PointLight>(l2w, _pMedium, I * sc);
    }
   

    LightPtr CreateSpotLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium /*= nullptr*/)
    {
        Spectrum I      = _param.FindOneSpectrum("I", Spectrum(1.0));
        Spectrum sc     = _param.FindOneSpectrum("scale", Spectrum(1.0));
        float coneangle = _param.FindOneFloat("coneangle", 30.);
        float conedelta = _param.FindOneFloat("conedeltaangle", 5.);
        // Compute spotlight world to light transformation
        Vector3f from   = _param.FindOnePoint3f("from", Vector3f(0, 0, 0));
        Vector3f to     = _param.FindOnePoint3f("to",   Vector3f(0, 0, 1));
        Vector3f dir = Normalize(to - from);
        Vector3f du, dv;
        CoordinateSystem(dir, &du, &dv);
        Transform dirToZ =
            Transform(Matrix4x4(du.x,  du.y, du.z, 0.f, 
                                dv.x,  dv.y, dv.z, 0.f, 
                                dir.x, dir.y, dir.z, 0.f, 
                                0.f, 0.f, 0.f, 1.f));
        Transform light2world = _lightToWorld * Translate(Vector3f(from.x, from.y, from.z)) * (dirToZ).inverted();
        return std::make_shared<SpotLight>(light2world, _pMedium, I * sc, coneangle, coneangle - conedelta);
    }


    LightPtr CreateGoniometricLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium /*= nullptr*/)
    {
        Spectrum I   = _param.FindOneSpectrum("I", Spectrum(1.0));
        Spectrum sc  = _param.FindOneSpectrum("scale", Spectrum(1.0));
        std::string texname = _param.FindOneFilename("mapname", "");
        return std::make_shared<GonioPhotometricLight>(_lightToWorld, _pMedium, I * sc, texname);
    }

    LightPtr CreateProjectionLight(const Transform& _lightToWorld, const ParamSet& _param, const Medium* _pMedium /*= nullptr*/)
    {
        Spectrum I  = _param.FindOneSpectrum("I", Spectrum(1.0));
        Spectrum sc = _param.FindOneSpectrum("scale", Spectrum(1.0));
        float fov   = _param.FindOneFloat("fov", 45.);
        std::string texname = _param.FindOneFilename("mapname", "");
        return std::make_shared<ProjectionLight>(_lightToWorld, _pMedium, I * sc, texname, fov);
    }

    LightPtr CreateDistantLight(const Transform& _lightToWorld, const ParamSet& _param)
    {
        Spectrum L      = _param.FindOneSpectrum("L", Spectrum(1.0));
        Spectrum sc     = _param.FindOneSpectrum("scale", Spectrum(1.0));
        Vector3f from   = _param.FindOnePoint3f("from", Vector3f(0, 0, 0));
        Vector3f to     = _param.FindOnePoint3f("to", Vector3f(0, 0, 1));
        Vector3f dir = from - to;
        return std::make_shared<DistantLight>(_lightToWorld, L * sc, dir);
    }

    LightPtr CreateInfiniteLight(const Transform& _lightToWorld, const ParamSet& _param)
    {
        Spectrum L          = _param.FindOneSpectrum("L", Spectrum(1.0));
        Spectrum sc         = _param.FindOneSpectrum("scale", Spectrum(1.0));
        std::string texmap  = _param.FindOneFilename("mapname", "");
        int nSamples        = _param.FindOneInt("samples", _param.FindOneInt("nsamples", 1));
        if (PbrtOptions.quickRender) 
            nSamples = std::max(1, nSamples / 4);
        return std::make_shared<InfiniteAreaLight>(_lightToWorld, L * sc, nSamples, texmap);
    }

    LightPtr CreateDiffuseAreaLight(const Transform& _lightToWorld, const ParamSet& _param, const ShapePtr& _shape, const Medium* _pMedium /*= nullptr*/)
    {
        Spectrum L    = _param.FindOneSpectrum("L", Spectrum(1.0));
        Spectrum sc   = _param.FindOneSpectrum("scale", Spectrum(1.0));
        int nSamples  = _param.FindOneInt("samples", _param.FindOneInt("nsamples", 1));
        bool twoSided = _param.FindOneBool("twosided", false);
        if (PbrtOptions.quickRender) 
            nSamples = std::max(1, nSamples / 4);
      
        return std::make_shared<DiffuseAreaLight>(_lightToWorld, _pMedium, L * sc,  nSamples, _shape, twoSided );
    }

    

   

}


