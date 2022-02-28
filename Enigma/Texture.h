#pragma once
#include <memory>
#include <map>
#include "Transform.h"
#include "MipMap.h"
#include "MathCommon.h"
#include "IO.h"
#include "TexInfo.h"
#include "Interaction.h"
#include "Defines.h"


namespace RayTrace
{
	

#pragma region TextureMapping
    struct TextureMapping2DParams
    {
        float m_s;
        float m_t;
        float m_dsdx,
              m_dsdy;
        float m_dtdx,
              m_dtdy;
    };

    struct TextureMapping3DParams
    {
        Vector3f m_point;
        Vector3f m_dpdx;
        Vector3f m_dpdy;
    };


    class TextureMapping2D
    {
    public:
        virtual Vector2f map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const = 0;

    };

    class UVMapping2D : public TextureMapping2D {
    public:
        UVMapping2D(float _ssu = 1.0f, float _ssv = 1.0f, float _ddu = 0.0f, float _ddv = 0.f);

        Vector2f map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const override;

    private:
        float m_su, m_sv;
        float m_du, m_dv;
    };

    class SphericalMapping2D : public TextureMapping2D {
    public:
        // SphericalMapping2D Public Methods
        SphericalMapping2D(const Transform& toSph);

        Vector2f map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const override;

    private:
        Vector2f sphere(const Vector3f& p) const;

        Transform m_worldToTexture;
    };

    class CylindricalMapping2D : public TextureMapping2D {
    public:
        CylindricalMapping2D(const Transform& toSph);

        Vector2f map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const override;

    private:
        Vector2f        cylinder(const Vector3f& p) const;

        Transform m_worldToTexture;
    };

    class PlanarMapping2D : public TextureMapping2D {
    public:
        PlanarMapping2D(const Vector3f& _vs, const Vector3f& _vt, float _ds = 0.f, float _dt = 0.f);

        Vector2f map(const SurfaceInteraction& si, Vector2f* dstdx, Vector2f* dstdy) const override;

    private:
        Vector3f m_vs, m_vt;
        float		  m_ds, m_dt;
    };

    class TextureMapping3D
    {
    public:
       virtual Vector3f map(const SurfaceInteraction& si, Vector3f* dpdx, Vector3f* dpdy) const = 0;
    };

    class IdentityMapping3D : public TextureMapping3D
    {
    public:
        IdentityMapping3D(const Transform& _wtt);
        Vector3f map(const SurfaceInteraction& si, Vector3f* dpdx, Vector3f* dpdy) const override;

    private:
        Transform m_worldToTexture;
    };

#pragma endregion


#pragma region Texture
	template <typename T> class Texture  {
	public:
		// Texture Interface
		virtual ~Texture() { }
        //virtual T Evaluate(const DifferentialGeometry&) const = 0;
        virtual T Evaluate(const SurfaceInteraction&) const = 0;

	};

    template <typename T> 
    class ConstantTexture : public Texture<T>
    {
    public:
        ConstantTexture(const T& _val) : m_value(_val) {}
        T Evaluate(const SurfaceInteraction&) const override
        {
            return m_value;
        }

    private:
        T m_value;
    };

  

    template <typename T1, typename T2>
    class ScaleTexture : public Texture<T2>
    {       
    public:
        using TexPtr1 = std::shared_ptr<Texture<T1>>;
        using TexPtr2 = std::shared_ptr<Texture<T2>>;

        ScaleTexture( const TexPtr1& _t1, const TexPtr2& _t2) 
            : m_tex1( _t1 )
            , m_tex2( _t2 )
        {}

        T2 Evaluate(const SurfaceInteraction& si) const override {
            return m_tex1->Evaluate(si) * m_tex2->Evaluate(si);
        }

    private:
        TexPtr1 m_tex1;
        TexPtr2 m_tex2;
    };


    template <typename T>
    class MixTexture : public Texture<T>
    {
    public:       
        using TexPtr = std::shared_ptr<Texture<T>>;
       
        MixTexture(const TexPtr& _t1, const TexPtr& _t2, const FloatTexturePtr& _amount)
            : m_t1(_t1)
            , m_t2(_t2)
            , m_amount(_amount)
        {

        }

        T Evaluate(const SurfaceInteraction& si) const override {
            auto val1 = m_t1->Evaluate(si);
            auto val2 = m_t2->Evaluate(si);
            float amount = m_amount->Evaluate(si);
            return Lerp(val1, val2, amount);
        }

    private:
        TexPtr          m_t1, m_t2;
        FloatTexturePtr m_amount;
    };

  

    // CheckerboardTexture Declarations
    template <typename T>
    class Checkerboard2DTexture : public Texture<T> {
    public:
        // Checkerboard2DTexture Public Methods
        Checkerboard2DTexture(const TextureMapping2DPtr& mapping,
            const std::shared_ptr<Texture<T>>& tex1,
            const std::shared_ptr<Texture<T>>& tex2,
            eAAMethod aaMethod)
            : m_mapping(mapping)
            , m_tex1(tex1)
            , m_tex2(tex2)
            , m_aaMethod(aaMethod) {}
        T Evaluate(const SurfaceInteraction& si) const {
            Vector2f dstdx, dstdy;
            Point2f st = m_mapping->map(si, &dstdx, &dstdy);
            if (m_aaMethod == eAAMethod::None) {
                // Point sample _Checkerboard2DTexture_
                if (((int)std::floor(st[0]) + (int)std::floor(st[1])) % 2 == 0)
                    return m_tex1->Evaluate(si);
                return m_tex2->Evaluate(si);
            }
            else {
                // Compute closed-form box-filtered _Checkerboard2DTexture_ value

                // Evaluate single check if filter is entirely inside one of them
                float ds = std::max(std::abs(dstdx[0]), std::abs(dstdy[0]));
                float dt = std::max(std::abs(dstdx[1]), std::abs(dstdy[1]));
                float s0 = st[0] - ds, s1 = st[0] + ds;
                float t0 = st[1] - dt, t1 = st[1] + dt;
                if (std::floor(s0) == std::floor(s1) &&
                    std::floor(t0) == std::floor(t1)) {
                    // Point sample _Checkerboard2DTexture_
                    if (((int)std::floor(st[0]) + (int)std::floor(st[1])) % 2 == 0)
                        return m_tex1->Evaluate(si);
                    return m_tex2->Evaluate(si);
                }

                // Apply box filter to checkerboard region
                auto bumpInt = [](float x) {
                    return (int)std::floor(x / 2) +
                        2 * std::max(x / 2 - (int)std::floor(x / 2) - (float)0.5,
                            (float)0);
                };
                float sint = (bumpInt(s1) - bumpInt(s0)) / (2 * ds);
                float tint = (bumpInt(t1) - bumpInt(t0)) / (2 * dt);
                float area2 = sint + tint - 2 * sint * tint;
                if (ds > 1 || dt > 1) area2 = .5f;
                return (1 - area2) * m_tex1->Evaluate(si) +
                    area2 * m_tex2->Evaluate(si);
            }
        }

    private:
        // Checkerboard2DTexture Private Data
        std::shared_ptr<TextureMapping2D> m_mapping;
        const std::shared_ptr<Texture<T>> m_tex1, m_tex2;
        const eAAMethod m_aaMethod;
    };

    template <typename T>
    class Checkerboard3DTexture : public Texture<T> {
    public:
        // Checkerboard3DTexture Public Methods
        Checkerboard3DTexture(const TextureMapping3DPtr& mapping,
            const std::shared_ptr<Texture<T>>& tex1,
            const std::shared_ptr<Texture<T>>& tex2)
            : m_mapping(mapping)
            , m_tex1(tex1)
            , m_tex2(tex2) {}
        T Evaluate(const SurfaceInteraction& si) const
        {
            Vector3f dpdx, dpdy;
            Point3f p = m_mapping->map(si, &dpdx, &dpdy);
            if (((int)std::floor(p.x) + (int)std::floor(p.y) + (int)std::floor(p.z)) % 2 ==0)
                return m_tex1->Evaluate(si);
            else
                return m_tex2->Evaluate(si);
        }

    private:
        // Checkerboard3DTexture Private Data
        std::shared_ptr<TextureMapping3D> m_mapping;
        std::shared_ptr<Texture<T>> m_tex1, m_tex2;
    };
   


    template <typename T>
    class BiLerpTexture : public Texture<T>
    {
    public:
        BiLerpTexture(const TextureMapping2DPtr& _mapping, const T& _t00, const T& _t01, const T& _t10, const T& _t11)
            : m_pMapping( _mapping )
            , m_t00(_t00 )
            , m_t01(_t01)
            , m_t10(_t10)
            , m_t11(_t11)
        {

        }

        T Evaluate(const SurfaceInteraction& si) const override {
            Vector2f dstdx, dstdy;
            Vector2f st = m_pMapping->map(si, &dstdx, &dstdy);
            return (1 - st[0]) * (1 - st[1]) * m_t00 + (1 - st[0]) * (st[1]) * m_t01 +
                  (st[0]) * (1 - st[1]) * m_t10 + (st[0]) * (st[1]) * m_t11;
        }

    private:
        TextureMapping2DPtr m_pMapping;
        T m_t00, m_t01, m_t10, m_t11;
    };


    class UVSpectrumTexture : public Texture<Spectrum> {
    public:
        // UVTexture Public Methods
        UVSpectrumTexture(const TextureMapping2DPtr& _m);
        ~UVSpectrumTexture() {
            
        }
        Spectrum Evaluate(const SurfaceInteraction& si) const override;
    private:
        TextureMapping2DPtr  m_mapping;
    };

    template <typename TMemory, typename TReturn>
    class ImageTexture : public Texture<TReturn>
    {
    public:    
        using TextureDataPtr = std::shared_ptr<MipMap<TMemory>>;
        ImageTexture(const TextureMapping2DPtr& _m, const TexInfo& _tInfo)
            : m_map(_m)
            , m_texInfo(_tInfo)
        {
            auto result = getTexture(_tInfo);
            assert(result != nullptr);
            m_mipMap = result;
        }

        TReturn Evaluate(const SurfaceInteraction& si) const override;



    private:
        TextureDataPtr  getTexture(const TexInfo& _info)
        {
            if (s_textureCache.find(_info) != std::end(s_textureCache))
                return s_textureCache[_info];

            TextureDataPtr retVal = nullptr;
            int width = 0;
            int height = 0;
            int numChannels = 0;
            eTextureType type;
            auto colors = ReadImage(_info.m_filename.c_str(), &width, &height, numChannels, type);
            auto pixels = PixelDataToSpectrum(colors, numChannels);
            colors.clear();
            if (!pixels.empty())
            {
                // / Flip image in y; texture coordinate space has(0, 0) at the lower
                // left corner.
                for (int y = 0; y < height / 2; ++y)
                    for (int x = 0; x < width; ++x) {
                        int o1 = y * width + x;
                        int o2 = (height - 1 - y) * width + x;
                        std::swap(pixels[o1], pixels[o2]);
                    }
                
                
                std::vector<TMemory> convertedTexels( width * height );
                for (int i = 0; i < width * height; ++i)
                    ImageTexture::ConvertIn(pixels[i], &convertedTexels[i], _info.m_scale, _info.m_gamma );
                
                
                retVal = std::make_shared< MipMap<TMemory>>( width, height, convertedTexels, _info.m_doTrilinear, _info.m_maxAniso, _info.m_wrapMode );
            }
            else
            {
                TMemory oneVal = powf(_info.m_scale, _info.m_gamma );
                std::vector<TMemory> val(1, oneVal);
                retVal = std::make_shared< MipMap<TMemory>>(1, 1, val, _info.m_doTrilinear, _info.m_maxAniso, _info.m_wrapMode);
            }
           
            s_textureCache[_info] = retVal;
            return retVal;
        }


        static void ConvertIn(const RGBSpectrum& _from, RGBSpectrum* _to, 
            float _scale, float _gamma)
        {
            RGBSpectrum retVal = _from * _scale;    
            *_to =  Pow(retVal, _gamma );
        }

        static void ConvertIn(const RGBSpectrum& _from, float* _to, float _scale, float _gamma)
        {
            *_to = powf(_from.y() * _scale, _gamma);
        }

        static void ConvertOut(const RGBSpectrum& _from, Spectrum* _to) {
            float rgb[3];
            _from.ToRGB(rgb);
            *_to = Spectrum::FromRGB(rgb);
        }

        static void ConvertOut(float _from, float* _to) {
            *_to = _from;
        }

        static std::map<TexInfo, TextureDataPtr> s_textureCache;

        TextureMapping2DPtr m_map = nullptr;
        TextureDataPtr      m_mipMap = nullptr;
        TexInfo             m_texInfo;      
    };


    template <typename TMemory, typename TReturn>
    TReturn ImageTexture<TMemory, TReturn>::Evaluate(const SurfaceInteraction& si) const
    {
        Vector2f dstdx, dstdy;
        Vector2f st = m_map->map(si, &dstdx, &dstdy);

        TMemory mem = m_mipMap->lookup( st, dstdx, dstdy);
        TReturn ret;
        ConvertOut(mem, &ret);
        return ret;
    }

    template <typename TMemory, typename TReturn>
    std::map<TexInfo, typename ImageTexture<TMemory, TReturn>::TextureDataPtr> ImageTexture<TMemory, TReturn>::s_textureCache;

#pragma endregion	

      FloatTexture* CreateConstantFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateScaleFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateMixFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateBilerpFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateImageFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateUVFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateCheckerboardFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateDotsFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateFBmFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateWrinkledFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateMarbleFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreateWindyFloatTexture(const Transform& _texToWorld, const TextureParams& _param);
      FloatTexture* CreatePtexFloatTexture(const Transform& _texToWorld, const TextureParams& _param);


      SpectrumTexture* CreateConstantSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateScaleSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateMixSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateBilerpSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateImageSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateUVSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateCheckerboardSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateDotsSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateFBmSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateWrinkledSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateMarbleSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreateWindySpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);
      SpectrumTexture* CreatePtexSpectrumTexture(const Transform& _texToWorld, const TextureParams& _param);


}