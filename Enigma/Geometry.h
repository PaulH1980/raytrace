#pragma once
#include "FrontEndDef.h"

namespace RayTrace
{
    struct GeometryFlags
    {
        union
        {
            uint32_t m_flags = 0;
            struct {
                uint32_t m_modified;
                uint32_t m_uvs;
            }m_fields;
        };
    };

    struct sVertex
    {
        int m_edgeId;
    };


    struct sEdge
    {
        //vertices
        int m_start;
        int m_end;

        int m_neighbour;
    };

    struct sFace
    {
        //edge
        int m_start;
    };


    struct sUnprocessedFace
    {
        std::vector<Vector3f> m_verts;
        std::vector<Vector3f> m_normals;
        std::vector<Vector2f> m_uvs;

        void Build()
        {

        }

        void Flip() {
            std::reverse(m_verts.begin(), m_verts.end());
            std::reverse(m_uvs.begin(), m_uvs.end());
            Build();
        }
    };




    class GeometryBase
    {

    public:
        GeometryBase() {}

        virtual void Update() {};
        virtual void Clear() {
            m_verts.clear();
            m_normals.clear();
            m_tangents.clear();
            m_uvs.clear();
            m_indices.clear();

            //Half edge data-struct stuff
            m_edges.clear();
            m_faces.clear();
            m_vertices.clear();
        }


        uint32_t GetVertexCount() const {
            return m_verts.size();
        }

        uint32_t GetIndicesCount() const {
            return m_indices.size();
        }

        const Vector3f* GetVertices() const {
            return m_verts.data();
        }


        const Vector3f* GetNormals() const {
            return m_normals.data();
        }

        const Vector3f* GetTangents() const {
            return m_tangents.data();
        }

        const Vector2f* GetTexCoords() const {
            return m_uvs.data();
        }

        const uint32_t* GetIndices() const {
            return m_indices.data();
        }


        ShaderProgram* m_pShader = nullptr;
        HWMaterial* m_pMaterial = nullptr;

        Matrix4x4 m_localTrans,
                  m_worldTrans;

        std::vector<Vector3f> m_verts;
        std::vector<Vector3f> m_normals;
        std::vector<Vector3f> m_tangents;
        std::vector<Vector2f> m_uvs;
        std::vector<uint32_t> m_indices;

        //Half edge data-struct stuff
        std::vector<sEdge>    m_edges;
        std::vector<sFace>    m_faces;
        std::vector<sVertex>  m_vertices;

        std::vector<sUnprocessedFace> m_unprocessed;
    };
}