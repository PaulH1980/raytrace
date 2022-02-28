#pragma once
#define GLEW_STATIC
#include <glew.h>
#include "Defines.h"
#include "FrontEndDef.h"


namespace RayTrace
{
    
    enum class eDrawMode
    {
        DRAWMODE_POINTS = GL_POINTS,
        DRAWMODE_LINE_STRIP = GL_LINE_STRIP,
        DRAWMODE_LINE_LOOP = GL_LINE_LOOP,
        DRAWMODE_LINES = GL_LINES,
        DRAWMODE_LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
        DRAWMODE_LINES_ADJACENCY = GL_LINES_ADJACENCY,
        DRAWMODE_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        DRAWMODE_TRIANGLE_FAN = GL_TRIANGLE_FAN,
        DRAWMODE_TRIANGLES = GL_TRIANGLES,
        DRAWMODE_TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
        DRAWMODE_TRIANGLES_ADJACENCY = GL_TRIANGLES_ADJACENCY,
        DRAWMODE_PATCHES = GL_PATCHES,
    };
    
    
    enum eVertexBufferTarget
    {
        VERTEX_BUFFER_TYPE_UNDEFINED = -1,
        VERTEX_BUFFER_VERTEX = GL_ARRAY_BUFFER,
        VERTEX_BUFFER_INDEX = GL_ELEMENT_ARRAY_BUFFER,
        VERTEX_BUFFER_STORAGE = GL_SHADER_STORAGE_BUFFER,
        VERTEX_BUFFER_UNIFORM = GL_UNIFORM_BUFFER,
        VERTEX_BUFFER_COPY_READ_BUFFER = GL_COPY_READ_BUFFER,
        VERTEX_BUFFER_COPY_WRITE_BUFFER = GL_COPY_WRITE_BUFFER,
        VERTEX_BUFFER_DISPATCH_INDIRECT_BUFFER = GL_DISPATCH_INDIRECT_BUFFER,
        VERTEX_BUFFER_DRAW_INDIRECT_BUFFER = GL_DRAW_INDIRECT_BUFFER,
        VERTEX_BUFFER_PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER,
        VERTEX_BUFFER_PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER,
        VERTEX_BUFFER_QUERY_BUFFER = GL_QUERY_BUFFER,
        VERTEX_BUFFER_TEXTURE_BUFFER = GL_TEXTURE_BUFFER,
        VERTEX_BUFFER_TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER
    };

    enum eVertexBufferUsage
    {
        VERTEX_BUFFER_USAGE_UNDEFINED = -1,
        VERTEX_BUFFER_USAGE_STATIC_DRAW = GL_STATIC_DRAW,
        VERTEX_BUFFER_USAGE_STATIC_COPY = GL_STATIC_COPY,
        VERTEX_BUFFER_USAGE_STATIC_READ = GL_STATIC_READ,

        VERTEX_BUFFER_USAGE_STREAM_DRAW = GL_STREAM_DRAW,
        VERTEX_BUFFER_USAGE_STREAM_COPY = GL_STREAM_COPY,
        VERTEX_BUFFER_USAGE_STREAM_READ = GL_STREAM_READ,

        VERTEX_BUFFER_USAGE_DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
        VERTEX_BUFFER_USAGE_DYNAMIC_COPY = GL_DYNAMIC_COPY,
        VERTEX_BUFFER_USAGE_DYNAMIC_READ = GL_DYNAMIC_READ,

    };

    enum eVertexBufferAccess {
        VERTEX_BUFFER_ACCEES_READ_ONLY = GL_READ_ONLY,
        VERTEX_BUFFER_ACCEES_WRITE_ONLY = GL_WRITE_ONLY,
        VERTEX_BUFFER_ACCEES_READ_WRITE = GL_READ_WRITE
    };


    enum class eTarget
    {
        TARGET_TEXTURE_UNDEFINED = -1,
        TARGET_TEXTURE_1D = GL_TEXTURE_1D,
        //2D stuff                   
        TARGET_TEXTURE_2D = GL_TEXTURE_2D,
        TARGET_TEXTURE_CUBE_MAP_POSITIVE_X = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        TARGET_TEXTURE_CUBE_MAP_NEGATIVE_X = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        TARGET_TEXTURE_CUBE_MAP_POSITIVE_Y = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        TARGET_TEXTURE_CUBE_MAP_NEGATIVE_Y = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        TARGET_TEXTURE_CUBE_MAP_POSITIVE_Z = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        TARGET_TEXTURE_CUBE_MAP_NEGATIVE_Z = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
        TARGET_TEXTURE_1D_ARRAY = GL_TEXTURE_1D_ARRAY,
        //3D Stuff                   
        TARGET_TEXTURE_3D = GL_TEXTURE_3D,
        TARGET_TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY,
        TARGET_TEXTURE_CUBE_MAP = GL_TEXTURE_CUBE_MAP
    };



    enum class eMinFilter
    {
        MIN_FILTER_NEAREST = GL_NEAREST,
        MIN_FILTER_LINEAR = GL_LINEAR,
        MIN_FILTER_NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        MIN_FILTER_LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
        MIN_FILTER_NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
        MIN_FILTER_LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
    };

    enum class eMagFilter
    {
        MAG_FILTER_NEAREST = GL_NEAREST,
        MAG_FILTER_LINEAR = GL_LINEAR
    };

    enum class eWrapMode
    {
        WRAP_MODE_CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
        WRAP_MODE_MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
        WRAP_MODE_REPEAT = GL_REPEAT
    };

    enum class eFormat
    {
        FORMAT_UNDEFINED = -1,
        
        FORMAT_RED = GL_RED,
        FORMAT_RG = GL_RG,
        FORMAT_RGB = GL_RGB,
        FORMAT_BGR = GL_BGR,
        FORMAT_RGBA = GL_RGBA,
        FORMAT_BGRA = GL_BGRA,
        FORMAT_RED_INTEGER = GL_RED_INTEGER,
        FORMAT_RG_INTEGER = GL_RG_INTEGER,
        FORMAT_RGB_INTEGER = GL_RGB_INTEGER,
        FORMAT_BGR_INTEGER = GL_BGR_INTEGER,
        FORMAT_RGBA_INTEGER = GL_RGBA_INTEGER,
        FORMAT_BGRA_INTEGER = GL_BGRA_INTEGER,
        FORMAT_STENCIL_INDEX = GL_STENCIL_INDEX,
        FORMAT_DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
        FORMAT_DEPTH_STENCIL = GL_DEPTH_STENCIL
    };

    enum class eInternalFormat
    {

        INTERNAL_UNDEFINED = -1,

        INTERNAL_DEPTH_COMPONENT16 = GL_DEPTH_COMPONENT16,
        INTERNAL_DEPTH_COMPONENT24 = GL_DEPTH_COMPONENT24,
        INTERNAL_DEPTH_COMPONENT32 = GL_DEPTH_COMPONENT32,
        INTERNAL_DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F,
        INTERNAL_DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
        INTERNAL_DEPTH32F_STENCIL8 = GL_DEPTH32F_STENCIL8,

        INTERNAL_DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
        INTERNAL_DEPTH_STENCIL = GL_DEPTH_STENCIL,
        INTERNAL_RED = GL_RED,
        INTERNAL_RG = GL_RG,
        INTERNAL_RGB = GL_RGB,
        INTERNAL_RGBA = GL_RGBA,
        //sized internal                                                            
        INTERNAL_R8 = GL_R8,
        INTERNAL_R8_SNORM = GL_R8_SNORM,
        INTERNAL_R16 = GL_R16,
        INTERNAL_R16_SNORM = GL_R16_SNORM,
        INTERNAL_RG8 = GL_RG8,
        INTERNAL_RG8_SNORM = GL_RG8_SNORM,
        INTERNAL_RG16 = GL_RG16,
        INTERNAL_RG16_SNORM = GL_RG16_SNORM,
        INTERNAL_R3_G3_B2 = GL_R3_G3_B2,
        INTERNAL_RGB4 = GL_RGB4,
        INTERNAL_RGB5 = GL_RGB5,
        INTERNAL_RGB8 = GL_RGB8,
        INTERNAL_RGB8_SNORM = GL_RGB8_SNORM,
        INTERNAL_RGB10 = GL_RGB10,
        INTERNAL_RGB12 = GL_RGB12,
        INTERNAL_RGB16 = GL_RGB16,
        INTERNAL_RGB16_SNORM = GL_RGB16_SNORM,
        INTERNAL_RGBA2 = GL_RGBA2,
        INTERNAL_RGBA4 = GL_RGBA4,
        INTERNAL_RGB5_A1 = GL_RGB5_A1,
        INTERNAL_RGBA8 = GL_RGBA8,
        INTERNAL_RGBA8_SNORM = GL_RGBA8_SNORM,
        INTERNAL_RGB10_A2 = GL_RGB10_A2,
        INTERNAL_RGB10_A2UI = GL_RGB10_A2UI,
        INTERNAL_RGBA12 = GL_RGBA12,
        INTERNAL_RGBA16 = GL_RGBA16,
        INTERNAL_SRGB8 = GL_SRGB8,
        INTERNAL_SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
        INTERNAL_R16F = GL_R16F,
        INTERNAL_RG16F = GL_RG16F,
        INTERNAL_RGB16F = GL_RGB16F,
        INTERNAL_RGBA16F = GL_RGBA16F,
        INTERNAL_R32F = GL_R32F,
        INTERNAL_RG32F = GL_RG32F,
        INTERNAL_RGB32F = GL_RGB32F,
        INTERNAL_RGBA32F = GL_RGBA32F,
        INTERNAL_R11F_G11F_B10F = GL_R11F_G11F_B10F,
        INTERNAL_RGB9_E5 = GL_RGB9_E5,
        INTERNAL_R8I = GL_R8I,
        INTERNAL_R8UI = GL_R8UI,
        INTERNAL_R16I = GL_R16I,
        INTERNAL_R16UI = GL_R16UI,
        INTERNAL_R32I = GL_R32I,
        INTERNAL_R32UI = GL_R32UI,
        INTERNAL_RG8I = GL_RG8I,
        INTERNAL_RG8UI = GL_RG8UI,
        INTERNAL_RG16I = GL_RG16I,
        INTERNAL_RG16UI = GL_RG16UI,
        INTERNAL_RG32I = GL_RG32I,
        INTERNAL_RG32UI = GL_RG32UI,
        INTERNAL_RGB8I = GL_RGB8I,
        INTERNAL_RGB8UI = GL_RGB8UI,
        INTERNAL_RGB16I = GL_RGB16I,
        INTERNAL_RGB16UI = GL_RGB16UI,
        INTERNAL_RGB32I = GL_RGB32I,
        INTERNAL_RGB32UI = GL_RGB32UI,
        INTERNAL_RGBA8I = GL_RGBA8I,
        INTERNAL_RGBA8UI = GL_RGBA8UI,
        INTERNAL_RGBA16I = GL_RGBA16I,
        INTERNAL_RGBA16UI = GL_RGBA16UI,
        INTERNAL_RGBA32I = GL_RGBA32I,
        INTERNAL_RGBA32UI = GL_RGBA32UI,
        //compressed
        INTERNAL_COMPRESSED_RED = GL_COMPRESSED_RED,
        INTERNAL_COMPRESSED_RG = GL_COMPRESSED_RG,
        INTERNAL_COMPRESSED_RGB = GL_COMPRESSED_RGB,
        INTERNAL_COMPRESSED_RGBA = GL_COMPRESSED_RGBA,
        INTERNAL_COMPRESSED_SRGB = GL_COMPRESSED_SRGB,
        INTERNAL_COMPRESSED_SRGB_ALPHA = GL_COMPRESSED_SRGB_ALPHA,
        INTERNAL_COMPRESSED_RED_RGTC1 = GL_COMPRESSED_RED_RGTC1,
        INTERNAL_COMPRESSED_SIGNED_RED_RGTC1 = GL_COMPRESSED_SIGNED_RED_RGTC1,
        INTERNAL_COMPRESSED_RG_RGTC2 = GL_COMPRESSED_RG_RGTC2,
        INTERNAL_COMPRESSED_SIGNED_RG_RGTC2 = GL_COMPRESSED_SIGNED_RG_RGTC2,
        INTERNAL_COMPRESSED_RGBA_BPTC_UNORM = GL_COMPRESSED_RGBA_BPTC_UNORM,
        INTERNAL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
        INTERNAL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
        INTERNAL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
    };

    


    enum class eTextureType
    {
        TYPE_UNDEFINED                              = -1,
        
        TYPE_UNSIGNED_BYTE                          = GL_UNSIGNED_BYTE,
        TYPE_BYTE                                   = GL_BYTE,
        TYPE_UNSIGNED_SHORT                         = GL_UNSIGNED_SHORT,
        TYPE_SHORT                                  = GL_SHORT,
        TYPE_UNSIGNED_INT                           = GL_UNSIGNED_INT,
        TYPE_INT                                    = GL_INT,
        TYPE_HALF_FLOAT                             = GL_HALF_FLOAT,
        TYPE_FLOAT                                  = GL_FLOAT,
        TYPE_UNSIGNED_BYTE_3_3_2                    = GL_UNSIGNED_BYTE_3_3_2,
        TYPE_UNSIGNED_BYTE_2_3_3_REV                = GL_UNSIGNED_BYTE_2_3_3_REV,
        TYPE_UNSIGNED_SHORT_5_6_5                   = GL_UNSIGNED_SHORT_5_6_5,
        TYPE_UNSIGNED_SHORT_5_6_5_REV               = GL_UNSIGNED_SHORT_5_6_5_REV,
        TYPE_UNSIGNED_SHORT_4_4_4_4                 = GL_UNSIGNED_SHORT_4_4_4_4,
        TYPE_UNSIGNED_SHORT_4_4_4_4_REV             = GL_UNSIGNED_SHORT_4_4_4_4_REV,
        TYPE_UNSIGNED_SHORT_5_5_5_1                 = GL_UNSIGNED_SHORT_5_5_5_1,
        TYPE_UNSIGNED_SHORT_1_5_5_5_REV             = GL_UNSIGNED_SHORT_1_5_5_5_REV,
        TYPE_UNSIGNED_INT_8_8_8_8                   = GL_UNSIGNED_INT_8_8_8_8,
        TYPE_UNSIGNED_INT_8_8_8_8_REV               = GL_UNSIGNED_INT_8_8_8_8_REV,
        TYPE_UNSIGNED_INT_10_10_10_2                = GL_UNSIGNED_INT_10_10_10_2,
        TYPE_UNSIGNED_INT_2_10_10_10_REV            = GL_UNSIGNED_INT_2_10_10_10_REV,
        TYPE_UNSIGNED_INT_24_8                      = GL_UNSIGNED_INT_24_8,
        TYPE_FLOAT_32_UNSIGNED_INT_24_8_REV         = GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
        TYPE_UNSIGNED_INT_10F_11F_11F_REV           = GL_UNSIGNED_INT_10F_11F_11F_REV,
        TYPE_UNSIGNED_INT_5_9_9_9_REV               = GL_UNSIGNED_INT_5_9_9_9_REV
    };

    struct ImageFormat
    {
        int                m_numChannels = {0 };
        int                m_redBits = 0;
        int                m_greenBits = 0;
        int                m_blueBits = 0;
        int                m_alphaBits = 0;

        eFormat            m_format = eFormat::FORMAT_RGB;
        eTextureType       m_type   = eTextureType::TYPE_UNSIGNED_BYTE;

        int                GetNumBytes() const {
            return (m_redBits + m_greenBits + m_blueBits + m_alphaBits) / 8;
        }
    };

    inline ImageFormat GetFormatInfo(eInternalFormat _format)
    {
        static std::once_flag flag;
        static std::map<eInternalFormat, ImageFormat> SizedBytesPerPixel;
        std::call_once(flag, [&]() {
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R8] = { 1,  8,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R8_SNORM] = { 1,  8,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R16] = { 1, 16,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R16_SNORM] = { 1, 16,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG8] = { 2,  8,  8,  0,  0, eFormat::FORMAT_RG, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG8_SNORM] = { 2,  8,  8,  0,  0, eFormat::FORMAT_RG, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG16] = { 2, 16, 16,  0,  0, eFormat::FORMAT_RG, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG16_SNORM] = { 2, 16, 16,  0,  0, eFormat::FORMAT_RG, eTextureType::TYPE_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R3_G3_B2] = { 3,  3,  3,  2,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_BYTE_2_3_3_REV };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB4] = { 3,  4,  4,  4,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB5] = { 3,  5,  5,  5,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB8] = { 3,  8,  8,  8,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB8_SNORM] = { 3,  8,  8,  8,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB10] = { 3, 10, 10, 10,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB12] = { 3, 12, 12, 12,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB16] = { 3, 16, 16, 16,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB16_SNORM] = { 3, 16, 16, 16,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA2] = { 4,  2,  2,  2,  2, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA4] = { 4,  4,  4,  4,  4, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB5_A1] = { 4,  5,  5,  5,  1, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA8] = { 4,  8,  8,  8,  8, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA8_SNORM] = { 4,  8,  8,  8,  8, eFormat::FORMAT_RGBA, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB10_A2] = { 4, 10, 10, 10,  2, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_INT_10_10_10_2 };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB10_A2UI] = { 4, 10, 10, 10,  2, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_UNSIGNED_INT_10_10_10_2 };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA12] = { 4, 12, 12, 12, 12, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA16] = { 4, 16, 16, 16, 16, eFormat::FORMAT_RGBA, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_SRGB8] = { 3,  8,  8,  8,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_SRGB8_ALPHA8] = { 4,  8,  8,  8,  8, eFormat::FORMAT_RGBA, eTextureType::TYPE_BYTE };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R16F] = { 1, 16,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_HALF_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG16F] = { 2, 16, 16,  0,  0, eFormat::FORMAT_RG,  eTextureType::TYPE_HALF_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB16F] = { 3, 16, 16, 16,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_HALF_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA16F] = { 4, 16, 16, 16, 16, eFormat::FORMAT_RGBA, eTextureType::TYPE_HALF_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R32F] = { 1, 32,  0,  0,  0, eFormat::FORMAT_RED, eTextureType::TYPE_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG32F] = { 2, 32, 32,  0,  0, eFormat::FORMAT_RG,  eTextureType::TYPE_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB32F] = { 3, 32, 32, 32,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA32F] = { 4, 32, 32, 32, 32, eFormat::FORMAT_RGBA, eTextureType::TYPE_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R11F_G11F_B10F] = { 3, 11, 11, 10,  0, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_INT_10F_11F_11F_REV };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB9_E5] = { 4,  9,  9,  9,  5, eFormat::FORMAT_RGB, eTextureType::TYPE_UNSIGNED_INT_5_9_9_9_REV };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R8I] = { 1, 8,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R8UI] = { 1, 8,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R16I] = { 1, 16,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R16UI] = { 1, 16,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R32I] = { 1, 32,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_R32UI] = { 1, 32,  0,  0,  0, eFormat::FORMAT_RED_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG8I] = { 2, 8, 8,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG8UI] = { 2, 8, 8,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG16I] = { 2, 16, 16,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG16UI] = { 2, 16, 16,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG32I] = { 2, 32, 32,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RG32UI] = { 2, 32, 32,  0,  0, eFormat::FORMAT_RG_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB8I] = { 3, 8, 8, 8,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB8UI] = { 3, 8, 8, 8,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB16I] = { 3, 16, 16, 16,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB16UI] = { 3, 16, 16, 16,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB32I] = { 3, 32, 32, 32,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGB32UI] = { 3, 32, 32, 32,  0, eFormat::FORMAT_RGB_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA8I] = { 4, 8, 8, 8, 8, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA8UI] = { 4, 8, 8, 8, 8, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA16I] = { 4, 16, 16, 16, 16, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA16UI] = { 4, 16, 16, 16, 16, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA32I] = { 4, 32, 32, 32, 32, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_RGBA32UI] = { 4, 32, 32, 32, 32, eFormat::FORMAT_RGBA_INTEGER, eTextureType::TYPE_UNSIGNED_INT };

            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH_COMPONENT16] = { 1, 16,  0,  0,  0, eFormat::FORMAT_DEPTH_COMPONENT, eTextureType::TYPE_UNSIGNED_SHORT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH_COMPONENT24] = { 1, 24,  0,  0,  0, eFormat::FORMAT_DEPTH_COMPONENT, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH_COMPONENT32] = { 1, 32,  0,  0,  0, eFormat::FORMAT_DEPTH_COMPONENT, eTextureType::TYPE_UNSIGNED_INT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH_COMPONENT32F] = { 1, 32,  0,  0,  0, eFormat::FORMAT_DEPTH_COMPONENT, eTextureType::TYPE_FLOAT };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH24_STENCIL8] = { 1, 32,  0,  0,  0, eFormat::FORMAT_DEPTH_STENCIL, eTextureType::TYPE_UNSIGNED_INT_24_8 };
            SizedBytesPerPixel[eInternalFormat::INTERNAL_DEPTH32F_STENCIL8] = { 2, 32,  32,  0,  0, eFormat::FORMAT_DEPTH_STENCIL, eTextureType::TYPE_FLOAT_32_UNSIGNED_INT_24_8_REV };
        });

        auto it = SizedBytesPerPixel.find(_format);
        if (it == std::end(SizedBytesPerPixel))
            return {};
        return it->second;
    }

    inline bool IsByte(eTextureType _type)
    {
        return _type == eTextureType::TYPE_BYTE ||
               _type == eTextureType::TYPE_UNSIGNED_BYTE;
    }

    inline bool IsShort(eTextureType _type)
    {
        return _type == eTextureType::TYPE_SHORT ||
               _type == eTextureType::TYPE_UNSIGNED_SHORT;
    }

    inline  bool IsInteger(eTextureType _type)
    {
        return _type == eTextureType::TYPE_INT ||
               _type == eTextureType::TYPE_UNSIGNED_INT;
    }

    inline  bool IsHalfFloat(eTextureType _type)
    {
        return _type == eTextureType::TYPE_HALF_FLOAT;
    }


    inline  bool IsFloatingPoint(eTextureType _type)
    {
        return _type == eTextureType::TYPE_HALF_FLOAT ||
               _type == eTextureType::TYPE_FLOAT;
    }  

    inline bool IsDepth(eFormat _format)
    {
        return _format == eFormat::FORMAT_DEPTH_COMPONENT || 
               _format == eFormat::FORMAT_DEPTH_STENCIL;
    }

    inline bool IsStencil(eFormat _format) {
        return _format == eFormat::FORMAT_DEPTH_STENCIL;
    }


    inline bool IsDepthStencil(eFormat _format)
    {
        return IsDepth(_format) && IsStencil(_format);
    }


    inline bool IsColor(eFormat _format)
    {
        if (_format == eFormat::FORMAT_UNDEFINED)
            return false;
        
        if (IsDepth(_format) || IsStencil(_format))
            return false;

        return true;        
    }
}