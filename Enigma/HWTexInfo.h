#pragma once

#include "HWFormats.h"
#include "ImageChannels.h"

namespace RayTrace
{
    struct HWTexInfo
    {
       
        uint32_t         m_id             = INVALID_ID;
        int              m_width          = 0,
                         m_height         = 0,
                         m_depth          = 1;
        bool             m_mips           = true; //gen mips?
        bool             m_internal       = false;
        bool             m_flipYOnRead    = false;
        bool             m_fbo            = false;
        float            m_maxAniso       = 8.0f;
        std::string      m_texName        = "Undefined";
        const char*      m_pData[6]       = {nullptr};
        std::size_t      m_bytesPerPixel  = 0;
        Vector4f         m_borderColor    = { 0, 0, 0, 0 };
        int              m_numChannels    = 0;
        ChannelMask      m_channels       = ChannelMask::CHANNEL_UNDEFINED;
       

        eTarget          m_target         = eTarget::TARGET_TEXTURE_2D;
        eWrapMode        m_wrapS          = eWrapMode::WRAP_MODE_REPEAT,
                         m_wrapT          = eWrapMode::WRAP_MODE_REPEAT,
                         m_wrapR          = eWrapMode::WRAP_MODE_REPEAT;                            
        eMinFilter       m_minFilter      = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;
        eMagFilter       m_magFilter      = eMagFilter::MAG_FILTER_LINEAR;      
        eTextureType     m_type           = eTextureType::TYPE_UNSIGNED_BYTE;
        eInternalFormat  m_internalFormat = eInternalFormat::INTERNAL_RGB;
        eFormat          m_format         = eFormat::FORMAT_RGB;   
       
    };

    eInternalFormat InternalFormatFromChannelCount_u8(int _numChannels);
    eInternalFormat InternalFormatFromChannelCount_f16(int _numChannels);
    eInternalFormat InternalFormatFromChannelCount_f32(int _numChannels);


    HWTexInfo TexInfoFromInternalFormt(eInternalFormat _format, eTarget _target = eTarget::TARGET_TEXTURE_2D, 
        eWrapMode _mode = eWrapMode::WRAP_MODE_REPEAT, bool _mips = true,
        eMinFilter _min = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR,
        eMagFilter _mag = eMagFilter::MAG_FILTER_LINEAR);
}