#include <gli/gli/gli.hpp>

#include "HWVertexBuffer.h"
#include "stb_image_write.h"
#include "ImageChannels.h"
#include "IO.h"
#include "FilePaths.h"
#include "HWTexture.h"

namespace RayTrace
{

   

    HWTexture* CreateTexture(Context* _pContext, HWTexInfo _info)
    {
         return new HWTexture(_pContext, _info);
    }

    HWTexture* CreateTexture(Context* _pContext, const std::string& _fileName, HWTexInfo _params)
    {
        ImageExInfo info;
        info.m_readFlipped = _params.m_flipYOnRead;
        if (!ReadImage(_fileName, info)) {
            AddLogMessage(_pContext, fmt::format("Failed to read texture {}", _fileName), eLogLevel::LOG_LEVEL_WARNING);
            return nullptr;
        }
       
        auto texPtr = CreateTexture(_pContext, info, _params);
        if (texPtr)
            AddLogMessage(_pContext, fmt::format("Created Texture {}", _fileName), eLogLevel::LOG_LEVEL_INFO);
        return texPtr;
    }


    HWTexture* CreateTexture(Context* _pContext, const ImageExInfo& _info, HWTexInfo _params)
    {
      
        auto exInfo = GetFormatInfo(_info.m_internalFormat);

        _params.m_width = _info.m_width;
        _params.m_height = _info.m_height;
        _params.m_depth = _info.m_depth;
        _params.m_internalFormat = _info.m_internalFormat;
        _params.m_channels = _info.m_channels;
        _params.m_numChannels = _info.m_numChannels;
        _params.m_type = exInfo.m_type;
        _params.m_pData[0] = (const char*)_info.m_rawData.data();
        _params.m_format = exInfo.m_format;
        _params.m_bytesPerPixel = exInfo.GetNumBytes();

        auto texPtr = CreateTexture(_pContext, _params);
        if (!texPtr)
            AddLogMessage(_pContext, "Failed To Create Texture", eLogLevel::LOG_LEVEL_WARNING);
        return texPtr;
    }

    HWTexture* CreateTextureCube(Context* _pContext, const std::array<std::string, 6>& _fileNames, HWTexInfo _params)
    {
        for (auto i = 0; i < 6; ++i) {
            if (!Exists(_fileNames.at(i)))
                return nullptr;
        }
        
        ImageExInfo info[6];
        bool firstTime = true;
        for (int i = 0; i < 6; ++i)
        {
           if (!ReadImage(_fileNames.at(i), info[i] ) )
                return nullptr;

            auto exInfo = GetFormatInfo(info[i].m_internalFormat);
            if (firstTime) {
                _params.m_width          = info[i].m_width;
                _params.m_height         = info[i].m_height;
                _params.m_internalFormat = info[i].m_internalFormat;
                _params.m_channels       = info[i].m_channels;
                _params.m_numChannels    = info[i].m_numChannels;
                _params.m_type           = exInfo.m_type;                
                _params.m_format         = exInfo.m_format;
                _params.m_bytesPerPixel  = exInfo.GetNumBytes();
                firstTime = false;
            }
            else
            {//verify 
                if( (_params.m_width             !=  info[i].m_width) ||
                    (_params.m_height            !=  info[i].m_height) ||
                    (_params.m_internalFormat    !=  info[i].m_internalFormat) ||
                    (_params.m_channels          !=  info[i].m_channels) ||
                    (_params.m_numChannels       !=  info[i].m_numChannels) ||
                    (_params.m_type              !=  exInfo.m_type) ||         
                    (_params.m_format            !=  exInfo.m_format) ||
                    (_params.m_bytesPerPixel     !=  exInfo.GetNumBytes()) )
                {
                    AddLogMessage(_pContext, "Cubemap Dimensions Mismatch", eLogLevel::LOG_LEVEL_WARNING);
                }
            }

            _params.m_pData[i] = (const char*)info[i].m_rawData.data();
        }

        return CreateTexture(_pContext, _params);
    }

    U8Vector GetTextureData( HWTexture* _pTexture, int _mipLevel /*= 0*/)
    {
        const auto& ti = _pTexture->m_texInfo;
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        int width = ti.m_width >> _mipLevel;
        int height = ti.m_height >> _mipLevel;
        std::vector<uint8_t> pixels(width * height * ti.m_bytesPerPixel);
        _pTexture->Bind(0);
        glGetTexImage((GLenum)ti.m_target, _mipLevel, (GLenum)ti.m_format, (GLenum)ti.m_type, pixels.data());
        _pTexture->UnBind();
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        return pixels;
    }

    bool SaveTexture(const std::string& _fileName, const U8Vector& _texData, const HWTexInfo& _ti, bool _flipY, int _mipLevel)
    {
        int width  = _ti.m_width >> _mipLevel;
        int height = _ti.m_height >> _mipLevel;
        const auto formatInfo      = GetFormatInfo(_ti.m_internalFormat);
        const auto numChannels     = formatInfo.m_numChannels;

       
        ImageExInfo image;
        image.m_numChannels = numChannels;
        image.m_internalFormat      = _ti.m_internalFormat;
        image.m_writeFlipped = _flipY;
        image.m_width  = _ti.m_width >> _mipLevel;
        image.m_height = _ti.m_height >> _mipLevel;
        image.m_type   = _ti.m_type;     
        

        
        if (IsHalfFloat(_ti.m_type))
        {
            const short* texDataS16 = (const short*)_texData.data();
            
            FloatVector f32Data;
          
            for (int i = 0; i < width * height * numChannels ; i += numChannels )
            {
                for (int ch = 0; ch < numChannels; ch++)
                {
                    f32Data.push_back(glm::detail::toFloat32(texDataS16[i + ch]));
                }
            }            
            image.m_rawData.resize(width * height * numChannels * sizeof(float));
            memcpy(image.m_rawData.data(), f32Data.data(), image.m_rawData.size());

            std::string fileNameExt = _fileName +  ".hdr";
            return WriteImageHDR(fileNameExt, image);        
        }
        else if (IsFloatingPoint(_ti.m_type))
        {
            image.m_rawData = _texData;
            std::string fileNameExt = _fileName + ".hdr";      
            return WriteImageHDR(fileNameExt, image);
        }        
        else if (IsByte(_ti.m_type))
        {
            image.m_rawData = _texData;
            std::string fileNameExt = _fileName + ".png";
            return WriteImagePNG(fileNameExt, image);
        }
        else if (IsInteger(_ti.m_type))
        {

        }
        return false;

    }

    bool SaveTexture(const std::string& _fileName,  HWTexture* pTex, bool _flipY, int _mipLevel /*= 0*/)
    {
        auto texData = GetTextureData(pTex, _mipLevel);
        return SaveTexture(_fileName, texData, pTex->m_texInfo, _flipY, _mipLevel);
    }

    HWTexture::HWTexture(Context* _pContext, HWTexInfo _info)
        : ObjectBase( _pContext )
        , m_texInfo(_info)
    {
        if (!m_texInfo.m_mips)
        {
            if (m_texInfo.m_minFilter != eMinFilter::MIN_FILTER_LINEAR ||
                m_texInfo.m_minFilter != eMinFilter::MIN_FILTER_NEAREST) {
                m_texInfo.m_minFilter  = eMinFilter::MIN_FILTER_LINEAR;
            }
        }

        glGenTextures(1, &m_texInfo.m_id); assert(m_texInfo.m_id != INVALID_ID);
        glBindTexture((int)m_texInfo.m_target, m_texInfo.m_id);

        glTexParameterfv((int)m_texInfo.m_target, GL_TEXTURE_BORDER_COLOR,  glm::value_ptr( m_texInfo.m_borderColor));
        glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_MIN_FILTER, (int)m_texInfo.m_minFilter);
        glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_MAG_FILTER, (int)m_texInfo.m_magFilter);
        glTexParameterf((int)m_texInfo.m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_texInfo.m_maxAniso);


        if (Is1D())
        {
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_S, (int)m_texInfo.m_wrapS);
            glTexImage1D(
                (int)m_texInfo.m_target,
                0, //level
                (int)m_texInfo.m_internalFormat,
                m_texInfo.m_width,
                0,
                (int)m_texInfo.m_format,
                (int)m_texInfo.m_type,
                m_texInfo.m_pData[0]
            );

        }
        else if (Is2D())
        {
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_S, (int)m_texInfo.m_wrapS);
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_T, (int)m_texInfo.m_wrapT);

            glTexImage2D(
                (int)m_texInfo.m_target,
                0, //level
                (int)m_texInfo.m_internalFormat,
                m_texInfo.m_width,
                m_texInfo.m_height,
                0,
                (int)m_texInfo.m_format,
                (int)m_texInfo.m_type,
                m_texInfo.m_pData[0]
            );

        }
        else if (Is3D())
        {
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_S, (int)m_texInfo.m_wrapS);
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_T, (int)m_texInfo.m_wrapT);
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_R, (int)m_texInfo.m_wrapR);

            glTexImage3D(
                (int)m_texInfo.m_target,
                0, //level
                (int)m_texInfo.m_internalFormat,
                m_texInfo.m_width,
                m_texInfo.m_height,
                m_texInfo.m_depth,
                0,
                (int)m_texInfo.m_format,
                (int)m_texInfo.m_type,
                m_texInfo.m_pData[0]
            );
        }
        else if (IsCube())
        {
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_S, (int)m_texInfo.m_wrapS);
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_T, (int)m_texInfo.m_wrapT);
            glTexParameteri((int)m_texInfo.m_target, GL_TEXTURE_WRAP_R, (int)m_texInfo.m_wrapR);
            std::size_t offset = m_texInfo.m_bytesPerPixel * m_texInfo.m_width * m_texInfo.m_height;
            for (int i = 0; i < 6; ++i)
            {
                const char* pData = m_texInfo.m_pData[i];
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, //level
                    (int)m_texInfo.m_internalFormat,
                    m_texInfo.m_width,
                    m_texInfo.m_height,
                    0,
                    (int)m_texInfo.m_format,
                    (int)m_texInfo.m_type,
                    pData
                );
            }
        }
        if (m_texInfo.m_mips)
            glGenerateMipmap((int)m_texInfo.m_target);
        glBindTexture((int)m_texInfo.m_target, 0);

        for (int i = 0; i < 6; ++i)
            m_texInfo.m_pData[i] = nullptr;

        if (glGetError() != GL_NO_ERROR)
            throw std::exception("Texture Creation Failure");


        auto info = GetFormatInfo(m_texInfo.m_internalFormat);
        m_texInfo.m_bytesPerPixel = info.GetNumBytes();
    }


    HWTexture* HWTexture::Clone()
    {
        
        HWTexture* copy = CreateTexture( m_pContext, m_texInfo);

        const auto& _src = m_texInfo;
        const auto& _dst = copy->m_texInfo;  

        int depth = IsCube() ? 6 : _src.m_depth;

        glCopyImageSubData(_src.m_id, (int) _src.m_target, 0, 0, 0, 0,
                           _dst.m_id, (int) _dst.m_target, 0, 0, 0, 0,
                           _src.m_width, _src.m_height, _src.m_depth );
        assert(glGetError() == GL_NO_ERROR);
        return copy;
    }

    void HWTexture::Clear()
    {
        if (m_texInfo.m_id != INVALID_ID)
            glDeleteTextures(1, &m_texInfo.m_id);
        m_texInfo.m_id = INVALID_ID;
    }

    uint32_t HWTexture::Bind(uint32_t _texUnit)
    {
        assert(m_texUnit == INVALID_ID);
        glActiveTexture(GL_TEXTURE0 + _texUnit);
        glBindTexture((int)m_texInfo.m_target, m_texInfo.m_id);
        m_texUnit = _texUnit;
        return _texUnit + 1;
    }

    void HWTexture::UnBind()
    {
        assert(m_texUnit != INVALID_ID);
        m_texUnit = INVALID_ID;
        glBindTexture((int)m_texInfo.m_target, 0);
    }

    void HWTexture::Update(const void* _pData, const Vector3i& offset, const Vector3i& _data, int _level /*= 0 */)
    {
        if (Is1D())
        {

        }
        else if (Is2D())
        {

        }
        else if (Is3D())
        {

        }
        else if (IsCube())
        {

        }
    }

    bool HWTexture::IsCube() const
    {
        return m_texInfo.m_depth == 0;
    }

    bool HWTexture::Is1D() const
    {
        return m_texInfo.m_height == 1;
    }

    bool HWTexture::Is2D() const
    {
        return m_texInfo.m_width > 1 &&
            m_texInfo.m_height > 1 &&
            m_texInfo.m_depth != 0;
    }

    bool HWTexture::Is3D() const
    {
        return m_texInfo.m_depth > 1;
    }

}

