#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <cctype>
#include <string.h>
#include "Color.h"
#include "HWFormats.h"
#include "Defines.h"
#include "ImageChannels.h"

namespace RayTrace
{
    struct ImageExInfo
    {
        //format as retrieved from medium
        ChannelMask          m_channels = ChannelMask::CHANNEL_UNDEFINED; //as read from medium
        eInternalFormat      m_internalFormat = eInternalFormat::INTERNAL_UNDEFINED; //as read from medium
        eTextureType         m_type = eTextureType::TYPE_UNDEFINED;
        uint32_t             m_numChannels = 0;

        //will be filled after a successful read      
        std::vector<uint8_t> m_rawData;
        uint32_t             m_width = 0;
        uint32_t             m_height = 0;
        uint32_t             m_depth = 1;

        bool                 m_writeFlipped = false;      
        bool                 m_readFlipped  = false;
        bool                 m_readConvertToF16F = true; //convert images to fp16 
        
    };

    bool SplitChannels(const ImageExInfo& _data, U8Vector* _r, U8Vector* _g, U8Vector* _b, U8Vector* _a = nullptr);
    bool CombineChannels(eInternalFormat _format, int _width, int _height,    ImageExInfo& _data, U8Vector* _r, U8Vector* _g, U8Vector* _b, U8Vector* _a = nullptr);

    bool ReadImage(const std::string& _fileName, ImageExInfo& _imageData);
    bool WriteImage(const std::string& _fileName, const ImageExInfo& _imageData);
    bool ResizeImage(const ImageExInfo& _in, ImageExInfo& _out);

    void SwapImage(ImageExInfo& _data, bool _swapRows = true);

    bool ReadImageJPG(const std::string& _name, ImageExInfo& _info);
    bool ReadImagePNG(const std::string& _name, ImageExInfo& _info);
    bool ReadImageHDR(const std::string& _name, ImageExInfo& _info);
    bool ReadImageEXR(const std::string& _name, ImageExInfo& _info);
    bool ReadImageTGA(const std::string& _name, ImageExInfo& _info);


    bool WriteImagePNG(const std::string& _name, const ImageExInfo& _info);
    bool WriteImageHDR(const std::string& _name, const ImageExInfo& _info);
    bool WriteImageTGA(const std::string& _name, const ImageExInfo& _info);

    std::vector<RGBSpectrum>     PixelDataToSpectrum(const std::vector<Color4f>& _in, int _numChannels);
    bool                         SplitImageChannels(const std::vector<Color4f>& _in, FloatVector* _r, FloatVector* _g, FloatVector* _b, FloatVector* _a = nullptr);

    
    bool ReadFloatFile(const std::string& _fileName, std::vector<float>& values);
    bool ReadTextFile(const std::string& _fileName, std::string& _reesult);


    std::vector<Color4f> ReadImage(const std::string& _fileName, int* _width, int* _height,int& numChannels, eTextureType& _type );
    std::vector<Color4f> ReadImage(const std::string& _fileName, Vector2i& dims, int& numChannels, eTextureType& _type);
  
  

    //std::vector<uint8_t>    PixelDataToBytes(const std::vector<Color4f>& _in, int _numChannels);
    std::vector<uint8_t>    PixelDataToBytes(const std::vector<Color4f>& _in, int _numChannels      );
   

    //std::vector<uint8_t>    GetPixelData(const std::vector<Color4f>& _in, eTextureType _inputType, ChannelMask _requestedChannels = CHANNEL_RGBA);

    //std::vector<uint8_t>    PixelDataToU8(const std::vector<Color4f>& _in,  ChannelMask _output);
    //std::vector<uint8_t>    PixelDataToF16(const std::vector<Color4f>& _in, ChannelMask _output);
    //std::vector<uint8_t>    PixelDataToF32(const std::vector<Color4f>& _in, ChannelMask _output);
    
    void        WriteImage(const std::string& _fileName, const float* _pixels,
        int XRes, int YRes, int totalXRes, int totalYRes, int xOffset, int yOffset, const float* _alpha);


    // Platform independent filename-handling functions.
    bool        IsAbsolutePath(const std::string& filename);
    std::string AbsolutePath(const std::string& filename);
    std::string ResolveFilename(const std::string& filename);
    std::string DirectoryContaining(const std::string& filename);
    void        SetSearchDirectory(const std::string& dirname);

    bool HasExtension(const std::string& value, const std::string& ending);



}