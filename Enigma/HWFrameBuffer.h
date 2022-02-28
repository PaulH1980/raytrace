#pragma once
#include "FrontEndDef.h"
#include "HWFormats.h"
#include "HWTexInfo.h"
#include "ObjectBase.h"

namespace RayTrace
{
    static const int MIN_FBO_DIMS = 2;

    /*
        @brief: Create a 2d fbo, if  'depthAsRbo' is false, there can only be 1 depth attachment
    */
    HWFrameBuffer* CreateFrameBuffer(Context* _pContext,
        const std::vector<eInternalFormat>& _internalFormats,
        const HWTexInfo& _sharedParams = HWTexInfo(),
        bool _depthAsRbo = true );

   
    

    class HWFrameBuffer : public ObjectBase
    {

        RT_OBJECT(HWFrameBuffer, ObjectBase)

    public:
        using FrameBufferTextures = std::vector<std::unique_ptr<HWTexture>>;

        /*
            @brief: 2D Constructor
        */
        HWFrameBuffer(Context* _pContext, const std::vector<HWTexInfo>& _texInfo, bool _depthAsRbo = true);
        virtual ~HWFrameBuffer() {
            HWFrameBuffer::Clear();
        };

        virtual bool    Resize(int _width, int _height);

        virtual void    Bind();

        virtual void    UnBind();

        virtual void    AttachCubemapFace( int _faceIndex, int _colorIndex = 0, int _mipLevel = 0 );

        virtual void    Clear();

        Vector2i        GetSize() const;

        bool            IsBound() const;

        bool            Is2D() const;
        bool            IsCube() const;

        const  FrameBufferTextures&         GetTextures() const;      
        const  HWTextureUPtr&               GetDepthTexture() const;

        HWTexture*      GetTexture( int _idx ) const;

        //Move Operations to retrieve textures, after calls to these the fbo can no longer be used
        FrameBufferTextures                 MoveTextures();
        HWTextureUPtr                       MoveDepthTexture();
      
        
        int             GetWidth() const { return m_width; }
        int             GetHeight() const { return  m_height; }
    private:

        bool            Create(); 
       

        std::vector<HWTexInfo>      m_texInfos;  
    
        eTarget                     m_target =   eTarget::TARGET_TEXTURE_UNDEFINED;
        
        FrameBufferTextures         m_textures;
        HWTextureUPtr               m_depthTexture;
        uint32_t                    m_fboId = INVALID_ID;       //framebuffer object
        uint32_t                    m_rboId = INVALID_ID;       //renderbuffer object
        int                         m_width  = MIN_FBO_DIMS,
                                    m_height = MIN_FBO_DIMS;

        bool                        m_depthAsRbo = true;
        bool                        m_bound      = false;


       
      
    };
}