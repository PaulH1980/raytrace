#pragma once
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "Defines.h"
#include "MathCommon.h"
#include "BBox.h"
#include "Memory.h"
#include "Spectrum.h"

namespace RayTrace
{
    inline Vector2i Ceil2Int(const Vector2f& _v)
    {
        return Vector2i( std::ceil(_v[0]), std::ceil(_v[1]));
    }
    inline Vector2i Floor2Int(const Vector2f& _v)
    {
        return Vector2i(std::floor(_v[0]), std::floor(_v[1]));
    }

    // FilmTilePixel Declarations
    struct FilmTilePixel {
        Spectrum contribSum   = 0.f;
        float filterWeightSum = 0.f;
    };

    using SetPixelCallBack = std::function<void(int _x, int _y, uint8_t _r, uint8_t _g, uint8_t _b)>;
    using ResizeCallback   = std::function<void(int _x, int _y)>;

    // Film Declarations
    class Film {
    public:
        // Film Public Methods
        Film(const Vector2i& resolution, const BBox2f& cropWindow,
            std::unique_ptr<Filter> filter, float diagonal,
            const std::string& filename, float scale,
            float maxSampleLuminance = InfinityF32);

        virtual void                WriteImage(float splatScale);


        BBox2i                GetSampleBounds() const;
        BBox2f                GetPhysicalExtent() const;
        std::unique_ptr<FilmTile>   GetFilmTile(const BBox2i& sampleBounds);
        void                        MergeFilmTile(std::unique_ptr<FilmTile> tile);
        void                        SetImage(const Spectrum* img) const;
        void                        AddSplat(const Vector2f& p, Spectrum v);
        
        void                        Clear();
        
        void                        InstallPixelCallback(SetPixelCallBack _cb);

        // Film Public Data
        const Vector2i    m_fullResolution;
        const float             m_diagonal;
        std::unique_ptr<Filter> m_filter;
        const std::string       m_filename;
        BBox2i            m_croppedPixelBounds;

        // Film Private Data
        struct Pixel {
            Pixel() { xyz[0] = xyz[1] = xyz[2] = filterWeightSum = 0; }
            float               xyz[3];
            float               filterWeightSum;
            std::atomic<float>  splatXYZ[3];
            float               pad;
        };

        // Film Private Methods
        Pixel& GetPixel(const Vector2i& p);

    protected:
        mutable SetPixelCallBack    m_callback;

    private:
       
        std::unique_ptr<Pixel[]> m_pixels;
        static constexpr int filterTableWidth = 16;
        float       m_filterTable[filterTableWidth * filterTableWidth];
        std::mutex  m_mutex;
        const float m_scale;
        const float m_maxSampleLuminance;

       

      
        virtual void  OnTileMerged(const FilmTile* _tile);
    };

    class FilmTile {
    public:
        // FilmTile Public Methods
        FilmTile(const BBox2i& pixelBounds, const Vector2f& filterRadius,
            const float* filterTable, int filterTableSize,
            float maxSampleLuminance);

        void                    AddSample(const Vector2f& pFilm, Spectrum L, float sampleWeight = 1.);
        FilmTilePixel&          GetPixel(const Vector2i& p);
        const FilmTilePixel&    GetPixel(const Vector2i& p) const;
        BBox2i            GetPixelBounds() const;

    private:
        // FilmTile Private Data
        const BBox2i          m_pixelBounds;
        const Vector2f        m_filterRadius, 
                                    m_invFilterRadius;
        const float*                m_filterTable;
        const int                   m_filterTableSize;
        std::vector<FilmTilePixel>  m_pixels;
        const float                 m_maxSampleLuminance;
        friend class Film;
    };

    Film* CreateFilm(const ParamSet& params, std::unique_ptr<Filter> filter);
    Film* CreateFilmSDL(const ParamSet& params, std::unique_ptr<Filter> filter);

}