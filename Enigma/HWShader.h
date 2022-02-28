#pragma once
#include "HWFormats.h"
#include "Transform.h"
#include "HWTexInfo.h"

namespace RayTrace
{

    class Shader
    {
    public:
        enum eShaderType
        {
            SHADER_TYPE_VERT,
            SHADER_TYPE_FRAG,
            SHADER_TYPE_COMPUTE
        };

        Shader(const std::string& _code, eShaderType _type);

        ~Shader() {
            Clear();
        }

        bool                CreateAndCompile();
        bool                IsValid() const;
        void                Clear();
        uint32_t            GetShaderId() const;
        std::string         GetErrorString() const;

    public:
        std::string			m_shaderSource;
        uint32_t			m_shaderId;
        eShaderType			m_type;
    };

    class ShaderProgram
    {
    public:
        ShaderProgram(std::unique_ptr<Shader> _vert, std::unique_ptr<Shader> _frag);
        ShaderProgram(std::unique_ptr<Shader> _comp);

        bool AttachAndLink();

        void Bind();
        //Compute
        void Dispatch(uint32_t _x);
        void Dispatch(uint32_t _x, uint32_t _y);
        void Dispatch(uint32_t _x, uint32_t _y, uint32_t _z);
        void DispatchIndirecet(ptrdiff_t _v);

        void UnBind();
        void SetUint(const char* _name, const uint32_t* _v, uint32_t count = 1);
        void SetFloat(const char* _name, const float* _v, uint32_t count = 1);
        void SetInt(const char* _name, const int* _v, uint32_t _count = 1);
        void SetMatrix4x4(const char* _name, const Matrix4x4* _v, uint32_t _count = 1, bool _transposed = false);
        void SetVec2f(const char* _name, const Vector2f* _v, uint32_t _count = 1);
        void SetVec3f(const char* _name, const Vector3f* _v, uint32_t _count = 1);
        void SetVec4f(const char* _name, const Vector4f* _v, uint32_t _count = 1);

        int  GetLocation(const char* _name) const;

        uint32_t				m_progamId = { 0 };
        uint32_t				m_work[3] = { 0 };
        bool					m_isCompute = { false };
        bool                    m_bound = { false };
        std::unique_ptr<Shader> m_shaders[2];


    };

}