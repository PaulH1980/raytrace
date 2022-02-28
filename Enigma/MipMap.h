#pragma once
#include <vector>
#include <assert.h>
#include <memory>
#include "MathCommon.h"
#include "Memory.h"

#include "Spectrum.h"
#include "IO.h"
#include "TexInfo.h"

namespace RayTrace
{
    // Texture Function Definitions
    static inline float Lanczos(float x, float tau = 2.f) {
        x = fabsf(x);
        if (x < 1e-5) return 1;
        if (x > 1.)    return 0;
        x *= PI;
        float s = sinf(x * tau) / (x * tau);
        float lanczos = sinf(x) / x;
        return s * lanczos;
    }

    struct LookUpInfo
    {
        float m_s;
        float m_t;
        float m_ds0;
        float m_dt0;
        float m_ds1;
        float m_dt1;
    };

    struct ResampleWeight {
        int     m_firstTexel = 0;
        float   m_weight[4] = { 0.f };
    };
    
    
    template<typename T>
    class MipMap {
    public:
      
        using PyramidData = BlockedArray<T, 2 >;
        using PyramidDataUPtr = std::unique_ptr<PyramidData>;
      
        MipMap()
            : m_doTrilinear(false)
            , m_maxAnisotropy(0.0f)
            , m_wrapMode(eImageWrapMode::TEXTURE_WRAP_BLACK)
            , m_width(0)
            , m_height(0)
            , m_nLevels(0)
        {
            initLUT();
        }


        MipMap( int _width, int _height, std::vector<T> _imgData,
                bool _doTri = false, float _maxAniso = 8.0f, eImageWrapMode _wrapMode = eImageWrapMode::TEXTURE_WRAP_REPEAT);
       
        ~MipMap() {

        }

        uint32_t    width() const { return  m_width; }
        uint32_t    height() const { return m_height; }
        uint32_t    levels() const { return m_nLevels; }
        const T&    texel(uint32_t level, int s, int t) const;
        T           lookup(const Vector2f& st, Vector2f dstdx,  Vector2f dstdy);
        T           lookup(const Vector2f& st, float width = 0.0f);

      

    private:
        static void     initLUT();

        float                       clamp(float v) { return std::clamp(v, 0.f, INFINITY); }
        RGBSpectrum         clamp(const RGBSpectrum& v) { return v.Clamp(0.f, INFINITY); }
        SampledSpectrum     clamp(const SampledSpectrum& v) { return v.Clamp(0.f, INFINITY); }
        T                           triangle(int level, const Vector2f& st) const;
        T                           EWA(int level, Vector2f st, Vector2f dst0, Vector2f dst1) const;

        std::vector<ResampleWeight> resampleWeights(int _oldWidth, int _newWidth) const;
        std::vector<T>              resizeImage(const std::vector<T>& _imgData, int _oldWidth, int _oldHeight, int& _newWidth, int& _newHeight);
       


        uint32_t        m_width, 
                        m_height, 
                        m_nLevels;
        // MIPMap Private Data
        bool            m_doTrilinear;
        float           m_maxAnisotropy;
        eImageWrapMode  m_wrapMode;
        

        std::vector<PyramidDataUPtr> m_pyramidData; 
       
#define WEIGHT_LUT_SIZE 128
        static float*     s_weightLut; 

    };

    
   
  
 

    template<typename T>
    MipMap<T>::MipMap(int _width, int _height,  std::vector<T> _imgData, bool _doTri, float _maxAniso, eImageWrapMode _wrapMode)
        : m_width(_width)
        , m_height(_height)
        , m_doTrilinear(_doTri)
        , m_maxAnisotropy(_maxAniso)
        , m_wrapMode(_wrapMode)
    {
        initLUT();

        if (!IsPowerOf2(_width) || !IsPowerOf2(_height))
        {
            int newWidth, newHeight;
            _imgData =  resizeImage( _imgData, _width, _height, newWidth, newHeight);
            m_width = newWidth;
            m_height = newHeight;
        }    
        
        // Initialize levels of MIPMap from image
        m_nLevels = 1 + Log2Int(float(std::max(m_width, m_height)));
        m_pyramidData.resize(m_nLevels);
        //pyramid = new BlockedArray<T> *[nLevels];

        // Initialize most detailed level of MIPMap
        m_pyramidData[0] =  std::make_unique<PyramidData>(m_width, m_height, _imgData.data() );
       
        for (uint32_t i = 1; i < m_nLevels; ++i) {
            // Initialize $i$th MIPMap level from $i-1$st level
            uint32_t sRes = std::max(1u, m_pyramidData[i - 1]->uSize() / 2);
            uint32_t tRes = std::max(1u, m_pyramidData[i - 1]->vSize() / 2);

            m_pyramidData[i] = std::make_unique<PyramidData>(sRes, tRes);

            // Filter four texels from finer level of pyramid
            for (uint32_t t = 0; t < tRes; ++t)
                for (uint32_t s = 0; s < sRes; ++s)
                    (*m_pyramidData[i])(s, t) = .25f *
                    (texel(i - 1, 2 * s, 2 * t) + texel(i - 1, 2 * s + 1, 2 * t) +
                        texel(i - 1, 2 * s, 2 * t + 1) + texel(i - 1, 2 * s + 1, 2 * t + 1));
        }    
       
    }

    template<typename T>
    std::vector<ResampleWeight> MipMap<T>::resampleWeights(int _oldWidth, int _newWidth) const
    {
        assert(_newWidth >= _oldWidth);
        std::vector<ResampleWeight> wt(_newWidth);
        float filterwidth = 2.f;
        for (int i = 0; i < _newWidth; ++i) {
            // Compute image resampling weights for _i_th texel
            float center = (i + .5f) * _oldWidth / _newWidth;
            wt[i].m_firstTexel = Floor2Int((center - filterwidth) + 0.5f);
            for (int j = 0; j < 4; ++j) {
                float pos = wt[i].m_firstTexel + j + .5f;
                wt[i].m_weight[j] = Lanczos((pos - center) / filterwidth);
            }

            // Normalize filter weights for texel resampling
            float invSumWts = 1.f / (wt[i].m_weight[0] + wt[i].m_weight[1] +
                wt[i].m_weight[2] + wt[i].m_weight[3]);
            for (uint32_t j = 0; j < 4; ++j)
                wt[i].m_weight[j] *= invSumWts;
        }
        return wt;
    }


    template<typename T>
    std::vector<T> MipMap<T>::resizeImage( const std::vector<T>& _imgData, int _oldWidth, int _oldHeight, int& _newWidth, int& _newHeight)
    {
        std::vector<T> resampledImage;

        // Resample image to power-of-two resolution
        uint32_t sPow2 = RoundUpPow2(_oldWidth),
                 tPow2 = RoundUpPow2(_oldHeight);

        // Resample image in $s$ direction
        std::vector<ResampleWeight> sWeights = resampleWeights(_oldWidth, sPow2);
        resampledImage.resize(sPow2 * tPow2);

        // Apply _sWeights_ to zoom in $s$ direction
        for (uint32_t t = 0; t < (uint32_t)_oldHeight; ++t) {
            for (uint32_t s = 0; s < sPow2; ++s) {
                // Compute texel $(s,t)$ in $s$-zoomed image
                resampledImage[t * sPow2 + s] = 0.;
                for (int j = 0; j < 4; ++j) {
                    int origS = sWeights[s].m_firstTexel + j;
                    if (m_wrapMode == TEXTURE_WRAP_REPEAT)
                        origS = origS % _oldWidth; //Mod(origS, sres);
                    else if (m_wrapMode == TEXTURE_WRAP_CLAMP)
                        origS = std::clamp(origS, 0, _oldWidth - 1);
                    if (origS >= 0 && origS < (int)_oldWidth)
                        resampledImage[t * sPow2 + s] += sWeights[s].m_weight[j] * _imgData[t * _oldWidth + origS];
                }
            }
        }


        // Resample image in $t$ direction
        std::vector<ResampleWeight> tWeights = resampleWeights(_oldHeight, tPow2);
        std::vector<T> workData(tPow2);

        for (uint32_t s = 0; s < sPow2; ++s) {
            for (uint32_t t = 0; t < tPow2; ++t) {
                workData[t] = 0.;
                for (uint32_t j = 0; j < 4; ++j) {
                    int offset = tWeights[t].m_firstTexel + j;
                    if (m_wrapMode == TEXTURE_WRAP_REPEAT)
                        offset = offset % _oldHeight;
                    else if (m_wrapMode == TEXTURE_WRAP_CLAMP)
                        offset = std::clamp(offset, 0, _oldHeight - 1);
                    if (offset >= 0 && offset < (int)_oldHeight)
                        workData[t] += tWeights[t].m_weight[j] *
                        resampledImage[offset * sPow2 + s];
                }
            }
            for (uint32_t t = 0; t < tPow2; ++t)
                resampledImage[t * sPow2 + s] = clamp(workData[t]);
        }
        _newWidth = sPow2;
        _newHeight = tPow2;

       // WriteImage("MipMap.png", (float*)resampledImage.data(), _newWidth, _newHeight, _newWidth, _newHeight, 0, 0, nullptr);

        return resampledImage;
    }


    template<typename T>
    float* MipMap<T>::s_weightLut = nullptr;
   
    template<typename T>
    void MipMap<T>::initLUT()
    {
        if (s_weightLut)
            return;
        s_weightLut = AllocAligned<float>(WEIGHT_LUT_SIZE);
        for (int i = 0; i < WEIGHT_LUT_SIZE; ++i) {
            float alpha = 2;
            float r2 = float(i) / float(WEIGHT_LUT_SIZE - 1);
            s_weightLut[i] = expf(-alpha * r2) - expf(-alpha);
        }
    }

    
   
    template<typename T>
    T MipMap<T>::EWA(int level, Vector2f st, Vector2f dst0, Vector2f dst1) const
    {
        if (level >= (int)levels()) 
            return texel(levels() - 1, 0, 0);
        // Convert EWA coordinates to appropriate scale for level
        st[0] = st[0] * m_pyramidData[level]->uSize() - 0.5f;
        st[1] = st[1] * m_pyramidData[level]->vSize() - 0.5f;
        dst0[0] *= m_pyramidData[level]->uSize();
        dst0[1] *= m_pyramidData[level]->vSize();
        dst1[0] *= m_pyramidData[level]->uSize();
        dst1[1] *= m_pyramidData[level]->vSize();

        // Compute ellipse coefficients to bound EWA filter region
        float A = dst0[1] * dst0[1] + dst1[1] * dst1[1] + 1;
        float B = -2 * (dst0[0] * dst0[1] + dst1[0] * dst1[1]);
        float C = dst0[0] * dst0[0] + dst1[0] * dst1[0] + 1;
        float invF = 1 / (A * C - B * B * 0.25f);
        A *= invF;
        B *= invF;
        C *= invF;

        // Compute the ellipse's $(s,t)$ bounding box in texture space
        float det = -B * B + 4 * A * C;
        float invDet = 1 / det;
        float uSqrt = std::sqrt(det * C), vSqrt = std::sqrt(A * det);
        int s0 = std::ceil(st[0] - 2 * invDet * uSqrt);
        int s1 = std::floor(st[0] + 2 * invDet * uSqrt);
        int t0 = std::ceil(st[1] - 2 * invDet * vSqrt);
        int t1 = std::floor(st[1] + 2 * invDet * vSqrt);

        // Scan over ellipse bound and compute quadratic equation
        T sum(0.f);
        float sumWts = 0;
        for (int it = t0; it <= t1; ++it) {
            float tt = it - st[1];
            for (int is = s0; is <= s1; ++is) {
                float ss = is - st[0];
                // Compute squared radius and filter texel if inside ellipse
                float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
                if (r2 < 1) {
                    int index = std::min((int)(r2 * WEIGHT_LUT_SIZE), WEIGHT_LUT_SIZE - 1);
                    float weight = s_weightLut[index];
                    sum += texel(level, is, it) * weight;
                    sumWts += weight;
                }
            }
        }
        return sum / sumWts;
    }

    template<typename T>
    T MipMap<T>::triangle(int level, const Vector2f& st) const
    {
        level = std::clamp(level, 0, (int)levels() - 1);
        float s = st[0] * m_pyramidData[level]->uSize() - 0.5f;
        float t = st[1] * m_pyramidData[level]->vSize() - 0.5f;
        int s0 = std::floor(s), 
            t0 = std::floor(t);
        float ds = s - s0, dt = t - t0;
        return (1 - ds) * (1 - dt) * texel(level, s0, t0) +
            (1 - ds) * dt * texel(level, s0, t0 + 1) +
            ds * (1 - dt) * texel(level, s0 + 1, t0) +
            ds * dt * texel(level, s0 + 1, t0 + 1);
    }

   


    template<typename T>
    const T& MipMap<T>::texel(uint32_t level, int s, int t) const
    {
        assert(level < m_nLevels);
        const PyramidData& l = *m_pyramidData[level];
        // Compute texel $(s,t)$ accounting for boundary conditions
        switch (m_wrapMode) 
        {
            case  TEXTURE_WRAP_REPEAT:
                s = s % l.uSize(); //Mod(s, l.uSize());
                t = t % l.vSize();//Mod(t, l.vSize());
                break;
            case TEXTURE_WRAP_CLAMP:
                s = std::clamp(s, 0, (int)l.uSize() - 1);
                t = std::clamp(t, 0, (int)l.vSize() - 1);
                break;
            case TEXTURE_WRAP_BLACK: {
                static const T black = 0.f;
                if (s < 0 || s >= (int)l.uSize() ||
                    t < 0 || t >= (int)l.vSize())
                    return black;
                break;
            }
        }
        return l(s, t);
    }

    template<typename T>
    T MipMap<T>::lookup(const Vector2f& st, float _width /*= 0.0f*/)
    {
        float level = m_nLevels - 1.0f + Log2(std::max(_width, 1e-8f));
        if (level < 0)
            return triangle(0, st);
        else if (level >= m_nLevels - 1)
            return texel(m_nLevels - 1, 0, 0);
        else {
            uint32_t iLevel = Floor2Int(level);
            float delta = level - iLevel;
            return (1.f - delta) * triangle(iLevel, st) + delta * triangle(iLevel + 1, st);
        }
    }

    template<typename T>
    T MipMap<T>::lookup(const Vector2f& st,  Vector2f dst0,  Vector2f dst1)
    {
        if (m_doTrilinear) {
            float width = std::max(std::max(std::abs(dst0[0]), std::abs(dst0[1])),
                                   std::max(std::abs(dst1[0]), std::abs(dst1[1])));
            return lookup(st, width);
        }
        // Compute ellipse minor and major axes
        if (LengthSqr(dst0) < LengthSqr(dst1))
            std::swap(dst0, dst1);
        float majorLength = dst0.length();
        float minorLength = dst1.length();

        // Clamp ellipse eccentricity if too large
        if (minorLength * m_maxAnisotropy < majorLength && minorLength > 0) {
            float scale = majorLength / (minorLength * m_maxAnisotropy);
            dst1 *= scale;
            minorLength *= scale;
        }
        if (minorLength == 0)
            return triangle(0, st);

        // Choose level of detail for EWA lookup and perform EWA filtering
        float lod = std::max((float)0, levels() - (float)1 + Log2(minorLength));
        int ilod  = std::floor(lod);
        return Lerp( EWA(ilod, st, dst0, dst1), EWA(ilod + 1, st, dst0, dst1), lod - ilod);
    }

    


    //template<typename T>
    //T MipMap<T>::lookup(float _s, float _t, float _width /*= 0.f*/) const
    //{
    //    // Compute MIPMap level for trilinear filtering
    //    float level = m_nLevels - 1.0f + Log2(std::max(_width, 1e-8f));
    //    if (level < 0)
    //        return triangle(0, _s, _t);
    //    else if (level >= m_nLevels - 1)
    //        return texel(m_nLevels - 1, 0, 0);
    //    else {
    //        uint32_t iLevel = Floor2Int(level);
    //        float delta = level - iLevel;
    //        return (1.f - delta) * triangle(iLevel, _s, _t) +
    //                delta * triangle(iLevel + 1, _s, _t);
    //    }
    
    //template<typename T>
    //T MipMap<T>::lookup(const LookUpInfo& _info) const
    //{
    //     float s   = _info.m_s;  
    //     float t   = _info.m_t;  
    //     float ds0 = _info.m_ds0;
    //     float dt0 = _info.m_dt0;
    //     float ds1 = _info.m_ds1;
    //     float dt1 = _info.m_dt1;
    //    
    //    
    //    if (m_doTrilinear) {
    //       
    //        T val = lookup(s, t,
    //            2.f * std::max(std::max(fabsf(ds0), fabsf(dt0)),
    //                  std::max(fabsf(ds1), fabsf(dt1))));
    //      
    //        return val;
    //    }
    //   
    //    // Compute ellipse minor and major axes
    //    if (ds0 * ds0 + dt0 * dt0 < ds1 * ds1 + dt1 * dt1) {
    //        std::swap(ds0, ds1);
    //        std::swap(dt0, dt1);
    //    }
    //    float majorLength = sqrtf(ds0 * ds0 + dt0 * dt0);
    //    float minorLength = sqrtf(ds1 * ds1 + dt1 * dt1);

    //    // Clamp ellipse eccentricity if too large
    //    if (minorLength * m_maxAnisotropy < majorLength && minorLength > 0.f) {
    //        float scale = majorLength / (minorLength * m_maxAnisotropy);
    //        ds1 *= scale;
    //        dt1 *= scale;
    //        minorLength *= scale;
    //    }
    //    if (minorLength == 0.f) {          
    //        T val = triangle(0, s, t);          
    //        return val;
    //    }

    //    // Choose level of detail for EWA lookup and perform EWA filtering
    //    float lod = std::max(0.f, m_nLevels - 1.f + Log2(minorLength));
    //    uint32_t ilod = Floor2Int(lod);
    //    float d = lod - ilod;
    //    LookUpInfo info = { s, t, ds0, dt0, ds1, dt1 };
    //    T val = (1.f - d) * EWA(ilod, info) +
    //             d * EWA(ilod + 1, info);
    //   
    //    return val;
    //}

}


