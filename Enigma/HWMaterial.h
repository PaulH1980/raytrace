#pragma once
#include "FrontEndDef.h"
#include "Color.h"
namespace RayTrace
{
   
    enum eTextureTypes
    {
        
        UNKNOWN = -1,
        
        ALBEDO             = 0,
        NORMAL             = 1,
        AO_ROUGH_METAL     = 2, //combined
        EMISSION           = 3,       
        TEX_MISC_0         = 4,
        TEX_MISC_1         = 5,
        TEX_MISC_2         = 6,
        TEX_MISC_3         = 7,
        NUM_EDITOR_SLOTS   = TEX_MISC_3,
        
        BRDF_LUT           = 8, //brdf look up table
        IRRADIANCE         = 9, //irradiance cube
        PREFILTER          = 10, //pre filtered cube
        ENV                = 11, //environment cube
       

        NUM_TEX
    };

    static std::string TextureNames[NUM_TEX] = {
        "tex_albedo"            ,
        "tex_normal"            ,
        "tex_ao_rough_metal"    ,
        "tex_emission"          ,           
        "tex_mics0"             ,
        "tex_misc1"             ,
        "tex_misc2"             ,
        "tex_misc3"             ,
        "tex_brdfLUT"           ,
        "tex_irradiance"        ,
        "tex_prefilterMap"      ,
        "tex_env"
    };
    
    struct MaterialConstants
    {
        Color4f m_rgba   = { 0.5f, 0.5f, 0.5f, 1.0f };
        Color3f m_normal = { 0.5f, 0.5f, 1.f };
        Color3f m_ao_rough_metal = { 1.f, 0.f, 0.f };
        Color3f m_emission = { 0.f, 0.f, 0.f };
    };

    
    
    class HWMaterial
    {
    public:

        HWMaterial();
        ~HWMaterial();

        void                Bind(ShaderProgram* _pActiveShader);
        void                UnBind();
        int                 NumActiveTextures() const;
        bool                HasAlphaMask() const;
     

        Vector4f            m_baseColorFactor = {1.f, 1.f, 1.f, 1.f};
        float               m_metallicFacctor = 1.0f;
        float               m_roughnessFactor = 1.0f;
        HWTexture*          m_textures[NUM_TEX] = { nullptr };
        Vector4f            m_constants[NUM_TEX];
        bool                m_useConstant[NUM_TEX] = { true };
        bool                m_bound             = false;       
    };
}