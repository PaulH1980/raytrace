#pragma once
#include "HWFormats.h"
#include "Transform.h"


namespace RayTrace
{

   





    bool IsDynamic(eVertexBufferUsage _usage);
    bool IsStream(eVertexBufferUsage _usage);
    bool IsStatic(eVertexBufferUsage _usage);

    //helpers
    Vec2fBuffer* CreateVec2fBuffer(const Vector2f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage = eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_DRAW);
    Vec3fBuffer* CreateVec3fBuffer(const Vector3f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage = eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_DRAW);
    Vec4fBuffer* CreateVec4fBuffer(const Vector4f* _pData, uint32_t _count, uint32_t _attId, eVertexBufferUsage _usage = eVertexBufferUsage::VERTEX_BUFFER_USAGE_STATIC_DRAW);
    IndexBuffer* CreateIndexBuffer(const uint32_t* _pData, uint32_t _count);

    
    
   
    
    //////////////////////////////////////////////////////////////////////////
   //VertexArrayObject declaration
   //////////////////////////////////////////////////////////////////////////
    class VertexArrayObject
    {
    public:

        VertexArrayObject(HWBufferBase* _buffers[], int _count);
        virtual ~VertexArrayObject();

        void Bind() {
            assert(m_bound == false && m_vaoId != 0);
            glBindVertexArray(m_vaoId);
            m_bound = true;
        }

        void UnBind() {
            assert(m_bound == true);
            glBindVertexArray(0);
            m_bound = false;
        }
        bool     m_bound = false;
        uint32_t m_vaoId = 0; // zero means invalid for vao's
        uint32_t m_prevId = INVALID_ID;        
    };




    class HWBufferBase
    {
    public:

        virtual ~HWBufferBase();
        virtual void Bind() = 0;
        virtual void EnableAttributes() const = 0;
        virtual void DisableAttributes() const = 0;
        virtual void Update(const void* _data, uint32_t _numBytes, uint32_t _offset) const = 0;
        virtual void UnBind() = 0;
    };



    template<typename T>
    class VertexBuffer : public HWBufferBase
    {
    public:
        typedef T Type;

        struct BufferInfo//
        {
            uint32_t		m_index = 0;
            uint32_t		m_numComps = 4;
            uint32_t		m_compType = GL_FLOAT;
            uint32_t		m_stride = 0;
            bool			m_normalized = false;
        };

        VertexBuffer(const T* _pData, uint32_t _count,
            eVertexBufferTarget _type = VERTEX_BUFFER_VERTEX,
            eVertexBufferUsage _usage = VERTEX_BUFFER_USAGE_STATIC_DRAW)
            : m_type(_type)
            , m_usage(_usage)
            , m_bound(false)
            , m_count(_count)
            , m_bufSize(0)
            , m_id(0)
        {

            bool valid = Initialize(_pData, _count, _type, _usage);
            assert(valid);
        }

        VertexBuffer()
            : m_type(VERTEX_BUFFER_TYPE_UNDEFINED)
            , m_usage(VERTEX_BUFFER_USAGE_UNDEFINED)
            , m_bound(false)
            , m_mapped(false)
            , m_count(0)
            , m_bufSize(0)
            , m_id(0)
        {

        }

        virtual ~VertexBuffer();

        bool Initialize(const T* _pData, uint32_t _count,
            eVertexBufferTarget _type = VERTEX_BUFFER_VERTEX,
            eVertexBufferUsage _usage = VERTEX_BUFFER_USAGE_STATIC_DRAW)
        {
            assert(m_id == 0);

            m_type = _type;
            m_usage = _usage;
            m_bound = false;
            m_mapped = false;
            m_bufSize = sizeof(T) * _count;
            m_count = _count;

            glGenBuffers(1, &m_id);
            Bind();
            glBufferData(GetType(), m_bufSize, _pData, GetUsage());
            UnBind();

            return true;
        }

        int GetType() const
        {
            if (m_type == VERTEX_BUFFER_TYPE_UNDEFINED)
                throw std::exception("Invalid Buffer Type");
            return m_type;
        }

        int GetUsage() const
        {
            if (m_usage == VERTEX_BUFFER_USAGE_UNDEFINED)
                throw std::exception("Invalid Buffer Usage");
            return m_usage;
        }

        void Bind() override
        {
            assert(m_bound == false);
            glBindBuffer(GetType(), m_id);
            m_bound = true;
        }

        void BindRange( uint32_t _blockIdx,  uint32_t _offset, uint32_t _size) {
            glBindBufferRange(GetType(), _blockIdx, m_id, _offset, _size);
        }

        void BindBufferBase( uint32_t _blockIdx ) {
            glBindBufferBase(GetType(), _blockIdx, m_id);
        }

        void UpdateTyped(const T* _pData, uint32_t _count, uint32_t _offset = 0) {
            Update(_pData, sizeof(T) * _count, _offset);
        }

        void Update(const void* _pData, uint32_t _numBytes, uint32_t _offset) const override {
            assert(m_bound == true && IsMappable());
            glBufferSubData(GetType(), _offset, _numBytes, _pData);
        }

        bool IsMappable() const {
            return IsDynamic(m_usage) || IsStream(m_usage);
        }

        void* MapBuffer(eVertexBufferAccess _acces)
        {
            assert(m_bound == true);
            void* result = glMapBuffer(m_type, _acces);
            assert(result != nullptr);
            m_mapped = true;
            return result;
        }

        void UnmapBuffer() {
            assert(m_mapped == true && m_bound == true);
            glUnmapBuffer(m_type);
            m_mapped = false;
        }

        void EnableAttributes() const override {
            assert(m_bound);
            if (GetType() == GL_ELEMENT_ARRAY_BUFFER)
            {
                //NOP
            }
            else if (GetType() == VERTEX_BUFFER_VERTEX)
            {
                glVertexAttribPointer(m_info.m_index, m_info.m_numComps, m_info.m_compType,
                    m_info.m_normalized, m_info.m_stride, nullptr);
                glEnableVertexAttribArray(m_info.m_index);
            }
        }

        void DisableAttributes() const override {
            assert(m_bound);
            if (GetType() == GL_ELEMENT_ARRAY_BUFFER)
            {
                //NOP
            }
            else if (GetType() == VERTEX_BUFFER_VERTEX) {
                glDisableVertexAttribArray(m_info.m_index);
            }

        }

        virtual void UnBind() override
        {
            assert(m_bound == true);
            glBindBuffer(GetType(), 0);
            m_bound = false;
        }

        uint32_t    GetCount() const
        {
            return m_count;
        }


        uint32_t    GetBufferId() const {
            return m_id;
        }

        bool                 m_mapped;
        bool    			 m_bound;
        uint32_t			 m_id;
        uint32_t             m_count;
        std::size_t          m_bufSize;
        eVertexBufferTarget  m_type;
        eVertexBufferUsage	 m_usage;
        BufferInfo           m_info;
    };

    template<typename T>
    VertexBuffer<T>::~VertexBuffer()
    {
        if (m_id != 0)
            glDeleteBuffers(1, &m_id);
    }

}