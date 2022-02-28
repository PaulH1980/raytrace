#include "HWShader.h"
#include "HWTexture.h"
#include "HWMaterial.h"

namespace RayTrace
{

    HWMaterial::HWMaterial()
    {
        for (auto i = 0; i < NUM_EDITOR_SLOTS; ++i)
            m_useConstant[i] = false;
    }

    HWMaterial::~HWMaterial()
    {

    }

    void HWMaterial::Bind(ShaderProgram* _pActiveShader)
    {
        assert(m_bound == false);
        int _texId = 0;
        uint32_t flags = 0;
        for (int i = 0; i < NUM_TEX; ++i)
        {
            auto tex = m_textures[i];
            
            if (tex) {
                int nextId = tex->Bind(_texId);
                _pActiveShader->SetInt( TextureNames[i].data(), &_texId);
                _texId = nextId;
                flags |= 1 << i;
            }         
        }
        _pActiveShader->SetUint("hasTextureFlag", &flags, 1);
        m_bound = true;
    }

    void HWMaterial::UnBind()
    {
        assert(m_bound == true);
        for (int i = 0; i < NUM_TEX; ++i)
            if (m_textures[i])
                m_textures[i]->UnBind();

        m_bound = false;
    }

    int HWMaterial::NumActiveTextures() const
    {
        int count = 0;
        for (int i = 0; i < NUM_TEX; ++i)
            if (m_textures[i] != nullptr)
                count++;
        return count;
    }

    bool HWMaterial::HasAlphaMask() const
    {
        if (m_textures[ALBEDO] == nullptr)
            return false;
        if (m_textures[ALBEDO]->m_texInfo.m_internal)
            return false;

        return m_textures[ALBEDO]->m_texInfo.m_bytesPerPixel == 4;
    }


}

