#include "ParameterSet.h"
#include "Error.h"
#include "Texture.h"



namespace RayTrace
{
    
#pragma region TextureMapping

    UVMapping2D::UVMapping2D(float _ssu, float _ssv, float _ddu, float _ddv) 
        : m_su(_ssu)
        , m_sv(_ssv)
        , m_du(_ddu)
        , m_dv(_ddv)
    {

    }


    Vector2f UVMapping2D::map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const
    {
        // Compute texture differentials for 2D identity mapping
        *dstdx = Vector2f(m_su * si.m_dudx, m_sv * si.m_dvdx);
        *dstdy = Vector2f(m_su * si.m_dudy, m_sv * si.m_dvdy);
        return Vector2f(m_su * si.m_uv[0] + m_du, m_sv * si.m_uv[1] + m_dv);

    }

    SphericalMapping2D::SphericalMapping2D(const Transform& toSph)
        : m_worldToTexture(toSph)
    {

    }    

    Vector2f SphericalMapping2D::map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const
    {
        auto st = sphere(si.m_p);
        // Compute texture coordinate differentials for sphere $(u,v)$ mapping
        const float delta = .1f;
        auto stDeltaX = sphere(si.m_p + delta * si.m_dpdx);
        *dstdx = (stDeltaX - st) / delta;

        auto stDeltaY = sphere(si.m_p + delta * si.m_dpdy);
        *dstdy = (stDeltaY - st) / delta;

        // Handle sphere mapping discontinuity for coordinate differentials
        if ((*dstdx)[1] > .5)
            (*dstdx)[1] = 1 - (*dstdx)[1];
        else if ((*dstdx)[1] < -.5f)
            (*dstdx)[1] = -((*dstdx)[1] + 1);
        if ((*dstdy)[1] > .5)
            (*dstdy)[1] = 1 - (*dstdy)[1];
        else if ((*dstdy)[1] < -.5f)
            (*dstdy)[1] = -((*dstdy)[1] + 1);
        return st;
    }

    Vector2f SphericalMapping2D::sphere(const Vector3f& p) const
    {
        Vector3f vec = Normalize(m_worldToTexture.transformPoint(p) - Point3f(0, 0, 0));
        float theta = SphericalTheta(vec), phi = SphericalPhi(vec);
        return Point2f(theta * INV_PI, phi * INV_TWO_PI);
    }

    CylindricalMapping2D::CylindricalMapping2D(const Transform& toSph) 
        : m_worldToTexture(toSph)
    {

    }    

    Vector2f CylindricalMapping2D::map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const
    {
        auto st = cylinder(si.m_p);
        // Compute texture coordinate differentials for cylinder $(u,v)$ mapping
        const float delta = .01f;
        auto stDeltaX = cylinder(si.m_p + delta * si.m_dpdx);
        *dstdx = (stDeltaX - st) / delta;
        if ((*dstdx)[1] > .5)
            (*dstdx)[1] = 1.f - (*dstdx)[1];
        else if ((*dstdx)[1] < -.5f)
            (*dstdx)[1] = -((*dstdx)[1] + 1);
        auto  stDeltaY = cylinder(si.m_p + delta * si.m_dpdy);
        *dstdy = (stDeltaY - st) / delta;
        if ((*dstdy)[1] > .5)
            (*dstdy)[1] = 1.f - (*dstdy)[1];
        else if ((*dstdy)[1] < -.5f)
            (*dstdy)[1] = -((*dstdy)[1] + 1);
        return st;
    }

    
    Vector2f CylindricalMapping2D::cylinder(const Vector3f& p) const
    {
        Vector3f vec = Normalize(m_worldToTexture.transformPoint(p) - Vector3f(.0f));
        float s = (PI + atan2f(vec.y, vec.x)) / (2.f * PI);
        float t = vec.z;

        return Vector2f(s, t);

    }

    PlanarMapping2D::PlanarMapping2D(const Vector3f& _vs, const Vector3f& _vt, float _ds, float _dt)
        : m_vs(_vs)
        , m_vt(_vt)
        , m_ds(_ds)
        , m_dt(_dt)
    {

    }

    Vector2f PlanarMapping2D::map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const
    {
        Vector3f vec(si.m_p);
        *dstdx = Vector2f(Dot(si.m_dpdx, m_vs), Dot(si.m_dpdx, m_vt));
        *dstdy = Vector2f(Dot(si.m_dpdy, m_vs), Dot(si.m_dpdy, m_vt));
        return Vector2f(m_ds + Dot(vec, m_vs), m_dt + Dot(vec, m_vt));
    }

#pragma endregion

    IdentityMapping3D::IdentityMapping3D(const Transform& _wtt)
        : m_worldToTexture( _wtt )
    {

    }
        

    Vector3f IdentityMapping3D::map(const SurfaceInteraction& si, Vector3f* dpdx, Vector3f* dpdy) const
    {
         *dpdx = m_worldToTexture.transformVector(si.m_dpdx);
         *dpdy = m_worldToTexture.transformVector(si.m_dpdy);
         return m_worldToTexture.transformPoint(si.m_p);
    }

#pragma region Texture
    




    UVSpectrumTexture::UVSpectrumTexture(const TextureMapping2DPtr& _m) 
        : m_mapping(_m)
    {
        
    }

    Spectrum UVSpectrumTexture::Evaluate(const SurfaceInteraction& si) const
    {
        Vector2f dstdx, dstdy;
        Vector2f st = m_mapping->map(si, &dstdx, &dstdy);
        float rgb[3] = { st[0] - std::floor(st[0]), st[1] - std::floor(st[1]), 0 };
        return Spectrum::FromRGB(rgb);
    }

    
    FloatTexture* CreateConstantFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return new ConstantTexture<float>(tp.FindFloat("value", 1.f));
    }

    FloatTexture* CreateScaleFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return new ScaleTexture<float, float>(tp.GetFloatTexture("tex1", 1.f),
                                              tp.GetFloatTexture("tex2", 1.f));
    }

    FloatTexture* CreateMixFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return new MixTexture<float>(tp.GetFloatTexture("tex1", 0.f),
                                     tp.GetFloatTexture("tex2", 1.f),
                                     tp.GetFloatTexture("amount", 0.5f));
    }

    FloatTexture* CreateBilerpFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        // Initialize 2D texture mapping _map_ from _tp_
        TextureMapping2DPtr map;
        std::string type = tp.FindString("mapping", "uv");
        if (type == "uv") {
            float su = tp.FindFloat("uscale", 1.);
            float sv = tp.FindFloat("vscale", 1.);
            float du = tp.FindFloat("udelta", 0.);
            float dv = tp.FindFloat("vdelta", 0.);
            map.reset(new UVMapping2D(su, sv, du, dv));
        }
        else if (type == "spherical")
            map.reset(new SphericalMapping2D(Inverse(_texToWorld)));
        else if (type == "cylindrical")
            map.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
        else if (type == "planar")
            map.reset(new PlanarMapping2D(tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                tp.FindFloat("udelta", 0.f),
                tp.FindFloat("vdelta", 0.f)));
        else {
            Error("2D texture mapping \"%s\" unknown", type.c_str());
            map.reset(new UVMapping2D);
        }
        return new BiLerpTexture<float>(
            map, tp.FindFloat("v00", 0.f), tp.FindFloat("v01", 1.f),
            tp.FindFloat("v10", 0.f), tp.FindFloat("v11", 1.f));
    }

    FloatTexture* CreateImageFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        // Initialize 2D texture mapping _map_ from _tp_
        TextureMapping2DPtr texMap2d;
        std::string type = tp.FindString("mapping", "uv");
        if (type == "uv") {
            float su = tp.FindFloat("uscale", 1.);
            float sv = tp.FindFloat("vscale", 1.);
            float du = tp.FindFloat("udelta", 0.);
            float dv = tp.FindFloat("vdelta", 0.);
            texMap2d.reset(new UVMapping2D(su, sv, du, dv));
        }
        else if (type == "spherical")
            texMap2d.reset(new SphericalMapping2D(Inverse(_texToWorld)));
        else if (type == "cylindrical")
            texMap2d.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
        else if (type == "planar")
            texMap2d.reset(new PlanarMapping2D(tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                tp.FindFloat("udelta", 0.f),
                tp.FindFloat("vdelta", 0.f)));
        else {
            Error("2D texture mapping \"%s\" unknown", type.c_str());
            texMap2d.reset(new UVMapping2D);
        }

        // Initialize _ImageTexture_ parameters
        float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
        bool trilerp = tp.FindBool("trilinear", false);
        std::string wrap = tp.FindString("wrap", "repeat");
        eImageWrapMode wrapMode = eImageWrapMode::TEXTURE_WRAP_REPEAT;
        if (wrap == "black")
            wrapMode = eImageWrapMode::TEXTURE_WRAP_BLACK;
        else if (wrap == "clamp")
            wrapMode = eImageWrapMode::TEXTURE_WRAP_CLAMP;
        float scale = tp.FindFloat("scale", 1.f);
        std::string filename = tp.FindFilename("filename");
        bool gamma = tp.FindBool("gamma", HasExtension(filename, ".tga") ||
            HasExtension(filename, ".png"));
        TexInfo info(filename, trilerp, maxAniso, wrapMode, scale, gamma);
        return new ImageTexture<float, float>(texMap2d, info);
    }

    FloatTexture* CreateUVFloatTexture(const Transform& _texToWorld, const TextureParams& _param)
    {
        return nullptr;
    }

    SpectrumTexture* CreateUVSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param)
    {
        // Initialize 2D texture mapping _map_ from _tp_
        std::string type = _param.FindString("mapping", "uv");
        TextureMapping2DPtr map = NULL;
        if (type == "uv") {
            float su = 1.f;
            float sv = 1.f;
            float du = 0.f;
            float dv = 0.f;
            map = std::shared_ptr<UVMapping2D>(new UVMapping2D(su, sv, du, dv));
        }

        else if (type == "spherical")   map = std::shared_ptr<SphericalMapping2D>(new SphericalMapping2D(_texToWorld.inverted()));
        else if (type == "cylindrical") map = std::shared_ptr<CylindricalMapping2D>(new CylindricalMapping2D(_texToWorld.inverted()));
        else if (type == "planar")      map = std::shared_ptr< PlanarMapping2D>(new PlanarMapping2D(Vector3f(1, 0, 0), Vector3f(0, 1, 0), 0.f, 0.f));
        else {
            map = std::shared_ptr<UVMapping2D>(new UVMapping2D(1, 1, 0, 0));
        }
        return new UVSpectrumTexture(map);
    }

    FloatTexture* CreateCheckerboardFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        int dim = tp.FindInt("dimension", 2);
        if (dim != 2 && dim != 3) {
            Error("%d dimensional checkerboard texture not supported", dim);
            return nullptr;
        }
        std::shared_ptr<Texture<float>> tex1 = tp.GetFloatTexture("tex1", 1.f);
        std::shared_ptr<Texture<float>> tex2 = tp.GetFloatTexture("tex2", 0.f);
        if (dim == 2) {
            // Initialize 2D texture mapping _map_ from _tp_
            std::shared_ptr<TextureMapping2D> map;
            std::string type = tp.FindString("mapping", "uv");
            if (type == "uv") {
                float su = tp.FindFloat("uscale", 1.);
                float sv = tp.FindFloat("vscale", 1.);
                float du = tp.FindFloat("udelta", 0.);
                float dv = tp.FindFloat("vdelta", 0.);
                map.reset(new UVMapping2D(su, sv, du, dv));
            }
            else if (type == "spherical")
                map.reset(new SphericalMapping2D(Inverse(_texToWorld)));
            else if (type == "cylindrical")
                map.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
            else if (type == "planar")
                map.reset(new PlanarMapping2D(
                    tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                    tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                    tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f)));
            else {
                Error("2D texture mapping \"%s\" unknown", type.c_str());
                map.reset(new UVMapping2D);
            }

            // Compute _aaMethod_ for _CheckerboardTexture_
            std::string aa = tp.FindString("aamode", "closedform");
            eAAMethod aaMethod;
            if (aa == "none")
                aaMethod = eAAMethod::None;
            else if (aa == "closedform")
                aaMethod = eAAMethod::ClosedForm;
            else {
                Warning(
                    "Antialiasing mode \"%s\" not understood by "
                    "Checkerboard2DTexture; using \"closedform\"",
                    aa.c_str());
                aaMethod = eAAMethod::ClosedForm;
            }
            return new Checkerboard2DTexture<float>(map, tex1, tex2, aaMethod);
        }
        else {
            // Initialize 3D texture mapping _map_ from _tp_
            std::shared_ptr<TextureMapping3D> map(new IdentityMapping3D(_texToWorld));
            return new Checkerboard3DTexture<float>(map, tex1, tex2);
        }
    }

    FloatTexture* CreateDotsFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    FloatTexture* CreateFBmFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    FloatTexture* CreateWrinkledFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    FloatTexture* CreateMarbleFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    FloatTexture* CreateWindyFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    FloatTexture* CreatePtexFloatTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }


    SpectrumTexture* CreateConstantSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return new ConstantTexture<Spectrum>( tp.FindSpectrum("value", Spectrum(1.f)));
    }

    SpectrumTexture* CreateScaleSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return new ScaleTexture<Spectrum, Spectrum>(tp.GetSpectrumTexture("tex1", 1.f),
                                                    tp.GetSpectrumTexture("tex2", 1.f));
    }

    SpectrumTexture* CreateMixSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
       return new MixTexture<Spectrum>(tp.GetSpectrumTexture("tex1", 0.f),
                                       tp.GetSpectrumTexture("tex2", 1.f),
                                       tp.GetFloatTexture("amount", 0.5f));
    }

    SpectrumTexture* CreateBilerpSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        // Initialize 2D texture mapping _map_ from _tp_
        TextureMapping2DPtr map;
        std::string type = tp.FindString("mapping", "uv");
        if (type == "uv") {
            float su = tp.FindFloat("uscale", 1.);
            float sv = tp.FindFloat("vscale", 1.);
            float du = tp.FindFloat("udelta", 0.);
            float dv = tp.FindFloat("vdelta", 0.);
            map.reset(new UVMapping2D(su, sv, du, dv));
        }
        else if (type == "spherical")
            map.reset(new SphericalMapping2D(Inverse(_texToWorld)));
        else if (type == "cylindrical")
            map.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
        else if (type == "planar")
            map.reset(new PlanarMapping2D(tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                tp.FindFloat("udelta", 0.f),
                tp.FindFloat("vdelta", 0.f)));
        else {
            Error("2D texture mapping \"%s\" unknown", type.c_str());
            map.reset(new UVMapping2D);
        }
        return new BiLerpTexture<Spectrum>(
            map, tp.FindFloat("v00", 0.f), tp.FindFloat("v01", 1.f),
            tp.FindFloat("v10", 0.f), tp.FindFloat("v11", 1.f));
    }

    SpectrumTexture* CreateImageSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        // Initialize 2D texture mapping _map_ from _tp_
        TextureMapping2DPtr texMap2d;
        std::string type = tp.FindString("mapping", "uv");
        if (type == "uv") {
            float su = tp.FindFloat("uscale", 1.);
            float sv = tp.FindFloat("vscale", 1.);
            float du = tp.FindFloat("udelta", 0.);
            float dv = tp.FindFloat("vdelta", 0.);
            texMap2d.reset(new UVMapping2D(su, sv, du, dv));
        }
        else if (type == "spherical")
            texMap2d.reset(new SphericalMapping2D(Inverse(_texToWorld)));
        else if (type == "cylindrical")
            texMap2d.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
        else if (type == "planar")
            texMap2d.reset(new PlanarMapping2D(tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                tp.FindFloat("udelta", 0.f),
                tp.FindFloat("vdelta", 0.f)));
        else {
            Error("2D texture mapping \"%s\" unknown", type.c_str());
            texMap2d.reset(new UVMapping2D);
        }

        // Initialize _ImageTexture_ parameters
        float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
        bool trilerp = tp.FindBool("trilinear", false);
        std::string wrap = tp.FindString("wrap", "repeat");
        eImageWrapMode wrapMode = eImageWrapMode::TEXTURE_WRAP_REPEAT;
        if (wrap == "black")
            wrapMode = eImageWrapMode::TEXTURE_WRAP_BLACK;
        else if (wrap == "clamp")
            wrapMode = eImageWrapMode::TEXTURE_WRAP_CLAMP;
        float scale = tp.FindFloat("scale", 1.f);
        std::string filename = tp.FindFilename("filename");
        bool gamma = tp.FindBool("gamma", HasExtension(filename, ".tga") ||
            HasExtension(filename, ".png"));
        TexInfo info(filename, trilerp, maxAniso, wrapMode, scale, gamma);
        return new ImageTexture<RGBSpectrum, Spectrum>(texMap2d, info);
    }

    

    SpectrumTexture* CreateCheckerboardSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        int dim = tp.FindInt("dimension", 2);
        if (dim != 2 && dim != 3) {
            Error("%d dimensional checkerboard texture not supported", dim);
            return nullptr;
        }
        std::shared_ptr<Texture<Spectrum>> tex1 =
            tp.GetSpectrumTexture("tex1", 1.f);
        std::shared_ptr<Texture<Spectrum>> tex2 =
            tp.GetSpectrumTexture("tex2", 0.f);
        if (dim == 2) {
            // Initialize 2D texture mapping _map_ from _tp_
            std::shared_ptr<TextureMapping2D> map;
            std::string type = tp.FindString("mapping", "uv");
            if (type == "uv") {
                float su = tp.FindFloat("uscale", 1.);
                float sv = tp.FindFloat("vscale", 1.);
                float du = tp.FindFloat("udelta", 0.);
                float dv = tp.FindFloat("vdelta", 0.);
                map.reset(new UVMapping2D(su, sv, du, dv));
            }
            else if (type == "spherical")
                map.reset(new SphericalMapping2D(Inverse(_texToWorld)));
            else if (type == "cylindrical")
                map.reset(new CylindricalMapping2D(Inverse(_texToWorld)));
            else if (type == "planar")
                map.reset(new PlanarMapping2D(
                    tp.FindVector3f("v1", Vector3f(1, 0, 0)),
                    tp.FindVector3f("v2", Vector3f(0, 1, 0)),
                    tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f)));
            else {
                Error("2D texture mapping \"%s\" unknown", type.c_str());
                map.reset(new UVMapping2D);
            }

            // Compute _aaMethod_ for _CheckerboardTexture_
            std::string aa = tp.FindString("aamode", "closedform");
            eAAMethod aaMethod;
            if (aa == "none")
                aaMethod = eAAMethod::None;
            else if (aa == "closedform")
                aaMethod = eAAMethod::ClosedForm;
            else {
                Warning(
                    "Antialiasing mode \"%s\" not understood by "
                    "Checkerboard2DTexture; using \"closedform\"",
                    aa.c_str());
                aaMethod = eAAMethod::ClosedForm;
            }
            return new Checkerboard2DTexture<Spectrum>(map, tex1, tex2,
                aaMethod);
        }
        else {
            // Initialize 3D texture mapping _map_ from _tp_
            std::shared_ptr<TextureMapping3D> map(new IdentityMapping3D(_texToWorld));
            return new Checkerboard3DTexture<Spectrum>(map, tex1, tex2);
        }
    }

    SpectrumTexture* CreateDotsSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    SpectrumTexture* CreateFBmSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    SpectrumTexture* CreateWrinkledSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    SpectrumTexture* CreateMarbleSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    SpectrumTexture* CreateWindySpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

    SpectrumTexture* CreatePtexSpectrumTexture(const Transform& _texToWorld, const TextureParams& tp)
    {
        return nullptr;
    }

#pragma endregion

  

}

