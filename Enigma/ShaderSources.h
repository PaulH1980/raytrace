#pragma once

namespace RayTrace
{
    static const char* GLSL_VERSION = "#version 430\n\n";

    static const char* GLSL_BRDF_LUT = "#define BRDF_LUT\n\n";

    static const char* GLSL_SHARED = 
R"(   

layout(std140, binding = 1) uniform PerLayer
{
    mat4 m_proj;
    mat4 m_view;
    mat4 m_skyView;
    mat4 m_projInv;
    mat4 m_viewInv;
    mat4 m_ortho2dProj;
    mat4 m_ortho2dView;
    vec4 m_cameraPos;
    vec4 m_cameraRight;
    vec4 m_cameraUp;
    vec4 m_cameraForward;
    vec4 m_viewport; //width, height, invWidth, invHeight  
    vec4 m_frustInfo; //near, far, pad, pad 
    vec4 m_time;     //game time, delta time, frameNumber, pad   
    vec4 m_lightDir;   //sunlight dir + pad
    vec4 m_lightColor; //sunlight rgb intenity
    vec4 m_selectionColor;
    
} g_perLayer;

uniform int  isSelected     = 0;
uniform uint hasTextureFlag = 0;

uniform sampler2D    tex_albedo;  		//alpha channel used as opacity mask
uniform sampler2D    tex_normal;   
uniform sampler2D    tex_ao_rough_metal;  
uniform sampler2D    tex_emission;

uniform sampler2D    tex_misc0;
uniform sampler2D    tex_misc1;
uniform sampler2D    tex_misc2;  
uniform sampler2D    tex_misc3;

uniform sampler2D    tex_brdfLUT;		//pre integration table
uniform samplerCube  tex_irradiance;
uniform samplerCube  tex_prefilterMap;	
uniform samplerCube  tex_env;

uniform vec4 const_albedo   		= vec4( 0.5, 0.5, 0.5, 1.0 );
uniform vec3 const_normal			= vec3( 0.5, 0.5, 1.0);
uniform vec3 const_ao_rough_metal	= vec3( 1.0, 0.5, 0.0);  
uniform vec3 const_emission			= vec3( 0.0, 0.0, 0.0);

const uint albedoFlags 			=  0x01;  		//alpha channel used as opacity mask
const uint normalFlags 			=  0x02;   
const uint ao_rough_metalFlags	=  0x04;   
const uint emissionFlags		=  0x08;   
 
const uint misc0Flags 			=  0x10;  		
const uint misc1Flags 			=  0x20;   
const uint misc2Flags			=  0x40;   
const uint misc3Flags			=  0x80; 

const uint irradianceFlags		=  0x100;   
const uint prefilterMapFlags	=  0x200;   	
const uint brdfLUTFlags			=  0x400; 
const uint envFlags			    =  0x800;   


float GetWidth()
{
     return g_perLayer.m_viewport.x;
}

float GetHeight()
{
     return g_perLayer.m_viewport.y;
}

float GetInvWidth()
{
     return g_perLayer.m_viewport.z;
}

float GetInvHeight()
{
     return g_perLayer.m_viewport.w;
}


float GetGameTime(){
    return g_perLayer.m_time.x;
}

float GetDeltaTime(){
    return g_perLayer.m_time.y;
}

float GetFrameNumber(){
    return g_perLayer.m_time.z;
}

float GetNear()
{
    return g_perLayer.m_frustInfo.x;
}

float GetFar()
{
    return g_perLayer.m_frustInfo.y;
}


float GetSelectionScale()
{
    if( isSelected != 0 )
    {
        return abs( sin( GetGameTime() * 2.0 ) * 0.5 ) + 0.5;
    }
    return 1.0;    
}

vec4 GetSelectionColor()
{
    if( isSelected != 0 )
    {
        vec3 color = g_perLayer.m_selectionColor.rgb * GetSelectionScale();   
        return vec4(color, GetSelectionScale() * 0.5);
    }
    return vec4(0.0);    
}

)";
       

static const char* GLSL_FUNCTIONS =
R"(
const vec2 invAtan = vec2(0.1591, 0.3183);
const float PI = 3.14159265359;


vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

vec3 Gamma(vec3 colorIn)
{
    vec3 color = colorIn / (colorIn + vec3(1.0));
    return pow(color, vec3(1.0 / 2.2));
}


float DistributionGGX(float NDotH, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(NDotH, 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}



#ifdef BRDF_LUT
float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
#else

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

#endif

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   


// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

)";




    inline std::string AssembleShaderCode( const std::string& _shader, const std::vector<std::string>& prepend)
    {
        std::string result;
        for (const auto& str : prepend)
            result += str;

        result += _shader;
        return result;
    }

}