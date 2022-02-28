
#include "MathCommon.h"
#include "Sample.h"
#include "Filter.h"
#include "IO.h"
#include "BBox.h"
#include "ParameterSet.h"
#include "Api.h"
#include "Error.h"
#include "ImageFilmSDL.h"
#include "Integrator.h"
#include "Scene.h"
#include "Film.h"




#define FILTER_TABLE_SIZE 16

namespace RayTrace
{


    static std::atomic<float>& operator+= (std::atomic<float>& atomicFloat, float increment)
    {
        float oldValue;
        float newValue;

        do
        {
            oldValue = atomicFloat.load(std::memory_order_relaxed);
            newValue = oldValue + increment;
        } while (!atomicFloat.compare_exchange_weak(oldValue, newValue,
            std::memory_order_release,
            std::memory_order_relaxed));
        return atomicFloat;
    }

    Film::Film(const Vector2i& resolution, const BBox2f& cropWindow, std::unique_ptr<Filter> filter, 
        float diagonal, const std::string& filename, float scale, float maxSampleLuminance /*= InfinityF32*/)
        : m_fullResolution(resolution)
        , m_diagonal(diagonal * .001)
        , m_filter(std::move(filter))
        , m_filename(filename)
        , m_scale(scale)
        , m_maxSampleLuminance(maxSampleLuminance) 
    {
        // Compute film image bounds
        m_croppedPixelBounds =
              BBox2i(Vector2i(std::ceil(m_fullResolution.x * cropWindow.m_min.x),
                              std::ceil(m_fullResolution.y * cropWindow.m_min.y)),
                     Vector2i(std::ceil(m_fullResolution.x * cropWindow.m_max.x),
                              std::ceil(m_fullResolution.y * cropWindow.m_max.y)));
     /*   LOG(INFO) << "Created film with full resolution " << resolution <<
            ". Crop window of " << cropWindow << " -> croppedPixelBounds " <<
            croppedPixelBounds;*/

        // Allocate film image storage
        m_pixels = std::unique_ptr<Pixel[]>(new Pixel[m_croppedPixelBounds.area()]);      

        // Precompute filter weight table
        int offset = 0;
        for (int y = 0; y < filterTableWidth; ++y) {
            for (int x = 0; x < filterTableWidth; ++x, ++offset) {
                Vector2f p;
                p.x = (x + 0.5f) * m_filter->m_radius.x / filterTableWidth;
                p.y = (y + 0.5f) * m_filter->m_radius.y / filterTableWidth;
                m_filterTable[offset] = m_filter->Evaluate(p);
            }
        }
    }

    BBox2i Film::GetSampleBounds() const
    {

        auto min = Vector2f(m_croppedPixelBounds.m_min) + Vec2fHalf - m_filter->m_radius;
        auto max = Vector2f(m_croppedPixelBounds.m_max) - Vec2fHalf + m_filter->m_radius;
      
        BBox2i bounds(Floor2Int(min), Ceil2Int(max));                     
        return bounds;
    }

    BBox2f Film::GetPhysicalExtent() const
    {
        float aspect = (float)m_fullResolution.y / (float)m_fullResolution.x;
        float x = std::sqrt(m_diagonal * m_diagonal / (1 + aspect * aspect));
        float y = aspect * x;
        return BBox2f(Vector2f(-x / 2, -y / 2),
                            Vector2f( x / 2,  y / 2));
    }

    std::unique_ptr<FilmTile> Film::GetFilmTile(const BBox2i& sampleBounds)
    {
        BBox2f floatBounds = (BBox2f)sampleBounds;
        Vector2i p0 = (Vector2i) Ceil2Int (floatBounds.m_min - Vec2fHalf - m_filter->m_radius);
        Vector2i p1 = (Vector2i) Floor2Int(floatBounds.m_max - Vec2fHalf + m_filter->m_radius) + Vector2i(1, 1);
        BBox2i  tilePixelBounds = Intersection(BBox2i(p0, p1), m_croppedPixelBounds );
        return std::unique_ptr<FilmTile>(new FilmTile( tilePixelBounds, m_filter->m_radius, m_filterTable, filterTableWidth, m_maxSampleLuminance));
    }

    void Film::MergeFilmTile(std::unique_ptr<FilmTile> tile)
    {
        const auto& bounds = tile->GetPixelBounds();
        int xMin = bounds.m_min.x;
        int xMax = bounds.m_max.x;
        int yMin = bounds.m_min.y;
        int yMax = bounds.m_max.y;
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (int y = yMin; y < yMax; ++y) {
                for (int x = xMin; x < xMax; ++x) {
                    const FilmTilePixel& tilePixel = tile->GetPixel(Vector2i(x, y));
                    Pixel& mergePixel = GetPixel(Vector2i(x, y));
                    float xyz[3];
                    tilePixel.contribSum.ToXYZ(xyz);
                    for (int i = 0; i < 3; ++i)
                        mergePixel.xyz[i] += xyz[i];

                    mergePixel.filterWeightSum += tilePixel.filterWeightSum;
                }
            }
        }
        OnTileMerged(tile.get()); 
    }

    void Film::SetImage(const Spectrum* img) const
    {
        int nPixels = m_croppedPixelBounds.area();
        for (int i = 0; i < nPixels; ++i) {
            Pixel& p = m_pixels[i];
            img[i].ToXYZ(p.xyz);
            p.filterWeightSum = 1;
            p.splatXYZ[0] = p.splatXYZ[1] = p.splatXYZ[2] = 0;
        }
    }

    void Film::AddSplat(const Vector2f& p, Spectrum v)
    {
        if (v.HasNaNs() || v.y() < 0.f || std::isinf(v.y())) {          
            return;
        }
      
        
        Vector2i pi = Floor2Int(p);
        if (!m_croppedPixelBounds.insideExclusive( pi ) ) 
            return;
        if (v.y() > m_maxSampleLuminance)
            v *= m_maxSampleLuminance / v.y();
        float xyz[3];
        v.ToXYZ(xyz);
        Pixel& pixel = GetPixel(pi);
        for (int i = 0; i < 3; ++i)
            pixel.splatXYZ[i] += xyz[i];
    }

    void Film::WriteImage(float splatScale /*= 1*/)
    {
        const auto& bounds = m_croppedPixelBounds;

        std::cout << "Splat Scale: " << splatScale << std::endl;

        std::unique_ptr<float[]> rgb(new float[3 * bounds.area()]);

        int xMin = bounds.m_min.x;
        int xMax = bounds.m_max.x;
        int yMin = bounds.m_min.y;
        int yMax = bounds.m_max.y;
        int offset = 0;
        for (int y = yMin; y < yMax; ++y) {
            for (int x = xMin; x < xMax; ++x) {
                // Convert pixel XYZ color to RGB
                Pixel& pixel = GetPixel(Vector2i(x,y));
                XYZToRGB(pixel.xyz, &rgb[3 * offset]);

                // Normalize pixel with weight sum
                float filterWeightSum = pixel.filterWeightSum;
                if (filterWeightSum != 0) {
                    float invWt = (float)1 / filterWeightSum;
                    rgb[3 * offset + 0] = std::max((float)0, rgb[3 * offset + 0] * invWt);
                    rgb[3 * offset + 1] = std::max((float)0, rgb[3 * offset + 1] * invWt);
                    rgb[3 * offset + 2] = std::max((float)0, rgb[3 * offset + 2] * invWt);
                }

                // Add splat value at pixel
                float splatRGB[3];
                float splatXYZ[3] = { pixel.splatXYZ[0], pixel.splatXYZ[1], pixel.splatXYZ[2] };
                XYZToRGB(splatXYZ, splatRGB);
                rgb[3 * offset + 0] += splatScale * splatRGB[0];
                rgb[3 * offset + 1] += splatScale * splatRGB[1];
                rgb[3 * offset + 2] += splatScale * splatRGB[2];

                // Scale pixel value by _scale_
                rgb[3 * offset + 0] *= m_scale;
                rgb[3 * offset + 1] *= m_scale;
                rgb[3 * offset + 2] *= m_scale;
               // std::cout << rgb[3 * offset + 0] << " " << rgb[3 * offset + 1] << " " << rgb[3 * offset + 2] << "\n";
                ++offset;
            }
        }

        FloatVector pixels(&rgb[0], &rgb[0] + (m_fullResolution.x * m_fullResolution.y * 3));

        RayTrace::WriteImage(m_filename, pixels.data(), m_fullResolution.x, m_fullResolution.y,
                           m_fullResolution.x, m_fullResolution.y, 0, 0, nullptr );


      //  ::WriteImage( m_filename, &rgb[0], croppedPixelBounds, fullResolution);
    }

    void Film::Clear()
    {
        int xMin = m_croppedPixelBounds.m_min.x;
        int xMax = m_croppedPixelBounds.m_max.x;
        int yMin = m_croppedPixelBounds.m_min.y;
        int yMax = m_croppedPixelBounds.m_max.y;

        for (int y = yMin; y < yMax; ++y){
            for (int x = xMin; x < xMax; ++x){
                Pixel& pixel = GetPixel(Vector2i(x,y));
                for (int c = 0; c < 3; ++c)
                    pixel.splatXYZ[c] = pixel.xyz[c] = 0;
                pixel.filterWeightSum = 0;
            }
        }       
    }

    

    void Film::InstallPixelCallback(SetPixelCallBack _cb)
    {
        m_callback = _cb;
    }

    void Film::OnTileMerged(const FilmTile* _tile)
    {
        //nop
    }

    Film::Pixel& Film::GetPixel(const Vector2i& p)
    {
        assert(m_croppedPixelBounds.insideExclusive(p));
        int width = m_croppedPixelBounds.m_max.x - m_croppedPixelBounds.m_min.x;
        int offset = (p.x - m_croppedPixelBounds.m_min.x) +
            (p.y - m_croppedPixelBounds.m_min.y) * width;
        return m_pixels[offset];
    }
    //////////////////////////////////////////////////////////////////////////
    //FilmTile
    //////////////////////////////////////////////////////////////////////////
    FilmTile::FilmTile(const BBox2i& pixelBounds, const Vector2f& filterRadius, 
        const float* filterTable, int filterTableSize, float maxSampleLuminance) 
        : m_pixelBounds(pixelBounds)
        , m_filterRadius(filterRadius)
        , m_invFilterRadius(Inverted(filterRadius))
        , m_filterTable(filterTable)
        , m_filterTableSize(filterTableSize)
        , m_maxSampleLuminance(maxSampleLuminance)
    {
        m_pixels = std::vector<FilmTilePixel>(std::max(0, pixelBounds.area()));
    }

    void FilmTile::AddSample(const Vector2f& pFilm, Spectrum L, float sampleWeight /*= 1.*/)
    {

        if (L.y() > m_maxSampleLuminance)
            L *= m_maxSampleLuminance / L.y();
        // Compute sample's raster bounds
        Vector2f pFilmDiscrete = pFilm - Vector2f(0.5f, 0.5f);
        Vector2i p0 = (Vector2i)Ceil2Int(pFilmDiscrete - m_filterRadius);
        Vector2i p1 = (Vector2i)Floor2Int(pFilmDiscrete + m_filterRadius) + Vector2i(1, 1);
        p0 = Max(p0, m_pixelBounds.m_min); //p0.maxVal(m_pixelBounds.m_min);
        p1 = Min(p1, m_pixelBounds.m_max);//p1.minVal(m_pixelBounds.m_max);

        // Loop over filter support and add sample to pixel arrays

        // Precompute $x$ and $y$ filter table offsets
        int* ifx = ALLOCA(int, p1.x - p0.x);
        for (int x = p0.x; x < p1.x; ++x) {
            float fx = std::abs((x - pFilmDiscrete.x) * m_invFilterRadius.x * m_filterTableSize);
            ifx[x - p0.x] = std::min((int)std::floor(fx), m_filterTableSize - 1);
        }
        int* ify = ALLOCA(int, p1.y - p0.y);
        for (int y = p0.y; y < p1.y; ++y) {
            float fy = std::abs((y - pFilmDiscrete.y) * m_invFilterRadius.y * m_filterTableSize);
            ify[y - p0.y] = std::min((int)std::floor(fy), m_filterTableSize - 1);
        }
        for (int y = p0.y; y < p1.y; ++y) {
            for (int x = p0.x; x < p1.x; ++x) {
                // Evaluate filter value at $(x,y)$ pixel
                int offset = ify[y - p0.y] * m_filterTableSize + ifx[x - p0.x];
                float filterWeight = m_filterTable[offset];

                // Update pixel values with filtered sample contribution
                FilmTilePixel& pixel = GetPixel(Vector2i(x, y));
                pixel.contribSum += L * sampleWeight * filterWeight;
                pixel.filterWeightSum += filterWeight;
            }
        }
    }

    const FilmTilePixel& FilmTile::GetPixel(const Vector2i& p) const
    {
        //CHECK(InsideExclusive(p, pixelBounds));
        assert(m_pixelBounds.inside(p));
        int width = m_pixelBounds.m_max.x - m_pixelBounds.m_min.x;
        int offset =
            (p.x - m_pixelBounds.m_max.x) + (p.y - m_pixelBounds.m_min.y) * width;
        return m_pixels[offset];
    }

    FilmTilePixel& FilmTile::GetPixel(const Vector2i& p)
    {
      /*  int width = pixelBounds.pMax.x - pixelBounds.pMin.x;
        int offset =
            (p.x - pixelBounds.pMin.x) + (p.y - pixelBounds.pMin.y) * width;
     */   
        
        assert(m_pixelBounds.inside(p));
        int width = m_pixelBounds.m_max.x - m_pixelBounds.m_min.x;
        int offset =
            (p.x - m_pixelBounds.m_min.x) + (p.y - m_pixelBounds.m_min.y) * width;
        return m_pixels[offset];
    }
   
    BBox2i FilmTile::GetPixelBounds() const
    {
        return m_pixelBounds;
    }

    Film* CreateFilm(const ParamSet& params, std::unique_ptr<Filter> filter)
    {
        std::string filename;
        if (PbrtOptions.imageFile != "") {
            filename = PbrtOptions.imageFile;
            std::string paramsFilename = params.FindOneString("filename", "");
            if (paramsFilename != "") {
            }
                /* Warning(
                     "Output filename supplied on command line, \"%s\" is overriding "
                     "filename provided in scene description file, \"%s\".",
                     PbrtOptions.imageFile.c_str(), paramsFilename.c_str());*/
        }
        else
            filename = params.FindOneString("filename", "pbrt.tga");

        int xres = params.FindOneInt("xresolution", 1280);
        int yres = params.FindOneInt("yresolution", 720);
        if (PbrtOptions.quickRender) xres = std::max(1, xres / 4);
        if (PbrtOptions.quickRender) yres = std::max(1, yres / 4);
        BBox2f crop;
        int cwi;
        const float* cr = params.FindFloat("cropwindow", &cwi);
        if (cr && cwi == 4) {
            crop.m_min.x = std::clamp(std::min(cr[0], cr[1]), 0.f, 1.f);
            crop.m_max.x = std::clamp(std::max(cr[0], cr[1]), 0.f, 1.f);
            crop.m_min.y = std::clamp(std::min(cr[2], cr[3]), 0.f, 1.f);
            crop.m_max.y = std::clamp(std::max(cr[2], cr[3]), 0.f, 1.f);
        }
        else if (cr)
            Error("%d values supplied for \"cropwindow\". Expected 4.", cwi);
        else
            crop = BBox2f(Vector2f(std::clamp(PbrtOptions.cropWindow[0][0], 0.f, 1.f),
                                   std::clamp(PbrtOptions.cropWindow[1][0], 0.f, 1.f)),
                          Vector2f(std::clamp(PbrtOptions.cropWindow[0][1], 0.f, 1.f),
                                   std::clamp(PbrtOptions.cropWindow[1][1], 0.f, 1.f)));

        float scale    = params.FindOneFloat("scale", 1.);
        float diagonal = params.FindOneFloat("diagonal", 35.);
        float maxSampleLuminance = params.FindOneFloat("maxsampleluminance", InfinityF32);
        return new Film( Vector2i(xres, yres), crop, std::move(filter), diagonal, filename, scale, maxSampleLuminance);
    }

    Film* CreateFilmSDL(const ParamSet& params, std::unique_ptr<Filter> filter)
    {
        //std::string filename;
        //if (PbrtOptions.imageFile != "") {
        //    filename = PbrtOptions.imageFile;
        //    std::string paramsFilename = params.FindOneString("filename", "");
        //    if (paramsFilename != "") {
        //    }
        //    /* Warning(
        //         "Output filename supplied on command line, \"%s\" is overriding "
        //         "filename provided in scene description file, \"%s\".",
        //         PbrtOptions.imageFile.c_str(), paramsFilename.c_str());*/
        //}
        //else
        //    filename = params.FindOneString("filename", "pbrt.tga");

        //int xres = params.FindOneInt("xresolution", 1280);
        //int yres = params.FindOneInt("yresolution", 720);
        //if (PbrtOptions.quickRender) xres = std::max(1, xres / 4);
        //if (PbrtOptions.quickRender) yres = std::max(1, yres / 4);
        //BBox2f crop;
        //int cwi;
        //const float* cr = params.FindFloat("cropwindow", &cwi);
        //if (cr && cwi == 4) {
        //    crop.m_min.x = std::clamp(std::min(cr[0], cr[1]), 0.f, 1.f);
        //    crop.m_max.x = std::clamp(std::max(cr[0], cr[1]), 0.f, 1.f);
        //    crop.m_min.y = std::clamp(std::min(cr[2], cr[3]), 0.f, 1.f);
        //    crop.m_max.y = std::clamp(std::max(cr[2], cr[3]), 0.f, 1.f);
        //}
        //else if (cr)
        //    Error("%d values supplied for \"cropwindow\". Expected 4.", cwi);
        //else
        //    crop = BBox2f(Vector2f(std::clamp(PbrtOptions.cropWindow[0][0], 0.f, 1.f),
        //        std::clamp(PbrtOptions.cropWindow[1][0], 0.f, 1.f)),
        //        Vector2f(std::clamp(PbrtOptions.cropWindow[0][1], 0.f, 1.f),
        //            std::clamp(PbrtOptions.cropWindow[1][1], 0.f, 1.f)));

        //float scale = params.FindOneFloat("scale", 1.);
        //float diagonal = params.FindOneFloat("diagonal", 35.);
        //float maxSampleLuminance = params.FindOneFloat("maxsampleluminance", InfinityF32);
        //return new ImageFilmSDL(Vector2i(xres, yres), crop, std::move(filter), diagonal, filename, scale, maxSampleLuminance);
        return nullptr;
    }

}

