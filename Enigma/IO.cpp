#include <exception>
#include <cctype>
#include <stdio.h>
#include <fstream>
#include <iostream>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "LodePng.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "targa.h"
#include "spectrum.h"
#include "ext/openexr/OpenEXR/IlmImf/ImfRgba.h"
#include "Error.h"
#include "ext/openexr/OpenEXR/IlmImf/ImfRgbaFile.h"
#include "FilePaths.h"
#include "Common.h"
#include "BBox.h"
#include "IO.h"




#define BUFFER_SIZE 80


namespace RayTrace
{
    //
    //template<typename PixType>
    //std::vector<uint8_t> GetPixelData(const ImageExInfo& _data, const ChannelMask& _mask)
    //{
    //    std::vector<Vector4f> m_rgba;        
    //    
    //    const auto exInfo       = GetFormatInfo(_data.m_internalFormat);
    //    const uint32_t pixCount = _data.m_width * _data.m_height;
    //    const auto* dataPtr     = (const PixType*) _data.m_rawData.data();
    //}


    bool CombineChannels(eInternalFormat _format, int _width, int _height, ImageExInfo& _data,   U8Vector* _r, U8Vector* _g, U8Vector* _b, U8Vector* _a /*= nullptr*/)
    {
        const auto exInfo = GetFormatInfo(_format);
        const uint32_t pixCount = _width * _height;
        uint32_t mask = ChannelMask::CHANNEL_UNDEFINED;;
        if (_r) 
            mask |= CHANNEL_RED;
        if (_g) 
            mask |= CHANNEL_GREEN;
        if (_b)
            mask |= CHANNEL_BLUE;
        if (_a)
            mask |= CHANNEL_ALPHA;
       

        _data.m_channels    = (ChannelMask)mask;
        _data.m_numChannels = ChannelCount((ChannelMask)mask);
        _data.m_rawData.resize(exInfo.GetNumBytes() * pixCount);
        _data.m_type        = exInfo.m_type;
        _data.m_internalFormat = _format;
        _data.m_width = _width;
        _data.m_height = _height;
        _data.m_depth = 1;

        uint32_t channelBytes[4] = {
           exInfo.m_redBits >> 3,
           exInfo.m_greenBits >> 3,
           exInfo.m_blueBits >> 3,
           exInfo.m_alphaBits >> 3
        };

        const uint8_t* outputs[4] = {
           _r != nullptr ? _r->data() : nullptr, 
           _g != nullptr ? _g->data() : nullptr,
           _b != nullptr ? _b->data() : nullptr,
           _a != nullptr ? _a->data() : nullptr
        };
        
        auto* src = _data.m_rawData.data();
        for (int pix = 0; pix < (int)pixCount; pix++)
        {
            for (int ch = 0; ch < (int)_data.m_numChannels; ++ch)
            {
                for (int bytes = 0; bytes < (int)channelBytes[ch]; bytes++)
                {
                    if (outputs[ch])
                    {
                        *src = *outputs[ch]; 
                        outputs[ch]++;
                    }
                    src++;
                }
            }
        }
        return true;
    }

    bool SplitChannels(const ImageExInfo& _data, U8Vector* _r, U8Vector* _g, U8Vector* _b, U8Vector* _a )
    {
        const auto exInfo = GetFormatInfo(_data.m_internalFormat);
        const uint32_t pixCount = _data.m_width * _data.m_height;
      

        uint32_t channelBytes[4] = {
            exInfo.m_redBits >> 3,
            exInfo.m_greenBits >> 3,
            exInfo.m_blueBits >> 3,
            exInfo.m_alphaBits >> 3
        };

        U8Vector* outputs[4] = {
            _r, _g ,_b, _a
        };

        const auto* src = _data.m_rawData.data();
        for (int pix = 0; pix < (int)pixCount; pix++)
        {
            for (int ch = 0; ch < 4; ++ch)
            {
                for (int bytes = 0; bytes < (int)channelBytes[ch]; bytes++) {
                    if (outputs[ch]) {
                        outputs[ch]->push_back(*src);
                    }
                    src++;
                }
            }
        }    
        return true;
    }


    bool ReadImage(const std::string& _fileName, ImageExInfo& _imageData)
    {
        bool succeed = false;
        if (!Exists(_fileName))
            return false;

        if (_fileName.size() >= 5) {

            auto extension = ToLower(GetExtension(_fileName));
            if (extension == ".jpg" || extension == ".jpeg")
            {
                return ReadImageJPG(_fileName, _imageData);
            }
            else if (extension == ".tga")
            {
                return ReadImageTGA(_fileName, _imageData);
            }
            else if (extension == ".png")
            {
                return ReadImagePNG(_fileName, _imageData);
            }
            else if (extension == ".exr")
            {
                return ReadImageEXR(_fileName, _imageData);
            }
            else if (extension == ".hdr")
            {
                return ReadImageHDR(_fileName, _imageData);
            }
            else if (extension == ".dds") {
                return false;
            }
        }
        return false;
    }


    bool WriteImage(const std::string& _fileName, const ImageExInfo& _imageData)
    {
        if (_fileName.size() >= 5) {

            auto extension = ToLower(GetExtension(_fileName));
            if (extension == ".jpg" || extension == ".jpeg")
            {
                //return ReadImageJPG(_fileName, _imageData);
            }
            else if (extension == ".tga")
            {
                return WriteImageTGA(_fileName, _imageData);
            }
            else if (extension == ".png")
            {
                return WriteImagePNG(_fileName, _imageData);
            }
            else if (extension == ".exr")
            {
                //return WriteImageEXR(_fileName, _imageData);
            }
            else if (extension == ".hdr")
            {
                return WriteImageHDR(_fileName, _imageData);
            }
            else if (extension == ".dds") {
                return false;
            }
        }
        return false;
    }

    bool ResizeImage(const ImageExInfo& _in, ImageExInfo& _out)
    {       
        
        int width  = _out.m_width;
        int height = _out.m_height;
        _out = _in;
        _out.m_width  = width;
        _out.m_height = height;
        if (IsFloatingPoint(_in.m_type))
        {
            if (IsHalfFloat(_in.m_type))
            {
                std::vector<float> tmpIn;
                std::vector<float> tmpOut;

                const short* u16buf = (const short*)(_in.m_rawData.data());                
                const int numPixelsIn = _in.m_width * _in.m_height * _in.m_numChannels;
                const int numPixelsOut = _out.m_width * _out.m_height * _out.m_numChannels;               

                for (int i = 0; i < numPixelsIn; ++i)
                    tmpIn.push_back(glm::detail::toFloat32(u16buf[i]));                

                tmpOut.resize(numPixelsOut);

                stbir_resize_float(
                    (float*)tmpIn.data(), _in.m_width, _in.m_height, 0,
                    (float*)tmpOut.data(), _out.m_width, _out.m_height, 0,
                    _in.m_numChannels);
                _out.m_rawData.resize(_out.m_width * _out.m_height * _in.m_numChannels * sizeof(short));
                
                short* u16BufOut = (short*)_out.m_rawData.data();
                for (int i = 0; i < numPixelsOut; i++)
                   u16BufOut[i] = glm::detail::toFloat16(tmpOut[i]);  
                
                return true;
            }
            else
            {
                _out.m_rawData.resize(_out.m_width * _out.m_height * _in.m_numChannels * sizeof(float));
                stbir_resize_float(
                    (float*)_in.m_rawData.data(), _in.m_width, _in.m_height, 0,
                    (float*)_out.m_rawData.data(), _out.m_width, _out.m_height, 0,
                    _in.m_numChannels);
                return true;
            }    
            
        }       
        else if (IsByte(_in.m_type))
        {
            _out.m_rawData.resize(_out.m_width * _out.m_height * _in.m_numChannels);            
            stbir_resize_uint8(
                _in.m_rawData.data(),_in.m_width,_in.m_height,0,
                _out.m_rawData.data(), _out.m_width, _out.m_height,0,
                _in.m_numChannels);  
            
            return true;
        }

        return false;
    }

    void SwapImage(ImageExInfo& _data, bool _swapRows /*= true*/)
    {
        if (_swapRows) {
            const auto rowStride = _data.m_width * GetFormatInfo(_data.m_internalFormat).GetNumBytes();
            for (int i = 0; i <(int) _data.m_height / 2; ++i)
            {
                int j = (_data.m_height - 1) - i;

                int offsetTop = i * rowStride;
                int offsetBot = j * rowStride;
                uint8_t* topPtr = &_data.m_rawData[offsetTop];
                uint8_t* botPtr = &_data.m_rawData[offsetBot];
                for (int w = 0; w < (int)rowStride; w++)
                {
                    std::swap(*topPtr, *botPtr);
                    topPtr++;
                    botPtr++;
                }         
            }
        }
    }

    std::vector<Color4f> ConvertToLegacy(const ImageExInfo& _imageData)
    {
        return {};
    }



    bool ReadImageJPG(const std::string& _name, ImageExInfo& _info)
    {
        stbi_set_flip_vertically_on_load(_info.m_readFlipped);
        auto pData = stbi_load(_name.c_str(), (int*)&_info.m_width, (int*)&_info.m_height, (int*)&_info.m_numChannels, 0);
        if (!pData)
            return false;
        _info.m_rawData.resize(_info.m_width * _info.m_height * _info.m_numChannels);
        memcpy(_info.m_rawData.data(), pData, _info.m_rawData.size());
        _info.m_type = eTextureType::TYPE_UNSIGNED_BYTE;
        _info.m_channels = ChannelCountToMask(_info.m_numChannels);
        _info.m_internalFormat = eInternalFormat::INTERNAL_RGB8;

        free(pData);
        return true;
    }

    bool ReadImagePNG(const std::string& _name, ImageExInfo& _info)
    {
        stbi_set_flip_vertically_on_load(_info.m_readFlipped );
        
        auto pData = stbi_load(_name.c_str(), (int*)&_info.m_width, (int*)&_info.m_height, (int*)&_info.m_numChannels, 0);
        if (!pData)
            return false;
        _info.m_rawData.resize(_info.m_width * _info.m_height * _info.m_numChannels);
        memcpy(_info.m_rawData.data(), pData, _info.m_rawData.size());
        _info.m_type = eTextureType::TYPE_UNSIGNED_BYTE;
        _info.m_channels = ChannelCountToMask(_info.m_numChannels);
        if (_info.m_numChannels == 1)
            _info.m_internalFormat = eInternalFormat::INTERNAL_R8;
        else if (_info.m_numChannels == 2)
            _info.m_internalFormat = eInternalFormat::INTERNAL_RG8;
        else if (_info.m_numChannels == 3)
            _info.m_internalFormat = eInternalFormat::INTERNAL_RGB8;
        else if (_info.m_numChannels == 4)
            _info.m_internalFormat = eInternalFormat::INTERNAL_RGBA8;

        free(pData);
        return true;
    
    }

    bool ReadImageHDR(const std::string& _name, ImageExInfo& _info)
    {
        stbi_set_flip_vertically_on_load(_info.m_readFlipped);
        float* pData = stbi_loadf(_name.c_str(), (int*)&_info.m_width, (int*)&_info.m_height, (int*)&_info.m_numChannels, 0);
        if (!pData)
            return false;
        if (_info.m_readConvertToF16F)
        {
            _info.m_type = eTextureType::TYPE_HALF_FLOAT;
            _info.m_channels = ChannelCountToMask(_info.m_numChannels);
            if (_info.m_numChannels == 1)
                _info.m_internalFormat = eInternalFormat::INTERNAL_R16F;
            else if (_info.m_numChannels == 2)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RG16F;
            else if (_info.m_numChannels == 3)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RGB16F;
            else if (_info.m_numChannels == 4)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RGBA16F;

            _info.m_rawData.resize(_info.m_width * _info.m_height * _info.m_numChannels * sizeof(short));
            short* shortBuf = (short*)_info.m_rawData.data();

            for (int i = 0; i < static_cast<int>(_info.m_width * _info.m_height * _info.m_numChannels); ++i)
            {
                shortBuf[i] = glm::detail::toFloat16(pData[i]);
            }
        }
        else
        {
            _info.m_type = eTextureType::TYPE_FLOAT;
            _info.m_channels = ChannelCountToMask(_info.m_numChannels);
            if (_info.m_numChannels == 1)
                _info.m_internalFormat = eInternalFormat::INTERNAL_R32F;
            else if (_info.m_numChannels == 2)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RG32F;
            else if (_info.m_numChannels == 3)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RGB32F;
            else if (_info.m_numChannels == 4)
                _info.m_internalFormat = eInternalFormat::INTERNAL_RGBA32F;

            _info.m_rawData.resize(_info.m_width * _info.m_height * _info.m_numChannels * sizeof(float));
            memcpy(_info.m_rawData.data(), pData, _info.m_rawData.size());           
        }

       
        free(pData);
        return true;
    }

    bool ReadImageEXR(const std::string& _name, ImageExInfo& _info)
    {
        using namespace Imf;
        using namespace Imath;
        try {
            RgbaInputFile file(_name.c_str());
            Box2i dw = file.dataWindow();

            _info.m_width = dw.max.x - dw.min.x + 1;
            _info.m_height = dw.max.y - dw.min.y + 1;
            _info.m_channels = CHANNEL_RGBA;
            _info.m_internalFormat = eInternalFormat::INTERNAL_RGBA16;
            _info.m_type = eTextureType::TYPE_HALF_FLOAT;
            _info.m_numChannels = 4;
            int dims = _info.m_width * _info.m_height;

            std::vector<Rgba> pixels(_info.m_width);
            file.setFrameBuffer(&pixels[0] - dw.min.x - dw.min.y * _info.m_width, 1, _info.m_width);
            file.readPixels(dw.min.y, dw.max.y);
            _info.m_rawData.resize(_info.m_width * _info.m_height * 4 * sizeof(short));
            short* shortBuf = (short*)_info.m_rawData.data();
            // std::vector<Color4f>  ret(dims);
            for (int i = 0; i < dims; ++i) {
                int idx = i * 4;
                shortBuf[idx + 0] = glm::detail::toFloat16(pixels[i].r);
                shortBuf[idx + 1] = glm::detail::toFloat16(pixels[i].g);
                shortBuf[idx + 2] = glm::detail::toFloat16(pixels[i].b);
                shortBuf[idx + 3] = glm::detail::toFloat16(pixels[i].a);
            }

            return true;

        }
        catch (const std::exception&) {
            //Error("Unable to read image file \"%s\": %s", name.c_str(), e.what());
        }
        return false;
    }

    bool ReadImageTGA(const std::string& _name, ImageExInfo& _info)
    {
        tga_image img;
        tga_result result;
        if ((result = tga_read(&img, _name.c_str())) != TGA_NOERR) {

            return false;
        }

        if (tga_is_right_to_left(&img)) tga_flip_horiz(&img);
        if (!tga_is_top_to_bottom(&img)) tga_flip_vert(&img);
        if (tga_is_colormapped(&img)) tga_color_unmap(&img);

        if (tga_is_mono(&img)) {
            _info.m_numChannels = 1;
            _info.m_channels = CHANNEL_RED;
            _info.m_internalFormat = eInternalFormat::INTERNAL_R8;
        }
        else if (img.pixel_depth == 24) {
            _info.m_numChannels = 3;
            _info.m_channels = CHANNEL_RGB;
            _info.m_internalFormat = eInternalFormat::INTERNAL_RGB8;
        }
        else if (img.pixel_depth == 32) {
            _info.m_numChannels = 4;
            _info.m_channels = CHANNEL_RGBA;
            _info.m_internalFormat = eInternalFormat::INTERNAL_RGBA8;
        }

        _info.m_width = img.width;
        _info.m_height = img.height;
        _info.m_type = eTextureType::TYPE_UNSIGNED_BYTE;

        _info.m_rawData.reserve(_info.m_height * _info.m_width * (img.pixel_depth / 8));

        for (int y = 0; y <(int) _info.m_height; y++) {
            for (int x = 0; x < (int)_info.m_width; x++)
            {
                uint8_t* src = tga_find_pixel(&img, x, y);

                int idx = y * _info.m_width + x;
                for (int ch = 0; ch < (img.pixel_depth / 8); ch++) {
                    _info.m_rawData.push_back(*src);
                    src++;
                }
            }
        }
        return true;
    }


    bool WriteImagePNG(const std::string& _name, const ImageExInfo& _info)
    {
        auto bpp = GetFormatInfo(_info.m_internalFormat).GetNumBytes();
        stbi_flip_vertically_on_write(_info.m_writeFlipped);
     


        return 0 != stbi_write_png(_name.c_str(), _info.m_width, _info.m_height, _info.m_numChannels, _info.m_rawData.data(), _info.m_width * bpp);
    }

    bool WriteImageHDR(const std::string& _name, const ImageExInfo& _info)
    {
        stbi_flip_vertically_on_write(_info.m_writeFlipped);

        return 0 != stbi_write_hdr(_name.c_str(), _info.m_width, _info.m_height, _info.m_numChannels, (const float*)_info.m_rawData.data());
    }

    bool WriteImageTGA(const std::string& _name, const ImageExInfo& _info)
    {
        stbi_flip_vertically_on_write(_info.m_writeFlipped);
        return 0 != stbi_write_tga(_name.c_str(), _info.m_width, _info.m_height, _info.m_numChannels, _info.m_rawData.data());
    }

 

#pragma region LEGACY

    static bool hostLittleEndian = true;


    // ImageIO Local Declarations
    static void         WriteImageTGA(const std::string& name, const float* pixels,
        const float* alpha, int xRes, int yRes,
        int totalXRes, int totalYRes,
        int xOffset, int yOffset);
    static bool         WriteImagePFM(const std::string& filename, const float* rgb, int xres, int yres);
    static void         WriteImageEXR(const std::string &name, const float *pixels,
                          int xRes, int yRes, int totalXRes, int totalYRes,
                          int xOffset, int yOffset);

    static std::vector<Color4f>  ReadImageJPG(const std::string& name, int* width, int* height);
    static std::vector<Color4f>  ReadImageTGA(const std::string& name, int* w, int* h, int* _numChannels);
    static std::vector<Color4f>  ReadImagePFM(const std::string& filename, int* xres, int* yres);
    static std::vector<Color4f>  ReadImagePNG(const std::string& name, int* w, int* h, int* _numChannels);
    static std::vector<Color4f>  ReadImageEXR(const std::string& name, int* w, int* h, BBox2i* dataWindow = nullptr, BBox2i* displayWindow = nullptr);
    static std::vector<Color4f>  ReadImageHDR(const std::string& name, int* w, int* h, int* _numChannels);

    // ImageIO Function Definitions
    std::vector<Color4f>  ReadImage(const std::string& name, int* width, int* height, int& _numChannels, eTextureType& _type) {

       if (name.size() >= 5) {
            _numChannels = 3;
            _type = eTextureType::TYPE_UNSIGNED_BYTE;

            uint32_t suffixOffset = name.size() - 4;
            auto extension = ToLower(GetExtension(name));
            if (extension == ".jpg" ) {
                return  ReadImageJPG(name, width, height);
            }
            else if (extension == ".tga") {
                return  ReadImageTGA(name, width, height, &_numChannels);
            }
            else if (extension == ".png") {
                return  ReadImagePNG(name, width, height, &_numChannels);
            }
            else if (extension == ".exr") {
                
                _type = eTextureType::TYPE_HALF_FLOAT;
                return ReadImageEXR(name, width, height);
            }
            else if (extension == ".hdr")
            {
                _type = eTextureType::TYPE_FLOAT;
                return  ReadImageHDR(name, width, height, &_numChannels);                
            }
          
        }


        std::vector<Color4f>  ret(1);
        ret[0] = 0.5f;
        *width = *height = 1;
        return ret;
    }

    std::vector<Color4f> ReadImage(const std::string& _fileName, Vector2i& dims, int& _numChannels, eTextureType& _type)
    {
        return ReadImage(_fileName, &dims.x, &dims.y, _numChannels, _type);
    }

    bool SplitImageChannels(const std::vector<Color4f>& _in, FloatVector* _r, FloatVector* _g, FloatVector* _b, FloatVector* _a /*= nullptr*/)
    {
        for (const auto& _rgba : _in)
        {
            if (_r)
                _r->push_back(_rgba[0]);
            if (_g)
                _g->push_back(_rgba[1]);
            if (_b)
                _b->push_back(_rgba[2]);
            if (_a)
                _a->push_back(_rgba[3]);
        }
        return true;
    }



    PixelVector PixelDataToSpectrum(const std::vector<Color4f>& _in, int _numChannels)
    {
        PixelVector ret(_in.size());
        for (int i = 0; i < _in.size(); i++)
        {
            const auto& rgba = _in[i];

            RGBSpectrum c;
            c[0] = rgba[0];
            c[1] = rgba[1];
            c[2] = rgba[2];
            ret[i] = c;
        }
        return  ret;
    }

    std::vector<uint8_t> PixelDataToBytes(const std::vector<Color4f>& _in, int _numChannels)
    {
        std::vector<uint8_t> ret(_in.size() * _numChannels);
        
        for (int it = 0; it < _in.size(); it++)
        {
            const auto& rgba = _in[it];
            for (int ch = 0; ch < _numChannels; ch++) {
                ret[it  * _numChannels + ch] = static_cast<uint8_t>( std::clamp( rgba[ch] * 255.0f, 0.f, 255.f));
            }
        }
        return ret;
    }

    

    //std::vector<uint8_t> GetPixelData(const std::vector<Color4f>& _in, eTextureType _inputType, ChannelMask _requestedChannels /*= CHANNEL_RGBA*/)
    //{
    //    return {};
    //}

   

    //std::vector<uint8_t> PixelDataToU8(const std::vector<Color4f>& _in, ChannelMask _output)
    //{
    //    auto numChannels = ChannelCount(_output);        
    //    std::vector<uint8_t> ret(_in.size() * numChannels);
    //    for (int i = 0; i < _in.size(); ++i)
    //    {
    //        int idx = i * numChannels;
    //        if (_output & CHANNEL_RED)
    //            ret[idx++] = std::clamp(_in[i][0] * 255.0f, 0.0f, 255.0f);
    //        if (_output & CHANNEL_GREEN)
    //            ret[idx++] = std::clamp(_in[i][1] * 255.0f, 0.0f, 255.0f);
    //        if (_output & CHANNEL_BLUE)
    //            ret[idx++] = std::clamp(_in[i][2] * 255.0f, 0.0f, 255.0f);
    //        if (_output & CHANNEL_ALPHA)
    //            ret[idx++] = std::clamp(_in[i][3] * 255.0f, 0.0f, 255.0f);
    //    }
    //    return  ret;

    //}

    //std::vector<uint8_t> PixelDataToF16(const std::vector<Color4f>& _in, ChannelMask _output)
    //{
    //    auto numChannels = ChannelCount(_output);
    //    std::vector<uint8_t> ret(_in.size() * numChannels * sizeof(short));
    //    short* dataPtr = (short*)(ret.data());
    //    for (int i = 0; i < _in.size(); ++i)
    //    {
    //        int idx = i * numChannels;
    //        if (_output & CHANNEL_RED)
    //            dataPtr[idx++] = glm::detail::toFloat16( _in[i][0] );
    //        if (_output & CHANNEL_GREEN)
    //            dataPtr[idx++] = glm::detail::toFloat16(_in[i][1]);
    //        if (_output & CHANNEL_BLUE)
    //            dataPtr[idx++] = glm::detail::toFloat16(_in[i][2]);
    //        if (_output & CHANNEL_ALPHA)
    //            dataPtr[idx++] = glm::detail::toFloat16(_in[i][3]);
    //    }
    //    return ret;
    //}

    //std::vector<uint8_t> PixelDataToF32(const std::vector<Color4f>& _in,  ChannelMask _output)
    //{
    //    auto numChannels = ChannelCount(_output);
    //    std::vector<uint8_t> ret(_in.size() * numChannels * sizeof(float));
    //    float* dataPtr = (float*)(ret.data());
    //    for (int i = 0; i < _in.size(); ++i)
    //    {
    //        int idx = i * numChannels;
    //        if (_output & CHANNEL_RED)
    //            dataPtr[idx++] = (_in[i][0]);
    //        if (_output & CHANNEL_GREEN)
    //            dataPtr[idx++] = (_in[i][1]);
    //        if (_output & CHANNEL_BLUE)
    //            dataPtr[idx++] =(_in[i][2]);
    //        if (_output & CHANNEL_ALPHA)
    //            dataPtr[idx++] = (_in[i][3]);
    //    }
    //    return ret;
    //}

    void WriteImage(const std::string& _fileName, const float* _pixels,
        int xRes, int yRes, int totalXRes, int totalYRes, int xOffset, int yOffset, const float* _alpha)
    {
        if (_fileName.size() >= 5) {
            uint32_t suffixOffset = _fileName.size() - 4;
            if (!strcmp(_fileName.c_str() + suffixOffset, ".tga") ||
                !strcmp(_fileName.c_str() + suffixOffset, ".TGA")) {

               // const float* alpha = (_alpha.empty()) ? nullptr : _alpha.data();

                WriteImageTGA(_fileName, _pixels, _alpha, xRes, yRes, totalXRes,
                    totalYRes, xOffset, yOffset);
                return;
            }

            if (!strcmp(_fileName.c_str() + suffixOffset, ".exr") ||
                !strcmp(_fileName.c_str() + suffixOffset, ".EXR")) {
                //WriteImagePFM(_fileName, _pixels.data(), xRes, yRes);
                WriteImageEXR(_fileName, _pixels, xRes, yRes, totalXRes, totalYRes, xOffset, yOffset);
                return;
            }

            if (!strcmp(_fileName.c_str() + suffixOffset, ".pfm") ||
                !strcmp(_fileName.c_str() + suffixOffset, ".PFM")) {
                WriteImagePFM(_fileName, _pixels, xRes, yRes);
                return;
            }
            if (!strcmp(_fileName.c_str() + suffixOffset, ".png") ||
                !strcmp(_fileName.c_str() + suffixOffset, ".PNG")) {
                std::vector<uint8_t> rgb8(3 * xRes * yRes);
                uint8_t* dst = rgb8.data();
                for (int y = 0; y < yRes; ++y) {
                    for (int x = 0; x < xRes; ++x) {
#define TO_BYTE(v) (uint8_t(std::clamp(255.f * powf((v), 1.f/2.2f), 0.f, 255.f)))
                        dst[0] = TO_BYTE(_pixels[3 * (y * xRes + x) + 0]);
                        dst[1] = TO_BYTE(_pixels[3 * (y * xRes + x) + 1]);
                        dst[2] = TO_BYTE(_pixels[3 * (y * xRes + x) + 2]);
#undef TO_BYTE
                        dst += 3;
                    }
                }
                if (lodepng_encode24_file(_fileName.c_str(), rgb8.data(), xRes, yRes) != 0)
                {
                    assert(false && "Error writing png file");
                }

                
                return;
            }
        }
    }


    static std::string searchDirectory;

    bool IsAbsolutePath(const std::string& filename) {
        if (filename.empty()) return false;
        return (filename[0] == '\\' || filename[0] == '/' ||
            filename.find(':') != std::string::npos);
    }

    std::string AbsolutePath(const std::string& filename) {
        char full[_MAX_PATH];
        if (_fullpath(full, filename.c_str(), _MAX_PATH))
            return std::string(full);
        else
            return filename;
    }

    std::string ResolveFilename(const std::string& filename) {
        if (searchDirectory.empty() || filename.empty())
            return filename;
        else if (IsAbsolutePath(filename))
            return filename;

        char searchDirectoryEnd = searchDirectory[searchDirectory.size() - 1];
        if (searchDirectoryEnd == '\\' || searchDirectoryEnd == '/')
            return searchDirectory + filename;
        else
            return searchDirectory + "\\" + filename;
    }

    std::string DirectoryContaining(const std::string& filename) {
        // This code isn't tested but I believe it should work. Might need to add
        // some const_casts to make it compile though.
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char ext[_MAX_EXT];

        errno_t err = _splitpath_s(filename.c_str(), drive, _MAX_DRIVE, dir,
            _MAX_DIR, nullptr, 0, ext, _MAX_EXT);
        if (err == 0) {
            char fullDir[_MAX_PATH];
            err = _makepath_s(fullDir, _MAX_PATH, drive, dir, nullptr, nullptr);
            if (err == 0) return std::string(fullDir);
        }
        return filename;
    }

    void SetSearchDirectory(const std::string& dirname)
    {
        searchDirectory = dirname;
    }

    bool HasExtension(const std::string& value, const std::string& ending)
    {
        if (ending.size() > value.size()) return false;
        return std::equal(
            ending.rbegin(), ending.rend(), value.rbegin(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }

    void WriteImageTGA(const std::string& name, const float* pixels,
        const float* alpha, int xRes, int yRes,
        int totalXRes, int totalYRes,
        int xOffset, int yOffset)
    {
        // Reformat to BGR layout.
        uint8_t* outBuf = new uint8_t[3 * xRes * yRes];
        uint8_t* dst = outBuf;
        for (int y = 0; y < yRes; ++y) {
            for (int x = 0; x < xRes; ++x) {
#define TO_BYTE(v) (uint8_t(std::clamp(255.f * powf((v), 1.f/2.2f), 0.f, 255.f)))
                dst[0] = TO_BYTE(pixels[3 * (y * xRes + x) + 2]);
                dst[1] = TO_BYTE(pixels[3 * (y * xRes + x) + 1]);
                dst[2] = TO_BYTE(pixels[3 * (y * xRes + x) + 0]);
                dst += 3;
            }
        }

        tga_result result;
        if ((result = tga_write_bgr(name.c_str(), outBuf, xRes, yRes, 24)) != TGA_NOERR)
        {
            assert(false);
        }

        delete[] outBuf;
    }

    static std::vector<Color4f> ReadImagePNG(const std::string& name, int* w, int* h, int* _numChannels)
    {
        uint32_t width, height;
        lodepng::State state;

        std::vector<unsigned char> buffer;
        std::vector<unsigned char> image;
      
        unsigned error = lodepng::load_file(buffer, name);//load the image file with given filename
        if (error != 0) {
            return {};
        }

        error = lodepng::decode(image, width, height, state, buffer);
        if (error != 0) {
            return {};
        }

        const auto& color = state.info_png.color;
        auto numChannels = lodepng_get_channels(&color);
        *_numChannels = numChannels;
       
       
        //unsigned int error = lodepng_decode32_file(&rgb, &width, &height, name.c_str());
        //if (error != 0) {
        //    return {};
        //}
        
        std::vector<Color4f> ret(width * height);
       
        unsigned char* src = image.data();
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x, src += numChannels) {
                Color4f c;
                if( numChannels == 1 )
                {
                    c[0] = c[1] = c[2] = c[3] = src[0] / 255.f;

                }
                else if (numChannels == 3)
                {
                    c[0] = src[0] / 255.f;
                    c[1] = src[1] / 255.f;
                    c[2] = src[2] / 255.f;
                    c[3] = 1.0f;
                }
                else if (numChannels == 4)
                {
                    c[0] = src[0] / 255.f;
                    c[1] = src[1] / 255.f;
                    c[2] = src[2] / 255.f;
                    c[3] = src[3] / 255.f;
                }
                ret[y * width + x] = c;
            }
        }
       
        *w = width;
        *h = height;
        return ret;
    }


    static std::vector<Color4f> ReadImageEXR(const std::string& name, int* w, int* h, BBox2i* dataWindow, BBox2i* displayWindow)
    {
        using namespace Imf;
        using namespace Imath;
        try {
            RgbaInputFile file(name.c_str());
            Box2i dw = file.dataWindow();

            // OpenEXR uses inclusive pixel bounds; adjust to non-inclusive
            // (the convention pbrt uses) in the values returned.
            if (dataWindow)
                *dataWindow = { {dw.min.x, dw.min.y}, {dw.max.x + 1, dw.max.y + 1} };
            if (displayWindow) {
                Box2i dispw = file.displayWindow();
                *displayWindow = { {dispw.min.x, dispw.min.y},
                                  {dispw.max.x + 1, dispw.max.y + 1} };
            }
            *w = dw.max.x - dw.min.x + 1;
            *h = dw.max.y - dw.min.y + 1;
            int dims = *w * *h;

            std::vector<Rgba> pixels(*w * *h);
            file.setFrameBuffer(&pixels[0] - dw.min.x - dw.min.y * *w, 1, *w);
            file.readPixels(dw.min.y, dw.max.y);

            std::vector<Color4f>  ret(dims);
            for (int i = 0; i < dims; ++i) {
                Color4f frgba = { pixels[i].r, pixels[i].g, pixels[i].b, pixels[i].a };
                ret[i] = frgba;
            }
            /*LOG(INFO) << StringPrintf("Read EXR image %s (%d x %d)",
                name.c_str(), *width, *height);*/
            return ret;
        }
        catch (const std::exception& ) {
            //Error("Unable to read image file \"%s\": %s", name.c_str(), e.what());
        }
        return {};
    }


    static std::vector<Color4f> ReadImageTGA(const std::string& name, int* width, int* height, int* _numChannels)
    {
        tga_image img;
        tga_result result;
        if ((result = tga_read(&img, name.c_str())) != TGA_NOERR) {
            Error("Unable to read from TGA file \"%s\" (%s)",
                name.c_str(), tga_error(result));
            return {};
        }

        if (tga_is_right_to_left(&img)) tga_flip_horiz(&img);
        if (!tga_is_top_to_bottom(&img)) tga_flip_vert(&img);
        if (tga_is_colormapped(&img)) tga_color_unmap(&img);

        if (tga_is_mono(&img))
            *_numChannels = 1;
        if (img.pixel_depth == 24)
            *_numChannels = 3;
        if (img.pixel_depth == 32)
            *_numChannels = 4;

        *width = img.width;
        *height = img.height;

        // "Unpack" the pixels (origin in the lower left corner).
        // TGA pixels are in BGRA format.
        //RGBSpectrum* ret = new RGBSpectrum[*width * *height];
        std::vector<Color4f> ret(*width * *height);
        Color4f* dst = ret.data();
        for (int y = 0; y < *height; y++)
            for (int x = 0; x < *width; x++) {
                uint8_t* src = tga_find_pixel(&img, x, y);
                if (tga_is_mono(&img)) {
                    *dst++ = Color4f(*src / 255.f);
                }
                else if (img.pixel_depth == 24 ){
                    float c[3];
                    c[2] = src[0] / 255.f;
                    c[1] = src[1] / 255.f;
                    c[0] = src[2] / 255.f;
                    *dst++ = Color4f( c[0], c[1], c[2], 1.0 );
                }
                else if (img.pixel_depth == 32)
                {
                    float c[4];
                    c[3] = src[3] / 255.f;
                    c[2] = src[0] / 255.f;
                    c[1] = src[1] / 255.f;
                    c[0] = src[2] / 255.f;
                    *dst++ = Color4f(c[0], c[1], c[2], c[3]);
                }
            }

        tga_free_buffers(&img);

        return ret;
    }



    // PFM Function Definitions
    /*
     * PFM reader/writer code courtesy Jiawen "Kevin" Chen (http://people.csail.mit.edu/jiawen/)
     */

    static inline int isWhitespace(char c)
    {
        return c == ' ' || c == '\n' || c == '\t';
    }



    // reads a "word" from the fp and puts it into buffer
    // and adds a null terminator
    // i.e. it keeps reading until a whitespace is reached
    // returns the number of characters read
    // *not* including the whitespace
    // return -1 on an error
    static int readWord(FILE* fp, char* buffer, int bufferLength) {
        int n;
        char c;

        if (bufferLength < 1)
            return -1;

        n = 0;
        c = fgetc(fp);
        while (c != EOF && !isWhitespace(c) && n < bufferLength) {
            buffer[n] = c;
            ++n;
            c = fgetc(fp);
        }

        if (n < bufferLength) {
            buffer[n] = '\0';
            return n;
        }

        return -1;
    }

    static std::vector<Color4f>  ReadImageHDR(const std::string& name, int* w, int* h, int* _numChannels)
    {
        float* pData = stbi_loadf(name.c_str(), w, h, _numChannels, 0);
        std::vector<Color4f> ret(*w * *h);
       
        int ch = *_numChannels;
        for (int i = 0; i < *w * *h; ++i)
        {
            int idx = i * ch;
            for (int j = 0; j < ch; ++j)
                ret[i][j] = pData[idx + j];
        }
        free(pData);
        return ret;
    }



    static std::vector<Color4f> ReadImageJPG(const std::string& name, int* width, int* height)
    {
        int numChannels;
        auto pData = stbi_load(name.c_str(), width, height, &numChannels, 0);
        if (!pData)
            return{};
        std::vector<Color4f> ret(*width * *height);
        for (int i = 0; i < *width * *height; ++i)
        {
            int idx = i * numChannels;            
            ret[i][0] = pData[idx + 0] / 255.0f;
            ret[i][1] = pData[idx + 1] / 255.0f;
            ret[i][2] = pData[idx + 2] / 255.0f;
            ret[i][3] = 1.0f;
        }
        free(pData);
        return ret;
    }


    static  std::vector<Color4f> ReadImagePFM(const std::string& filename, int* xres, int* yres)
    {

        float* data = nullptr;
        std::vector<Color4f> rgb;
        char buffer[BUFFER_SIZE];
        unsigned int nFloats;
        int nChannels, width, height;
        float scale;
        bool fileLittleEndian;

        FILE* fp = fopen(filename.c_str(), "rb") ;
        if (!fp) goto fail;
        // read either "Pf" or "PF"
        if (readWord(fp, buffer, BUFFER_SIZE) == -1) goto fail;

        if (strcmp(buffer, "Pf") == 0)
            nChannels = 1;
        else if (strcmp(buffer, "PF") == 0)
            nChannels = 3;
        else
            goto fail;


        // read the rest of the header
        // read width
        if (readWord(fp, buffer, BUFFER_SIZE) == -1) goto fail;
        width = atoi(buffer);
        *xres = width;

        // read height
        if (readWord(fp, buffer, BUFFER_SIZE) == -1) goto fail;
        height = atoi(buffer);
        *yres = height;

        // read scale
        if (readWord(fp, buffer, BUFFER_SIZE) == -1) goto fail;
        sscanf(buffer, "%f", &scale);

        // read the data
        nFloats = nChannels * width * height;
        data = new float[nFloats];
        // Flip in Y, as P*M has the origin at the lower left.
        for (int y = height - 1; y >= 0; --y) {
            if (fread(&data[y * nChannels * width], sizeof(float),
                nChannels * width, fp) != nChannels * width)
                goto fail;
        }

        // apply endian conversian and scale if appropriate
        fileLittleEndian = (scale < 0.f);
        if (hostLittleEndian ^ fileLittleEndian) {
            uint8_t bytes[4];
            for (unsigned int i = 0; i < nFloats; ++i) {
                memcpy(bytes, &data[i], 4);
                std::swap(bytes[0], bytes[3]);
                std::swap(bytes[1], bytes[2]);
                memcpy(&data[i], bytes, 4);
            }
        }
        if (std::abs(scale) != 1.f)
            for (unsigned int i = 0; i < nFloats; ++i) data[i] *= std::abs(scale);

        rgb.resize(width * height);
        if (nChannels == 1) {
            for (int i = 0; i < width * height; ++i) rgb[i] = Color4f(data[i]);
        }
        else {
            for (int i = 0; i < width * height; ++i) {
                float frgb[3] = { data[3 * i], data[3 * i + 1], data[3 * i + 2] };
                rgb[i] = Color4f(frgb[0], frgb[1], frgb[2], 1.0);
            }
        }

        delete[] data;
        fclose(fp);        
        return rgb;

    fail:
        Error("Error reading PFM file \"%s\"", filename.c_str());
        if (fp) fclose(fp);
        delete[] data;
        return {};
    }


    static void WriteImageEXR(const std::string& name, const float* pixels, int xRes, int yRes, int totalXRes, int totalYRes, int xOffset, int yOffset)
    {
        using namespace Imf;
        using namespace Imath;
        std::vector<Rgba> hrgba(xRes * yRes);

        for (int i = 0; i < xRes * yRes; ++i)
            hrgba[i] = Rgba(pixels[3 * i], pixels[3 * i + 1], pixels[3 * i + 2]);

        // OpenEXR uses inclusive pixel bounds.
        Box2i displayWindow(V2i(0, 0), V2i(totalXRes - 1, totalYRes - 1));
        Box2i dataWindow(V2i(xOffset, yOffset),
            V2i(xOffset + xRes - 1, yOffset + yRes - 1));

        try {
            RgbaOutputFile file(name.c_str(), displayWindow, dataWindow,
                WRITE_RGB);
            auto dataPtr = hrgba.data();
            file.setFrameBuffer(dataPtr - xOffset - yOffset * xRes, 1, xRes);
            file.writePixels(yRes);
        }
        catch (const std::exception& exc) {
            Error("Error writing \"%s\": %s", name.c_str(), exc.what());
        }

       
    }



    static bool WriteImagePFM(const std::string& filename, const float* rgb,
        int width, int height)
    {
        FILE* fp;
        unsigned int nFloats;
        float scale;

        fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            return false;
        }

        // only write 3 channel PFMs here...
        if (fprintf(fp, "PF\n") < 0)
            goto fail;

        // write the width and height, which must be positive
        if (fprintf(fp, "%d %d\n", width, height) < 0)
            goto fail;

        // write the scale, which encodes endianness
        scale = hostLittleEndian ? -1.f : 1.f;
        if (fprintf(fp, "%f\n", scale) < 0)
            goto fail;

        // write the data from bottom left to upper right as specified by 
        // http://netpbm.sourceforge.net/doc/pfm.html
        // The raster is a sequence of pixels, packed one after another, with no
        // delimiters of any kind. They are grouped by row, with the pixels in each
        // row ordered left to right and the rows ordered bottom to top.
        nFloats = 3 * width * height;
        for (int j = height - 1; j >= 0; j--) {
            if (fwrite(rgb + j * width * 3, sizeof(float), width * 3, fp) < (size_t)(width * 3))
                goto fail;
        }

        fclose(fp);
        return true;

    fail:
        fclose(fp);
        return false;
    }


    bool ReadFloatFile(const std::string& _fileName, std::vector<float>& values)
    {
        FILE* f = fopen(_fileName.c_str(), "r");
        if (!f) {
            //Error("Unable to open file \"%s\"", filename);
            //throw std::exception("Unable To Open File");
            return false;
        }

        int c;
        bool inNumber = false;
        char curNumber[32];
        int curNumberPos = 0;
        int lineNumber = 1;
        while ((c = getc(f)) != EOF) {
            if (c == '\n') ++lineNumber;
            if (inNumber)
            {

                if (isdigit(c) || c == '.' || c == 'e' || c == '-' || c == '+')
                    curNumber[curNumberPos++] = c;
                else {
                    curNumber[curNumberPos++] = '\0';
                    values.push_back((float)atof(curNumber));
                    inNumber = false;
                    curNumberPos = 0;
                }
            }
            else {
                if (isdigit(c) || c == '.' || c == '-' || c == '+') {
                    inNumber = true;
                    curNumber[curNumberPos++] = c;
                }
                else if (c == '#') {
                    while ((c = getc(f)) != '\n' && c != EOF)
                        ;
                    ++lineNumber;
                }
                else if (!isspace(c)) {
                    fclose(f);
                    return false;
                    //    Warning("Unexpected text found at line %d of float file \"%s\"",
                     //     lineNumber, filename);
                }
            }
        }
        fclose(f);
        return true;
    }

  


    bool ReadTextFile(const std::string& _fileName, std::string& _result)
    {
        std::ifstream file(_fileName);
        if (!file.is_open())
            return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        _result = buffer.str();
        return true;
    }

#pragma endregion

}

