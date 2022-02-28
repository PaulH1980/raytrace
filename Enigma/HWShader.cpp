#include <iostream>
#include "HWShader.h"

namespace RayTrace
{


    //////////////////////////////////////////////////////////////////////////
    //ShaderProgram Implementation
    //////////////////////////////////////////////////////////////////////////
    ShaderProgram::ShaderProgram(std::unique_ptr<Shader> _comp) : m_progamId(INVALID_ID)
        , m_isCompute(true)
    {
        m_shaders[0] = std::move(_comp);
        if (!AttachAndLink())
            throw std::exception("Failed To Create Program");
    }

    ShaderProgram::ShaderProgram(std::unique_ptr<Shader> _vert, std::unique_ptr<Shader> _frag)
        : m_progamId(INVALID_ID)
        , m_isCompute(false)
        , m_bound(false)
    {
        m_shaders[0] = std::move(_vert);
        m_shaders[1] = std::move(_frag);

        if (!AttachAndLink())
            throw std::exception("Failed To Create Program");
    }

    bool ShaderProgram::AttachAndLink()
    {
        m_progamId = glCreateProgram();
        assert(m_progamId != INVALID_ID);
        for (const auto& shader : m_shaders) {
            if (shader)
                glAttachShader(m_progamId, shader->GetShaderId());
        }
        glLinkProgram(m_progamId);

        for (const auto& shader : m_shaders) {
            if (shader) glDetachShader(m_progamId, shader->GetShaderId());
        }
        GLint status = 0;
        glGetProgramiv(m_progamId, GL_LINK_STATUS, (int*)&status);
        return GL_FALSE != status;
    }

    void ShaderProgram::Bind()
    {
        assert(m_bound == false);
        glUseProgram(m_progamId);
        m_bound = true;
    }

    void ShaderProgram::Dispatch(uint32_t _x, uint32_t _y, uint32_t _z)
    {
        assert(m_bound == true);
        glDispatchCompute(_x, _y, _z);
    }

    void ShaderProgram::Dispatch(uint32_t _x, uint32_t _y)
    {
        Dispatch(_x, _y, 1);
    }

    void ShaderProgram::Dispatch(uint32_t _x)
    {
        Dispatch(_x, 1);
    }

    void ShaderProgram::DispatchIndirecet(ptrdiff_t _v)
    {
        assert(m_bound == true);
        glDispatchComputeIndirect(_v);
    }

    void ShaderProgram::UnBind()
    {
        assert(m_bound == true);
        glUseProgram(0);
        m_bound = false;
    }

    void ShaderProgram::SetUint(const char* _name, const uint32_t* _v, uint32_t _count)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform1uiv(loc, _count, _v);
    }

    void ShaderProgram::SetFloat(const char* _name, const float* _v, uint32_t _count /*= 1*/)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform1fv(loc, _count, _v);
    }

    void ShaderProgram::SetInt(const char* _name, const int* _v, uint32_t _count /*= 1*/)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform1iv(loc, _count, _v);
    }

    void ShaderProgram::SetMatrix4x4(const char* _name, const Matrix4x4* _v, uint32_t _count /*= 1 */, bool _transposed)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;

        glUniformMatrix4fv(loc, _count, _transposed, (const float*)_v);
    }

    void ShaderProgram::SetVec2f(const char* _name, const Vector2f* _v, uint32_t _count /*= 1 */)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform2fv(loc, _count, (const float*)_v);
    }

    void ShaderProgram::SetVec3f(const char* _name, const Vector3f* _v, uint32_t _count /*= 1 */)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform3fv(loc, _count, (const float*)_v);
    }

    void ShaderProgram::SetVec4f(const char* _name, const Vector4f* _v, uint32_t _count /*= 1 */)
    {
        const auto loc = GetLocation(_name);
        if (loc == INVALID_ID)
            return;
        glUniform4fv(loc, _count, (const float*)_v);
    }

    int ShaderProgram::GetLocation(const char* _name) const
    {
        assert(m_bound);
        return glGetUniformLocation(m_progamId, _name);
    }


    //////////////////////////////////////////////////////////////////////////
    //Shader Implementation
    //////////////////////////////////////////////////////////////////////////
    Shader::Shader(const std::string& _code, eShaderType _type)
        : m_shaderSource(_code)
        , m_type(_type)
        , m_shaderId(INVALID_ID)
    {
        if (!CreateAndCompile())
            throw std::exception("Failure Compiling Shader");
    }



    bool Shader::CreateAndCompile()
    {
        int glType = INVALID_ID;
        switch (m_type)
        {
        case  eShaderType::SHADER_TYPE_VERT:
            glType = GL_VERTEX_SHADER;
            break;
        case eShaderType::SHADER_TYPE_FRAG:
            glType = GL_FRAGMENT_SHADER;
            break;
        case eShaderType::SHADER_TYPE_COMPUTE:
            glType = GL_COMPUTE_SHADER;
            break;
        default:
            throw std::exception("Invalid Shader Type");
        }

        m_shaderId = glCreateShader(glType);
        assert(IsValid());

        const char* pTxt = m_shaderSource.c_str();
        glShaderSource(m_shaderId, 1, &pTxt, nullptr);
        glCompileShader(m_shaderId);
        GLint status = 0;
        glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            GetErrorString();
        return status != GL_FALSE;
    }

    bool Shader::IsValid() const
    {
        return m_shaderId != INVALID_ID;
    }

    void Shader::Clear()
    {
        if (IsValid())
            glDeleteShader(m_shaderId);
    }

    uint32_t Shader::GetShaderId() const
    {
        return m_shaderId;
    }

    std::string Shader::GetErrorString() const
    {
        GLint maxLength = 0;
        glGetShaderiv(m_shaderId, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        std::string ret;
        ret.resize(maxLength);
        glGetShaderInfoLog(m_shaderId, maxLength, &maxLength, ret.data());
#if _DEBUG
        std::cout << ret << std::endl;
#endif
        return ret;
    }
}

