#pragma once

#include "HWFormats.h"
#include "Transform.h"
#include "HWTexInfo.h"
#include "ObjectBase.h"

namespace RayTrace
{

    HWTexture*  CreateTexture(Context* _pContext,  HWTexInfo _info);
    HWTexture*  CreateTexture(Context* _pContext, const ImageExInfo& _info, HWTexInfo _params);
    HWTexture*  CreateTexture(Context* _pContext, const std::string& _fileName, HWTexInfo _params );
    HWTexture*  CreateTextureCube(Context* _pContext, const std::array<std::string, 6>& _fileNames, HWTexInfo _params);

    U8Vector    GetTextureData( HWTexture* _pTexture, int mipLevel = 0);

    bool        SaveTexture(const std::string& _fileName, const U8Vector& _texData, const HWTexInfo& _ti, bool _flipY = false, int mipLevel = 0);
    bool        SaveTexture(const std::string& _fileName, HWTexture* pTex, bool _flipY = false, int _mipLevel = 0);


    class HWTexture : public ObjectBase
    {
        RT_OBJECT( HWTexture, ObjectBase )


    public:


        HWTexture( Context* _pContext, HWTexInfo _info);

        virtual ~HWTexture()
        {
            Clear();
        }


        HWTexture*      Clone();

        void            Clear();
        uint32_t        Bind(uint32_t texUnit);
        void            UnBind();
        void            Update(const void* _pData, const Vector3i& offset, const Vector3i& _data, int _level = 0);

        bool            IsCube() const;
        bool            Is1D() const;
        bool            Is2D() const;
        bool            Is3D() const;

        int             Width() const { return m_texInfo.m_width; }
        int             Depth() const { return m_texInfo.m_depth; }
        int             Height() const { return m_texInfo.m_height; }

   

        HWTexInfo   m_texInfo;
        uint32_t    m_texUnit = INVALID_ID;


    };

}