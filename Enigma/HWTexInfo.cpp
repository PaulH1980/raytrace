#include "HWTexInfo.h"

namespace RayTrace
{

    eInternalFormat InternalFormatFromChannelCount_u8(int _numChannels)
    {
        if (_numChannels == 1)
            return eInternalFormat::INTERNAL_R8;
        else if (_numChannels == 2)
            return eInternalFormat::INTERNAL_RG8;
        else if (_numChannels == 3)
            return eInternalFormat::INTERNAL_RGB8;
        else if (_numChannels == 4)
            return eInternalFormat::INTERNAL_RGBA8;
        else
            throw std::exception("Invalid Channel Count");
    }

    eInternalFormat InternalFormatFromChannelCount_f16(int _numChannels)
    {
        if (_numChannels == 1)
            return eInternalFormat::INTERNAL_R16F;
        else if (_numChannels == 2)
            return eInternalFormat::INTERNAL_RG16F;
        else if (_numChannels == 3)
            return eInternalFormat::INTERNAL_RGB16F;
        else if (_numChannels == 4)
            return eInternalFormat::INTERNAL_RGBA16F;
        else
            throw std::exception("Invalid Channel Count");
    }

    eInternalFormat InternalFormatFromChannelCount_f32(int _numChannels)
    {
        if (_numChannels == 1)
            return eInternalFormat::INTERNAL_R32F;
        else if (_numChannels == 2)
            return eInternalFormat::INTERNAL_RG32F;
        else if (_numChannels == 3)
            return eInternalFormat::INTERNAL_RGB32F;
        else if (_numChannels == 4)
            return eInternalFormat::INTERNAL_RGBA32F;
        else
            throw std::exception("Invalid Channel Count");
    }

    HWTexInfo TexInfoFromInternalFormt(eInternalFormat _format, eTarget _target /*= eTarget::TARGET_TEXTURE_2D*/, eWrapMode _mode /*= eWrapMode::WRAP_MODE_REPEAT*/, bool _mips /*= true*/, eMinFilter _min /*= eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR*/, eMagFilter _mag /*= eMagFilter::MAG_FILTER_LINEAR*/)
    {
        auto formatInfo = GetFormatInfo(_format);

        HWTexInfo _info;
        _info.m_internalFormat = _format;
        _info.m_target = _target;
        _info.m_format = formatInfo.m_format;
        _info.m_type = formatInfo.m_type;
        _info.m_bytesPerPixel = formatInfo.GetNumBytes();
        _info.m_depth = 1;
        _info.m_mips = _mips;
        _info.m_minFilter = _min;
        _info.m_magFilter = _mag;
        _info.m_wrapR = _info.m_wrapS = _info.m_wrapT = _mode;
        return _info;
    }

}

