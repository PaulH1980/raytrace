#include "HWRenderer.h"
#include "HWTexture.h"
#include "HWFrameBuffer.h"

namespace RayTrace
{




    HWFrameBuffer* CreateFrameBuffer(Context* _pContext, 
        const std::vector<eInternalFormat>& _internalFormats, 
        const HWTexInfo& _sharedParams,
        bool _depthAsRbo /*= true*/)
    {
       
        assert(_sharedParams.m_target == eTarget::TARGET_TEXTURE_2D ||
               _sharedParams.m_target == eTarget::TARGET_TEXTURE_CUBE_MAP);

        

        std::vector<HWTexInfo> texInfos;
        for (const auto& internalFormat : _internalFormats)
        {
            const auto extendedInfo = GetFormatInfo(internalFormat);

            HWTexInfo info          = _sharedParams; //copy from params           
            info.m_internalFormat   = internalFormat;
            info.m_format           = extendedInfo.m_format;
            info.m_type             = extendedInfo.m_type;         
            info.m_bytesPerPixel    = extendedInfo.GetNumBytes();
            info.m_numChannels      = extendedInfo.m_numChannels;   
            info.m_depth            = _sharedParams.m_target == eTarget::TARGET_TEXTURE_2D ? 1 : 0;
            texInfos.push_back(info);
        }

        auto ret = new HWFrameBuffer(_pContext, texInfos, _depthAsRbo);
        return ret;
    }





    HWFrameBuffer::HWFrameBuffer(Context* _pContext, const std::vector<HWTexInfo>& _textures, bool _depthAsRbo)
        : ObjectBase( _pContext)
        , m_texInfos(_textures)
        , m_depthAsRbo(_depthAsRbo)
        , m_width(0)
        , m_height(0)
    {
        int w = 0;
        int h = 0;
       
        for (const auto& tex : _textures)
        {            
            w = std::max(w, tex.m_width );
            h = std::max(h, tex.m_height);
            m_target = tex.m_target;
        }        


        bool succeed = Resize(w, h);
        if (!succeed) {

            Clear();
            throw std::exception("Failed To Create FrameBuffer");
        }
    }

  

    bool HWFrameBuffer::Resize(int _width, int _height)
    {
        assert(_width >= MIN_FBO_DIMS && _height >= MIN_FBO_DIMS );

        if (_width < MIN_FBO_DIMS || _height < MIN_FBO_DIMS)
            return false;
        
        if (m_width == _width && m_height == _height)
            return true;

        m_width  = _width;
        m_height = _height;

        for (auto& tex : m_texInfos)
        {
            tex.m_width = m_width;
            tex.m_height = m_height;
        }

        Clear();
        return Create();
    }

    void HWFrameBuffer::Bind()
    {
        assert(m_bound == false);
        assert(m_width >= MIN_FBO_DIMS && m_height >= MIN_FBO_DIMS);

        if (m_fboId != INVALID_ID)
            glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        if (m_rboId != INVALID_ID)
            glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);

        m_bound = true;
    }


    void HWFrameBuffer::UnBind()
    {
        assert(m_bound == true);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if( m_rboId != INVALID_ID )
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        m_bound = false;
    }

    void HWFrameBuffer::AttachCubemapFace(int _faceIndex, int _colorIndex, int _mipLevel)
    {
        assert(m_bound == true && m_target == eTarget::TARGET_TEXTURE_CUBE_MAP );
        assert(_colorIndex < m_texInfos.size());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _colorIndex,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + _faceIndex, m_texInfos[_colorIndex].m_id, _mipLevel);
    }

    void HWFrameBuffer::Clear()
    {
        if (m_fboId != INVALID_ID)
            glDeleteFramebuffers(1, &m_fboId);

        if (m_rboId != INVALID_ID)
            glDeleteRenderbuffers(1, &m_rboId);

        m_rboId = INVALID_ID;
        m_fboId = INVALID_ID;
        
    
        m_textures.clear();

        for (auto& texInfo : m_texInfos)
            texInfo.m_id = INVALID_ID;
    }

    Vector2i HWFrameBuffer::GetSize() const
    {
        return Vector2i(m_width, m_height);
    }


   


    bool HWFrameBuffer::Create()
    {
        glGenFramebuffers(1, &m_fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId); 
       
        std::vector<HWTexInfo*> color;
        std::vector<HWTexInfo*> depth;

        for (auto& tex : m_texInfos)
        {
            tex.m_fbo = true;
            if (IsDepth(tex.m_format))
            {
                const auto depthSize = std::to_string(depth.size());
                tex.m_texName = "fbo_depth_0" + depthSize;
                
                depth.push_back(&tex);
            }
            if (IsColor(tex.m_format))
            {
                const auto colorSize = std::to_string(color.size());
                tex.m_texName = "fbo_color_0" + colorSize;
                color.push_back(&tex);
            }
        }

        assert( !(depth.empty() && color.empty()) );

        for (int i = 0; i < color.size(); ++i ) {
            auto texPtr = CreateTexture(m_pContext,  *color[i]);
            assert(texPtr);
            texPtr->Bind(0);
            color[i]->m_id = texPtr->m_texInfo.m_id;            
            m_textures.push_back(std::unique_ptr<HWTexture>(texPtr));
            if (texPtr->IsCube())
            {
                for( int face = 0; face < 6; face++ )
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texPtr->m_texInfo.m_id, 0);

            }
            else
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, (GLenum)texPtr->m_texInfo.m_target, texPtr->m_texInfo.m_id, 0);

            }
            texPtr->UnBind();
           
        }
        if (!depth.empty()) {
            assert(depth.size() == 1);

            auto& depthInfo = depth[0];
            int format = -1;
            if (IsDepthStencil(depthInfo->m_format))
            {
                format = GL_DEPTH_STENCIL_ATTACHMENT;
            }
            else if (IsDepth(depthInfo->m_format))
            {
                format = GL_DEPTH_ATTACHMENT;
            }
            else {
                   assert(false && "Unknown Depth/Stencil Format");                
            }

            if (m_depthAsRbo)
            {
                glGenRenderbuffers(1, &m_rboId);
                glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
                glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)depthInfo->m_internalFormat, m_width, m_height); 
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, format, GL_RENDERBUFFER, m_rboId );
                glBindRenderbuffer(GL_RENDERBUFFER, 0);         
            }
            else
            {
                auto texPtr = CreateTexture(m_pContext, *depthInfo);
                assert(texPtr);
                depthInfo->m_id = texPtr->m_texInfo.m_id;
                m_depthTexture.reset(texPtr);           
                
                if (texPtr->IsCube())
                {
                    for (int face = 0; face < 6; face++) {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, format, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texPtr->m_texInfo.m_id, 0);
                    }                
                }
                else
                {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, format, (GLenum)texPtr->m_texInfo.m_target, texPtr->m_texInfo.m_id, 0);
                }
            }            
        }
        const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            assert(false && "FrameBuffer InComplete");
            return false;
        }          
            
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        const auto error = glGetError();
        if (error != GL_NO_ERROR)
        {
            assert(false && "Opengl Error");
               return false;
        }
        
        return true;
    }

    bool HWFrameBuffer::IsBound() const
    {
        return m_bound;
    }
   

    bool HWFrameBuffer::Is2D() const
    {
        return m_target == eTarget::TARGET_TEXTURE_2D;
    }

    bool HWFrameBuffer::IsCube() const
    {
        return m_target == eTarget::TARGET_TEXTURE_CUBE_MAP;
    }

    const HWFrameBuffer::FrameBufferTextures& HWFrameBuffer::GetTextures() const
    {
        return m_textures;
    }

    const HWTextureUPtr& HWFrameBuffer::GetDepthTexture() const
    {
        return m_depthTexture;
    }
   

    HWTexture* HWFrameBuffer::GetTexture(int _idx) const
    {
        return m_textures[_idx].get();
    }

    HWFrameBuffer::FrameBufferTextures HWFrameBuffer::MoveTextures()
    {
        return std::move(m_textures);
    }

    HWTextureUPtr HWFrameBuffer::MoveDepthTexture()
    {
        return std::move(m_depthTexture);
    }

}

