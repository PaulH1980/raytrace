#pragma once
#include "Defines.h"
#include "Film.h"
#include "SDL.h"


namespace RayTrace
{
    class ImageFilmSDL : public Film
    {
    public:

        //Vector2i(xres, yres), crop, std::move(filter), diagonal, filename, scale, maxSampleLuminance

        ImageFilmSDL(const Vector2i& _resolution, const BBox2f& _crop,
            FilterUPtr _filt, SDL_Renderer* _pRenderer, const std::string& _fileName)
            :Film(_resolution, _crop, std::move(_filt), 35.f, _fileName, 1.f)
            , m_pRenderer(_pRenderer)
        {

        }

        virtual void  OnTileMerged(const FilmTile* _tile) override
        {
            auto toU8 = [](float _val) {
                return uint8_t(std::clamp(255.f * std::powf((_val), 1.f / 2.2f), 0.f, 255.f));
            };
            auto bounds = _tile->GetPixelBounds();
            int x0 = bounds.m_min[0];
            int x1 = bounds.m_max[0];
            int y0 = bounds.m_min[1];
            int y1 = bounds.m_max[1];
            if ((x1 - x0) < 0 || (y1 - y0) < 0) {
                return;
            }

            // Convert image to RGB and compute final pixel values
            int nPix = (x0 - x1) * (y0 - y1);
            int offset = 0;
            std::lock_guard<std::mutex> lock(m_mutex);
            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    // Convert pixel XYZ color to RGB
                    int idx0 = 3 * offset;
                    int idx1 = idx0 + 1;
                    int idx2 = idx0 + 2;
                    float rgb[3] = { 0.f };

                    const auto& pix = GetPixel({ x, y });

                    Pixel& pixel = GetPixel(Vector2i(x, y));
                    XYZToRGB(pixel.xyz, rgb);

                    // Normalize pixel with weight sum
                    float filterWeightSum = pixel.filterWeightSum;
                    if (filterWeightSum != 0) {
                        float invWt = (float)1 / filterWeightSum;
                        rgb[0] = std::max((float)0, rgb[0] * invWt);
                        rgb[1] = std::max((float)0, rgb[1] * invWt);
                        rgb[2] = std::max((float)0, rgb[2] * invWt);
                    }

                    // Add splat value at pixel
                    float splatRGB[3];
                    float splatXYZ[3] = { pixel.splatXYZ[0], pixel.splatXYZ[1], pixel.splatXYZ[2] };
                    XYZToRGB(splatXYZ, splatRGB);
                    rgb[0] += splatRGB[0];
                    rgb[1] += splatRGB[1];
                    rgb[2] += splatRGB[2];

                    if (m_pRenderer) {
                        SDL_SetRenderDrawColor(m_pRenderer, toU8(rgb[0]), toU8(rgb[1]), toU8(rgb[2]), 255);
                        SDL_RenderDrawPoint(m_pRenderer, x, y);
                    }
                }
            }
            if (m_pRenderer) {
                SDL_RenderPresent(m_pRenderer);
            }
        }


        void WriteImage(float splatScale /* = 1 */) override
        {
            Film::WriteImage(splatScale);
        }

        std::mutex    m_mutex;
        SDL_Renderer* m_pRenderer = nullptr;        
    };
}