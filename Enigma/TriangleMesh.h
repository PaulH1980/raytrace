#pragma once
#include <map>

#include "Shape.h"

#include "Texture.h"


namespace RayTrace
{
	
    
    //////////////////////////////////////////////////////////////////////////
	//class TriangleMesh
	//////////////////////////////////////////////////////////////////////////
    // Triangle Declarations
    struct TriangleMesh {
		using AlphaTexturePtr = FloatTexturePtr;
        // TriangleMesh Public Methods
        TriangleMesh(const Transform& ObjectToWorld, int nTriangles,
            const int* vertexIndices, int nVertices, const Vector3f* P,
            const Vector3f* S, const  Vector3f* N, const Vector2f* uv,
            const AlphaTexturePtr& alphaMask,
            const AlphaTexturePtr& shadowAlphaMask,
            const int* faceIndices);

        // TriangleMesh Data
        const int m_nTriangles, m_nVertices;
        std::vector<uint32_t> m_vertexIndices;
		std::vector<int> m_faceIndices;
        std::unique_ptr<Vector3f[]> m_p;
        std::unique_ptr<Vector3f[]> m_n;
        std::unique_ptr<Vector3f[]> m_s;
        std::unique_ptr<Vector2f[]> m_uv;
        AlphaTexturePtr					m_alphaMask,
										m_shadowAlphaMask;
        Transform   m_o2w;

        
    };
	//////////////////////////////////////////////////////////////////////////
	//class Triangle
	//////////////////////////////////////////////////////////////////////////
	class Triangle : public Shape
	{
    public:
        // Triangle Public Methods
        Triangle(const Transform* ObjectToWorld, const Transform* WorldToObject,
            bool reverseOrientation, const std::shared_ptr<TriangleMesh>& mesh,
            int triNumber);
        BBox3f                objectBound() const;
        BBox3f                worldBound() const;
        bool                        intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const;
        bool                        intersectP(const Ray& ray, bool testAlphaTexture ) const;
        float                       area() const;

        using Shape::sample;  // Bring in the other Sample() overload.
        Interaction                 sample(const Vector2f& u, float* pdf) const;

        // Returns the solid angle subtended by the triangle w.r.t. the given
        // reference point p.
        float                       solidAngle(const Vector3f& p, int nSamples) const;

    private:
        // Triangle Private Methods
        void getUVs(Vector2f uv[3]) const;
        void getVertices(Vector3f verts[3]) const;
        bool getNormals(Vector3f norm[3]) const;
        bool getTangents(Vector3f tang[3]) const;

        bool intersects(const Ray& ray, float* tHit, Vector3f* bOut ) const;

        // Triangle Private Data
        std::shared_ptr<TriangleMesh> m_pMesh;
        const uint32_t* m_startIndex;
        int             m_faceIndex;
    };
 
    ShapesVector CreatePLYMesh(
        const Transform* o2w, const Transform* w2o, bool reverseOrientation,
        const ParamSet& params, FloatTextureMap* _textureMap = nullptr, 
        TriangleMeshPtr* _resultOut = nullptr);
       
	
	ShapesVector  CreateTriangleMeshShape(const Transform* _o2w, const Transform* _w2o,
			bool _reverseOrientation, const ParamSet& _param, FloatTextureMap* _textureMap, 
            TriangleMeshPtr* _resultOut = nullptr );

}