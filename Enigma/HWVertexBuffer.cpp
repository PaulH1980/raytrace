
#include "HWVertexBuffer.h"

namespace RayTrace
{

    Vec2fBuffer* CreateVec2fBuffer(const Vector2f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage)
    {
        Vec2fBuffer* ret = new Vec2fBuffer(_pData, _count, eVertexBufferTarget::VERTEX_BUFFER_VERTEX, _usage ) ;
        ret->m_info = { _attId, 2, GL_FLOAT, 0, false };
        return ret;
    }

    Vec3fBuffer* CreateVec3fBuffer(const Vector3f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage)
    {
        Vec3fBuffer* ret = new Vec3fBuffer(_pData, _count, eVertexBufferTarget::VERTEX_BUFFER_VERTEX, _usage);
        ret->m_info = { _attId, 3, GL_FLOAT, 0, false };
        return ret;
    }

    Vec4fBuffer* CreateVec4fBuffer(const Vector4f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage)
    {
        Vec4fBuffer* ret = new Vec4fBuffer(_pData, _count, eVertexBufferTarget::VERTEX_BUFFER_VERTEX, _usage);
        ret->m_info = { _attId, 4, GL_FLOAT, 0, false };
        return ret;
    }

    IndexBuffer* CreateIndexBuffer(const uint32_t* _pData, uint32_t _count)
    {
        IndexBuffer* ret = new IndexBuffer(_pData, _count, VERTEX_BUFFER_INDEX);
        return ret;
    }



    bool IsDynamic(eVertexBufferUsage _usage)
    {
        return _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_COPY ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_READ ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_DRAW;
    }

    bool IsStream(eVertexBufferUsage _usage)
    {
        return _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STREAM_COPY ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STREAM_READ ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STREAM_DRAW;
    }

    bool IsStatic(eVertexBufferUsage _usage)
    {
        return  _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_COPY ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_READ ||
            _usage == eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_DRAW;
    }




    VertexArrayObject::VertexArrayObject(HWBufferBase* _buffers[], int _count)
        : m_vaoId(INVALID_ID)
    {
        glGenVertexArrays(1, &m_vaoId);
        Bind();
        for (int i = 0; i < _count; ++i) {
            _buffers[i]->Bind();
            _buffers[i]->EnableAttributes();
        }
        UnBind();

        for (int i = 0; i < _count; ++i) {
            //_buffers[i]->DisableAttributes(i);
            _buffers[i]->UnBind();
        }

    }

    //////////////////////////////////////////////////////////////////////////
    //VertexArrayObject Implementation
    //////////////////////////////////////////////////////////////////////////
   
    VertexArrayObject::~VertexArrayObject()
    {
        if (m_vaoId != 0)
            glDeleteVertexArrays(1, &m_vaoId);

    }

    //////////////////////////////////////////////////////////////////////////
    //HWBufferBase Implementation
    //////////////////////////////////////////////////////////////////////////
    HWBufferBase::~HWBufferBase()
    {

    }

}

