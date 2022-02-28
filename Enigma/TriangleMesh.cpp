#include "Transform.h"
#include "MonteCarlo.h"
#include "BBox.h"
#include "Texture.h"
#include "Error.h"
#include "ParameterSet.h"


#include "ext/rply.h"


#include "TriangleMesh.h"





namespace RayTrace
{
#pragma region TriangleMesh
	

    TriangleMesh::TriangleMesh(const Transform& ObjectToWorld, int nTriangles, const int* vertexIndices, int nVertices, 
		const Vector3f* P, const Vector3f* S, const Vector3f* N, const Vector2f* uv,
		const AlphaTexturePtr& alphaMask, const AlphaTexturePtr& shadowAlphaMask, const int* faceIndices)
        : m_nTriangles(nTriangles)
        , m_nVertices(nVertices)
        , m_vertexIndices(vertexIndices, vertexIndices + 3 * nTriangles)
        , m_alphaMask(alphaMask)
        , m_shadowAlphaMask(shadowAlphaMask)
        , m_o2w( ObjectToWorld )
	{
      
        // Transform mesh vertices to world space
        m_p.reset(new Vector3f[nVertices]);
        for (int i = 0; i < nVertices; ++i) 
			m_p[i] = ObjectToWorld.transformPoint(P[i]);

        // Copy _UV_, _N_, and _S_ vertex data, if present
        if (uv) {
            m_uv.reset(new Vector2f[nVertices]);
            memcpy(m_uv.get(), uv, nVertices * sizeof(Vector2f));
        }
        if (N) {
			m_n.reset(new Vector3f[nVertices]);
            for (int i = 0; i < nVertices; ++i)  
				m_n[i] = ObjectToWorld.transformNormal(N[i]);
        }
        if (S) {
			m_s.reset(new Vector3f[nVertices]);
            for (int i = 0; i < nVertices; ++i)  
				m_s[i] = ObjectToWorld.transformVector(S[i]);
        }

        if (faceIndices)
            m_faceIndices = std::vector<int>(faceIndices, faceIndices + nTriangles);

    }
#pragma  endregion

#pragma  region PlyMesh
    struct CallbackContext {
        Vector3f* p;
        Vector3f* n;
        Vector2f* uv;
        int* indices;
        int* faceIndices;
        int indexCtr, faceIndexCtr;
        int face[4];
        bool error;
        int vertexCount;

        CallbackContext()
            : p(nullptr),
            n(nullptr),
            uv(nullptr),
            indices(nullptr),
            faceIndices(nullptr),
            indexCtr(0),
            faceIndexCtr(0),
            error(false),
            vertexCount(0) {}

        ~CallbackContext() {
            delete[] p;
            delete[] n;
            delete[] uv;
            delete[] indices;
            delete[] faceIndices;
        }
    };

    void rply_message_callback(p_ply ply, const char* message) {
        Warning("rply: %s", message);
    }

    /* Callback to handle vertex data from RPly */
    int rply_vertex_callback(p_ply_argument argument) {
        float** buffers;
        long index, flags;

        ply_get_argument_user_data(argument, (void**)&buffers, &flags);
        ply_get_argument_element(argument, nullptr, &index);

        int bufferIndex = (flags & 0xF00) >> 8;
        int stride = (flags & 0x0F0) >> 4;
        int offset = flags & 0x00F;

        float* buffer = buffers[bufferIndex];
        if (buffer)
            buffer[index * stride + offset] =
            (float)ply_get_argument_value(argument);

        return 1;
    }

    /* Callback to handle face data from RPly */
    int rply_face_callback(p_ply_argument argument) {
        CallbackContext* context;
        long flags;
        ply_get_argument_user_data(argument, (void**)&context, &flags);

        if (flags == 0) {
            // Vertex indices

            long length, value_index;
            ply_get_argument_property(argument, nullptr, &length, &value_index);

            if (length != 3 && length != 4) {
                Warning("plymesh: Ignoring face with %i vertices (only triangles and quads "
                    "are supported!)",
                    (int)length);
                return 1;
            }
            else if (value_index < 0) {
                return 1;
            }
            if (length == 4)
              //  CHECK(context->faceIndices == nullptr) <<
                "face_indices not yet supported for quads";

            if (value_index >= 0) {
                int value = (int)ply_get_argument_value(argument);
                if (value < 0 || value >= context->vertexCount) {
                    Error(
                        "plymesh: Vertex reference %i is out of bounds! "
                        "Valid range is [0..%i)",
                        value, context->vertexCount);
                    context->error = true;
                }
                context->face[value_index] = value;
            }

            if (value_index == length - 1) {
                for (int i = 0; i < 3; ++i)
                    context->indices[context->indexCtr++] = context->face[i];

                if (length == 4) {
                    /* This was a quad */
                    context->indices[context->indexCtr++] = context->face[3];
                    context->indices[context->indexCtr++] = context->face[0];
                    context->indices[context->indexCtr++] = context->face[2];
                }
            }
        }
        else {
            //CHECK_EQ(1, flags);
            // Face indices
            context->faceIndices[context->faceIndexCtr++] =
                (int)ply_get_argument_value(argument);
        }

        return 1;
    }


#pragma endregion


#pragma region Triangle


    Triangle::Triangle(const Transform* ObjectToWorld, const Transform* WorldToObject, 
		bool reverseOrientation, const std::shared_ptr<TriangleMesh>& mesh, int triNumber) 
		: Shape(ObjectToWorld, WorldToObject, reverseOrientation)
        , m_pMesh(mesh)
    {
        m_startIndex = &mesh->m_vertexIndices[3 * triNumber];        
        m_faceIndex = mesh->m_faceIndices.size() ? mesh->m_faceIndices[triNumber] : 0;
    }
  

    BBox3f Triangle::objectBound() const
	{
        Vector3f verts[3];
        getVertices(verts);

        BBox3f retVal;
        retVal.addPoint(m_worldToObject->transformPoint(verts[0]));
        retVal.addPoint(m_worldToObject->transformPoint(verts[1]));
        retVal.addPoint(m_worldToObject->transformPoint(verts[2]));
        return retVal;
	}

	BBox3f Triangle::worldBound() const
	{
      	Vector3f verts[3];
		getVertices(verts);

        BBox3f retVal;
        retVal.addPoint(verts[0]);
        retVal.addPoint(verts[1]);
        retVal.addPoint(verts[2]);
        return retVal; 
	}
	

    bool Triangle::intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    {        
       
        Vector3f b;
        float t;
        if( !intersects( ray, &t, &b))
            return false;

        const Vector3f& p0 = m_pMesh->m_p[m_startIndex[0]];
        const Vector3f& p1 = m_pMesh->m_p[m_startIndex[1]];
        const Vector3f& p2 = m_pMesh->m_p[m_startIndex[2]];

        float b0 = b[0];
        float b1 = b[1];
        float b2 = b[2];

        // Compute triangle partial derivatives
        Vector3f dpdu, dpdv;
        Vector2f uv[3];
        getUVs(uv);

        // Compute deltas for triangle partial derivatives
        Vector2f duv02 = uv[0] - uv[2], duv12 = uv[1] - uv[2];
        Vector3f dp02 = p0 - p2, dp12 = p1 - p2;
        float determinant = duv02[0] * duv12[1] - duv02[1] * duv12[0];
        bool degenerateUV = std::abs(determinant) < 1e-8;
        if (!degenerateUV) {
            float invdet = 1 / determinant;
            dpdu = (duv12[1] * dp02 - duv02[1] * dp12) * invdet;
            dpdv = (-duv12[0] * dp02 + duv02[0] * dp12) * invdet;
        }
        if (degenerateUV ||  LengthSqr(Cross(dpdu, dpdv) ) == 0) {
            // Handle zero determinant for triangle partial derivative matrix
            Vector3f ng = Cross(p2 - p0, p1 - p0);
            if (LengthSqr(ng) == 0)
                // The triangle is actually degenerate; the intersection is
                // bogus.
                return false;

            CoordinateSystem(Normalize(ng), &dpdu, &dpdv);
        }

        // Compute error bounds for triangle intersection
        float xAbsSum =
            (std::abs(b0 * p0.x) + std::abs(b1 * p1.x) + std::abs(b2 * p2.x));
        float yAbsSum =
            (std::abs(b0 * p0.y) + std::abs(b1 * p1.y) + std::abs(b2 * p2.y));
        float zAbsSum =
            (std::abs(b0 * p0.z) + std::abs(b1 * p1.z) + std::abs(b2 * p2.z));
        Vector3f pError = gamma(7) * Vector3f(xAbsSum, yAbsSum, zAbsSum);

        // Interpolate $(u,v)$ parametric coordinates and hit point
        Vector3f pHit = b0 * p0 + b1 * p1 + b2 * p2;
        Vector2f uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];

        // Test intersection against alpha texture, if present
        if (testAlphaTexture && m_pMesh->m_alphaMask) {
            SurfaceInteraction isectLocal(pHit, Vector3f(0, 0, 0), uvHit, -ray.m_dir,
                dpdu, dpdv, Vector3f(0, 0, 0),
                Vector3f(0, 0, 0), ray.m_time, this);
            if (m_pMesh->m_alphaMask->Evaluate(isectLocal) == 0) return false;
        }

        // Fill in _SurfaceInteraction_ from triangle hit
        *isect = SurfaceInteraction(pHit, pError, uvHit, -ray.m_dir, dpdu, dpdv,
            Vector3f(0, 0, 0), Vector3f(0, 0, 0), ray.m_time,
            this, m_faceIndex);

        // Override surface normal in _isect_ for triangle
        isect->m_n = isect->shading.m_n = Normalize(Cross(dp02, dp12));
        if (m_reverseOrientation ^ m_transformSwapsHandedness)
            isect->m_n = isect->shading.m_n = -isect->m_n;

        if (m_pMesh->m_n || m_pMesh->m_s) {
            // Initialize _Triangle_ shading geometry

            // Compute shading normal _ns_ for triangle
            Vector3f ns;
            if (m_pMesh->m_n) {
                Vector3f normals[3];
                getNormals(normals);
                ns = (b0 * normals[0] + b1 * normals[1] + b2 * normals[2]);
                if (LengthSqr(ns) > 0)
                    ns = Normalize(ns);
                else
                    ns = isect->m_n;
            }
            else
                ns = isect->m_n;

            // Compute shading tangent _ss_ for triangle
            Vector3f ss;
            if (m_pMesh->m_s) {
                Vector3f tang[3];
                getTangents(tang);

                ss = (b0 * tang[0] + b1 * tang[1] + b2 * tang[2]);
                if (LengthSqr(ss) > 0)
                    ss = Normalize(ss);
                else
                    ss = Normalize(isect->m_dpdu);
            }
            else
                ss = Normalize(isect->m_dpdu);

            // Compute shading bitangent _ts_ for triangle and adjust _ss_
            Vector3f ts = Cross(ss, ns);
            if (LengthSqr(ts) > 0.f) {
                ts = Normalize(ts);
                ss = Cross(ts, ns);
            }
            else
                CoordinateSystem((Vector3f)ns, &ss, &ts);

            // Compute $\dndu$ and $\dndv$ for triangle shading geometry
            Vector3f dndu, dndv;
            if (m_pMesh->m_n)
            {
                Vector3f normals[3];
                getNormals(normals);
                
                // Compute deltas for triangle partial derivatives of normal
                Vector2f duv02 = uv[0] - uv[2];
                Vector2f duv12 = uv[1] - uv[2];
                Vector3f dn1 = normals[0] - normals[2];
                Vector3f dn2 = normals[1] - normals[2];
                float determinant = duv02[0] * duv12[1] - duv02[1] * duv12[0];
                bool degenerateUV = std::abs(determinant) < 1e-8;
                if (degenerateUV) {
                    // We can still compute dndu and dndv, with respect to the
                    // same arbitrary coordinate system we use to compute dpdu
                    // and dpdv when this happens. It's important to do this
                    // (rather than giving up) so that ray differentials for
                    // rays reflected from triangles with degenerate
                    // parameterizations are still reasonable.
                    Vector3f dn = Cross(normals[2] - normals[0], normals[1] - normals[0]);                      
                    if (LengthSqr(dn) == 0)
                        dndu = dndv = Vec3fZero;
                    else {
                        Vector3f dnu, dnv;
                        CoordinateSystem(dn, &dndu, &dndv);                      
                    }
                }
                else {
                    float invDet = 1 / determinant;
                    dndu = (duv12[1] * dn1 - duv02[1] * dn2) * invDet;
                    dndv = (-duv12[0] * dn1 + duv02[0] * dn2) * invDet;
                }
            }
            else
                dndu = dndv = Vec3fZero;
            if (m_reverseOrientation) ts = -ts;
            isect->SetShadingGeometry(ss, ts, dndu, dndv, true);
        }

        *tHit = t;      
        return true;
    }

    bool Triangle::intersectP(const Ray& ray, bool testAlphaTexture) const
    {
        Vector3f b;
        float t;
        if( !intersects( ray, &t, &b))
            return false;

        const Vector3f& p0 = m_pMesh->m_p[m_startIndex[0]];
        const Vector3f& p1 = m_pMesh->m_p[m_startIndex[1]];
        const Vector3f& p2 = m_pMesh->m_p[m_startIndex[2]];

        float b0 = b[0];
        float b1 = b[1];
        float b2 = b[2];


        // Test shadow ray intersection against alpha texture, if present
        if (testAlphaTexture && (m_pMesh->m_alphaMask || m_pMesh->m_shadowAlphaMask)) {
            // Compute triangle partial derivatives
            Vector3f dpdu, dpdv;
            Vector2f uv[3];
            getUVs(uv);

            // Compute deltas for triangle partial derivatives
            Vector2f duv02 = uv[0] - uv[2], 
                     duv12 = uv[1] - uv[2];

            Vector3f dp02 = p0 - p2, 
                     dp12 = p1 - p2;
            float determinant = duv02[0] * duv12[1] - duv02[1] * duv12[0];
            bool degenerateUV = std::abs(determinant) < 1e-8;
            if (!degenerateUV) {
                float invdet = 1 / determinant;
                dpdu = (duv12[1] * dp02 - duv02[1] * dp12) * invdet;
                dpdv = (-duv12[0] * dp02 + duv02[0] * dp12) * invdet;
            }
            if (degenerateUV ||  LengthSqr(Cross(dpdu, dpdv)) == 0) {
                // Handle zero determinant for triangle partial derivative matrix
                Vector3f ng = Cross(p2 - p0, p1 - p0);
                if (LengthSqr(ng)== 0)
                    // The triangle is actually degenerate; the intersection is
                    // bogus.
                    return false;

                CoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &dpdu, &dpdv);
            }

            // Interpolate $(u,v)$ parametric coordinates and hit point
            Vector3f pHit  = b0 * p0 + b1 * p1 + b2 * p2;
            Vector2f uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];
            SurfaceInteraction isectLocal(pHit, Vector3f(0, 0, 0), uvHit, -ray.m_dir,dpdu, dpdv, Vector3f(0, 0, 0), Vector3f(0, 0, 0), ray.m_time, this);
            if (m_pMesh->m_alphaMask && 
                m_pMesh->m_alphaMask->Evaluate(isectLocal) == 0)
                return false;
            if( m_pMesh->m_shadowAlphaMask &&
                m_pMesh->m_shadowAlphaMask->Evaluate(isectLocal) == 0)
                return false;
        }     
        return true;
    }

    float Triangle::area() const
	{
		Vector3f verts[3];
		getVertices(verts); 

        auto crossed = Cross(verts[1] - verts[0], verts[2] - verts[0]);
        return Length(crossed) * 0.5f;

	}

    void Triangle::getUVs(Vector2f uv[3]) const
    {
        if (m_pMesh->m_uv) {

            uv[0] = m_pMesh->m_uv[m_startIndex[0]];
            uv[1] = m_pMesh->m_uv[m_startIndex[1]];
            uv[2] = m_pMesh->m_uv[m_startIndex[2]];
        }
        else {
            uv[0] = Vector2f(0, 0);
            uv[1] = Vector2f(1, 0);
            uv[2] = Vector2f(1, 1);
        }
    }

    void Triangle::getVertices(Vector3f verts[3]) const
	{
        verts[0] = m_pMesh->m_p[m_startIndex[0]];
        verts[1] = m_pMesh->m_p[m_startIndex[1]];
        verts[2] = m_pMesh->m_p[m_startIndex[2]];
	}

    bool Triangle::getNormals(Vector3f norm[3]) const
	{
		if ( !m_pMesh->m_n )
			return false;
        norm[0] = m_pMesh->m_n[m_startIndex[0]];
        norm[1] = m_pMesh->m_n[m_startIndex[1]];
        norm[2] = m_pMesh->m_n[m_startIndex[2]];
		return true;
	}

    bool Triangle::getTangents(Vector3f tang[3]) const
    {
        if (!m_pMesh->m_s)
            return false;
        tang[0] = m_pMesh->m_s[m_startIndex[0]];
        tang[1] = m_pMesh->m_s[m_startIndex[1]];
        tang[2] = m_pMesh->m_s[m_startIndex[2]];
        return true;
    }


    bool Triangle::intersects(const Ray& ray, float* tHit, Vector3f* bOut) const
    {
        const Vector3f& p0 = m_pMesh->m_p[m_startIndex[0]];
        const Vector3f& p1 = m_pMesh->m_p[m_startIndex[1]];
        const Vector3f& p2 = m_pMesh->m_p[m_startIndex[2]];

        // Perform ray--triangle intersection test

        // Transform triangle vertices to ray coordinate space

        // Translate vertices based on ray origin
        Vector3f p0t = p0 - ray.m_origin;
        Vector3f p1t = p1 - ray.m_origin;
        Vector3f p2t = p2 - ray.m_origin;

        // Permute components of triangle vertices and ray direction
        int kz = MaxDimension(Abs(ray.m_dir));
        int kx = kz + 1;
        if (kx == 3) kx = 0;
        int ky = kx + 1;
        if (ky == 3) ky = 0;
        Vector3f d = Permute(ray.m_dir, kx, ky, kz);
        p0t = Permute(p0t, kx, ky, kz);
        p1t = Permute(p1t, kx, ky, kz);
        p2t = Permute(p2t, kx, ky, kz);

        // Apply shear transformation to translated vertex positions
        float Sx = -d.x / d.z;
        float Sy = -d.y / d.z;
        float Sz = 1.f / d.z;
        p0t.x += Sx * p0t.z;
        p0t.y += Sy * p0t.z;
        p1t.x += Sx * p1t.z;
        p1t.y += Sy * p1t.z;
        p2t.x += Sx * p2t.z;
        p2t.y += Sy * p2t.z;

        // Compute edge function coefficients _e0_, _e1_, and _e2_
        float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
        float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
        float e2 = p0t.x * p1t.y - p0t.y * p1t.x;

        // Fall back to double precision test at triangle edges
        if (e0 == 0.0f || e1 == 0.0f || e2 == 0.0f)
        {
            double p2txp1ty = (double)p2t.x * (double)p1t.y;
            double p2typ1tx = (double)p2t.y * (double)p1t.x;
            e0 = (float)(p2typ1tx - p2txp1ty);
            double p0txp2ty = (double)p0t.x * (double)p2t.y;
            double p0typ2tx = (double)p0t.y * (double)p2t.x;
            e1 = (float)(p0typ2tx - p0txp2ty);
            double p1txp0ty = (double)p1t.x * (double)p0t.y;
            double p1typ0tx = (double)p1t.y * (double)p0t.x;
            e2 = (float)(p1typ0tx - p1txp0ty);
        }

        // Perform triangle edge and determinant tests
        if ((e0 < 0 || e1 < 0 || e2 < 0) &&
            (e0 > 0 || e1 > 0 || e2 > 0))
            return false;
        float det = e0 + e1 + e2;
        if (det == 0)
            return false;

        // Compute scaled hit distance to triangle and test against ray $t$ range
        p0t.z *= Sz;
        p1t.z *= Sz;
        p2t.z *= Sz;
        float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;
        if (det < 0 && (tScaled >= 0 || tScaled < ray.m_maxT * det))
            return false;
        else if (det > 0 && (tScaled <= 0 || tScaled > ray.m_maxT * det))
            return false;

        // Compute barycentric coordinates and $t$ value for triangle intersection
        float invDet = 1 / det;
        float b0 = e0 * invDet;
        float b1 = e1 * invDet;
        float b2 = e2 * invDet;
        float t = tScaled * invDet;

        // Ensure that computed triangle $t$ is conservatively greater than zero

        // Compute $\delta_z$ term for triangle $t$ error bounds
        float maxZt = MaxComponent(Abs(Vector3f(p0t.z, p1t.z, p2t.z)));
        float deltaZ = gamma(3) * maxZt;

        // Compute $\delta_x$ and $\delta_y$ terms for triangle $t$ error bounds
        float maxXt = MaxComponent(Abs(Vector3f(p0t.x, p1t.x, p2t.x)));
        float maxYt = MaxComponent(Abs(Vector3f(p0t.y, p1t.y, p2t.y)));
        float deltaX = gamma(5) * (maxXt + maxZt);
        float deltaY = gamma(5) * (maxYt + maxZt);

        // Compute $\delta_e$ term for triangle $t$ error bounds
        float deltaE =
            2 * (gamma(2) * maxXt * maxYt + deltaY * maxXt + deltaX * maxYt);

        // Compute $\delta_t$ term for triangle $t$ error bounds and check _t_
        float maxE = MaxComponent(Abs(Vector3f(e0, e1, e2)));
        float deltaT = 3 *
            (gamma(3) * maxE * maxZt + deltaE * maxZt + deltaZ * maxE) *
            std::abs(invDet);
        if (t <= deltaT) 
            return false;

        if( tHit)
            *tHit = t;
        if (bOut)
            *bOut = Vector3f(b0, b1, b2);

        return true;
    }

    Interaction Triangle::sample(const Vector2f& u, float* pdf) const
    {
        Vector2f b = UniformSampleTriangle(u);
        // Get triangle vertices in _p0_, _p1_, and _p2_
        const Vector3f& p0 = m_pMesh->m_p[m_startIndex[0]];
        const Vector3f& p1 = m_pMesh->m_p[m_startIndex[1]];
        const Vector3f& p2 = m_pMesh->m_p[m_startIndex[2]];
    
        Interaction it;
        it.m_p = b[0] * p0 + b[1] * p1 + (1 - b[0] - b[1]) * p2;
        // Compute surface normal for sampled point on triangle
        it.m_n = Normalize(Cross(p1 - p0, p2 - p0));
        // Ensure correct orientation of the geometric normal; follow the same
        // approach as was used in Triangle::Intersect().
        if (m_pMesh->m_n) {
            Vector3f normals[3];
            getNormals(normals);
            Vector3f ns(b[0] * normals[0] + b[1] * normals[1] + (1 - b[0] - b[1]) * normals[2]);         
            it.m_n = FaceForward(it.m_n, ns);
        }
        else if (m_reverseOrientation ^ m_transformSwapsHandedness)
            it.m_n *= -1;

        // Compute error bounds for sampled point on triangle
        Vector3f pAbsSum = Abs(b[0] * p0) + Abs(b[1] * p1) + Abs((1 - b[0] - b[1]) * p2);
        it.m_pError = gamma(6) * pAbsSum;
        *pdf = 1 / area();
        return it;
    }

    float Triangle::solidAngle(const Vector3f& p, int nSamples) const
    {
        Vector3f pSphere[3] = {
            Normalize(m_pMesh->m_p[m_startIndex[0]] - p),
            Normalize(m_pMesh->m_p[m_startIndex[1]] - p),
            Normalize(m_pMesh->m_p[m_startIndex[2]] - p)
        };
        Vector3f cross01 = (Cross(pSphere[0], pSphere[1]));
        Vector3f cross12 = (Cross(pSphere[1], pSphere[2]));
        Vector3f cross20 = (Cross(pSphere[2], pSphere[0]));

        // Some of these vectors may be degenerate. In this case, we don't want
        // to normalize them so that we don't hit an assert. This is fine,
        // since the corresponding dot products below will be zero.
        if ( LengthSqr(cross01 ) > 0) cross01 = Normalize(cross01);
        if ( LengthSqr(cross12) > 0) cross12 = Normalize(cross12);
        if ( LengthSqr(cross20) > 0) cross20 = Normalize(cross20);

        // We only need to do three cross products to evaluate the angles at
        // all three vertices, though, since we can take advantage of the fact
        // that Cross(a, b) = -Cross(b, a).
        return std::abs(
            std::acos(std::clamp(Dot(cross01, -cross12), -1.0f, 1.0f)) +
            std::acos(std::clamp(Dot(cross12, -cross20), -1.0f, 1.0f)) +
            std::acos(std::clamp(Dot(cross20, -cross01), -1.0f, 1.0f)) - PI);
    }


    std::vector<std::shared_ptr<Shape>> CreateTriangleMesh(
        const Transform* ObjectToWorld, const Transform* WorldToObject,
        bool reverseOrientation, int nTriangles, const int* vertexIndices,
        int nVertices, const Vector3f* p, const Vector3f* s, const Vector3f* n,
        const Vector2f* uv, const std::shared_ptr<Texture<float>>& alphaMask,
        const std::shared_ptr<Texture<float>>& shadowAlphaMask,
        const int* faceIndices, TriangleMeshPtr* _resultOut
        ) {
        std::shared_ptr<TriangleMesh> mesh = std::make_shared<TriangleMesh>(
            *ObjectToWorld, nTriangles, vertexIndices, nVertices, p, s, n, uv,
            alphaMask, shadowAlphaMask, faceIndices);
        if (_resultOut)
            *_resultOut = mesh;

        std::vector<std::shared_ptr<Shape>> tris;
        tris.reserve(nTriangles);
        for (int i = 0; i < nTriangles; ++i)
            tris.push_back(std::make_shared<Triangle>(ObjectToWorld, WorldToObject,
                reverseOrientation, mesh, i));
        return tris;
    }
#pragma endregion

    ShapesVector CreateTriangleMeshShape(
		const Transform* _o2w, const Transform* _w2o, 
		bool _reverseOrientation, const ParamSet& _params, 
		FloatTextureMap* _floatTextures, TriangleMeshPtr* _resultOut )
    {
        int nvi, npi, nuvi, nsi, nni;
        const int* vi = _params.FindInt("indices", &nvi);
        const Vector3f* P = _params.FindPoint3f("P", &npi);
        const Vector2f* uvs = _params.FindPoint2f("uv", &nuvi);
        if (!uvs) 
            uvs = _params.FindPoint2f("st", &nuvi);
        std::vector<Vector2f> tempUVs;
        if (!uvs) {
            const float* fuv = _params.FindFloat("uv", &nuvi);
            if (!fuv) 
                fuv = _params.FindFloat("st", &nuvi);
            if (fuv) {
                nuvi /= 2;
                tempUVs.reserve(nuvi);
                for (int i = 0; i < nuvi; ++i)
                    tempUVs.push_back(Vector2f(fuv[2 * i], fuv[2 * i + 1]));
                uvs = &tempUVs[0];
            }
        }
        if (uvs) {
            if (nuvi < npi) {
                Error(
                    "Not enough of \"uv\"s for triangle mesh.  Expected %d, "
                    "found %d.  Discarding.",
                    npi, nuvi);
                uvs = nullptr;
            }
            else if (nuvi > npi)
                Warning(
                    "More \"uv\"s provided than will be used for triangle "
                    "mesh.  (%d expected, %d found)",
                    npi, nuvi);
        }
        if (!vi) {
            Error(
                "Vertex indices \"indices\" not provided with triangle mesh shape");
            return std::vector<std::shared_ptr<Shape>>();
        }
        if (!P) {
            Error("Vertex positions \"P\" not provided with triangle mesh shape");
            return std::vector<std::shared_ptr<Shape>>();
        }
        const Vector3f* S = _params.FindVector3f("S", &nsi);
        if (S && nsi != npi) {
            Error("Number of \"S\"s for triangle mesh must match \"P\"s");
            S = nullptr;
        }
        const Vector3f* N = _params.FindNormal3f("N", &nni);
        if (N && nni != npi) {
            Error("Number of \"N\"s for triangle mesh must match \"P\"s");
            N = nullptr;
        }
        for (int i = 0; i < nvi; ++i)
            if (vi[i] >= npi) {
                Error(
                    "trianglemesh has out of-bounds vertex index %d (%d \"P\" "
                    "values were given",
                    vi[i], npi);
                return std::vector<std::shared_ptr<Shape>>();
            }

        int nfi;
        const int* faceIndices = _params.FindInt("faceIndices", &nfi);
        if (faceIndices && nfi != nvi / 3) {
            Error("Number of face indices, %d, doesn't match number of faces, %d",
                nfi, nvi / 3);
            faceIndices = nullptr;
        }

        std::shared_ptr<Texture<float>> alphaTex;
        std::string alphaTexName = _params.FindTexture("alpha");
        if (alphaTexName != "") {
            if (_floatTextures->find(alphaTexName) != _floatTextures->end())
                alphaTex = (*_floatTextures)[alphaTexName];
            else
                Error("Couldn't find float texture \"%s\" for \"alpha\" parameter",
                    alphaTexName.c_str());
        }
        else if (_params.FindOneFloat("alpha", 1.f) == 0.f)
            alphaTex.reset(new ConstantTexture<float>(0.f));

        std::shared_ptr<Texture<float>> shadowAlphaTex;
        std::string shadowAlphaTexName = _params.FindTexture("shadowalpha");
        if (shadowAlphaTexName != "") {
            if (_floatTextures->find(shadowAlphaTexName) != _floatTextures->end())
                shadowAlphaTex = (*_floatTextures)[shadowAlphaTexName];
            else
                Error(
                    "Couldn't find float texture \"%s\" for \"shadowalpha\" "
                    "parameter",
                    shadowAlphaTexName.c_str());
        }
        else if (_params.FindOneFloat("shadowalpha", 1.f) == 0.f)
            shadowAlphaTex.reset(new ConstantTexture<float>(0.f));

        return CreateTriangleMesh(_o2w, _w2o, _reverseOrientation, nvi / 3, vi, npi, P,
            S, N, uvs, alphaTex, shadowAlphaTex, faceIndices, _resultOut );
    }



    std::vector<std::shared_ptr<Shape>> CreatePLYMesh(
        const Transform* o2w,
        const Transform* w2o,
        bool reverseOrientation,
        const ParamSet& params,
        FloatTextureMap* floatTextures,
        TriangleMeshPtr* _resultOut 
        ) {
        const std::string filename = params.FindOneFilename("filename", "");
        p_ply ply = ply_open(filename.c_str(), rply_message_callback, 0, nullptr);
        if (!ply) {
            Error("Couldn't open PLY file \"%s\"", filename.c_str());
            return std::vector<std::shared_ptr<Shape>>();
        }

        if (!ply_read_header(ply)) {
            Error("Unable to read the header of PLY file \"%s\"", filename.c_str());
            return std::vector<std::shared_ptr<Shape>>();
        }

        p_ply_element element = nullptr;
        long vertexCount = 0, faceCount = 0;

        /* Inspect the structure of the PLY file */
        while ((element = ply_get_next_element(ply, element)) != nullptr) {
            const char* name;
            long nInstances;

            ply_get_element_info(element, &name, &nInstances);
            if (!strcmp(name, "vertex"))
                vertexCount = nInstances;
            else if (!strcmp(name, "face"))
                faceCount = nInstances;
        }

        if (vertexCount == 0 || faceCount == 0) {
            Error("%s: PLY file is invalid! No face/vertex elements found!",
                filename.c_str());
            return std::vector<std::shared_ptr<Shape>>();
        }

        CallbackContext context;

        if (ply_set_read_cb(ply, "vertex", "x", rply_vertex_callback, &context,
            0x030) &&
            ply_set_read_cb(ply, "vertex", "y", rply_vertex_callback, &context,
                0x031) &&
            ply_set_read_cb(ply, "vertex", "z", rply_vertex_callback, &context,
                0x032)) {
            context.p = new Vector3f[vertexCount];
        }
        else {
            Error("%s: Vertex coordinate property not found!",
                filename.c_str());
            return std::vector<std::shared_ptr<Shape>>();
        }

        if (ply_set_read_cb(ply, "vertex", "nx", rply_vertex_callback, &context,
            0x130) &&
            ply_set_read_cb(ply, "vertex", "ny", rply_vertex_callback, &context,
                0x131) &&
            ply_set_read_cb(ply, "vertex", "nz", rply_vertex_callback, &context,
                0x132))
            context.n = new Vector3f[vertexCount];

        /* There seem to be lots of different conventions regarding UV coordinate
         * names */
        if ((ply_set_read_cb(ply, "vertex", "u", rply_vertex_callback, &context,
            0x220) &&
            ply_set_read_cb(ply, "vertex", "v", rply_vertex_callback, &context,
                0x221)) ||
            (ply_set_read_cb(ply, "vertex", "s", rply_vertex_callback, &context,
                0x220) &&
                ply_set_read_cb(ply, "vertex", "t", rply_vertex_callback, &context,
                    0x221)) ||
            (ply_set_read_cb(ply, "vertex", "texture_u", rply_vertex_callback,
                &context, 0x220) &&
                ply_set_read_cb(ply, "vertex", "texture_v", rply_vertex_callback,
                    &context, 0x221)) ||
            (ply_set_read_cb(ply, "vertex", "texture_s", rply_vertex_callback,
                &context, 0x220) &&
                ply_set_read_cb(ply, "vertex", "texture_t", rply_vertex_callback,
                    &context, 0x221)))
            context.uv = new Vector2f[vertexCount];

        /* Allocate enough space in case all faces are quads */
        context.indices = new int[faceCount * 6];
        context.vertexCount = vertexCount;

        ply_set_read_cb(ply, "face", "vertex_indices", rply_face_callback, &context,
            0);
        if (ply_set_read_cb(ply, "face", "face_indices", rply_face_callback, &context,
            1))
            // Extra space in case they're quads
            context.faceIndices = new int[faceCount];

        if (!ply_read(ply)) {
            Error("%s: unable to read the contents of PLY file",
                filename.c_str());
            ply_close(ply);
            return std::vector<std::shared_ptr<Shape>>();
        }

        ply_close(ply);

        if (context.error) return std::vector<std::shared_ptr<Shape>>();

        // Look up an alpha texture, if applicable
        std::shared_ptr<Texture<float>> alphaTex;
        std::string alphaTexName = params.FindTexture("alpha");
        if (alphaTexName != "") {
            if (floatTextures->find(alphaTexName) != floatTextures->end())
                alphaTex = (*floatTextures)[alphaTexName];
            else
                Error("Couldn't find float texture \"%s\" for \"alpha\" parameter",
                    alphaTexName.c_str());
        }
        else if (params.FindOneFloat("alpha", 1.f) == 0.f) {
            alphaTex.reset(new ConstantTexture<float>(0.f));
        }

        std::shared_ptr<Texture<float>> shadowAlphaTex;
        std::string shadowAlphaTexName = params.FindTexture("shadowalpha");
        if (shadowAlphaTexName != "") {
            if (floatTextures->find(shadowAlphaTexName) != floatTextures->end())
                shadowAlphaTex = (*floatTextures)[shadowAlphaTexName];
            else
                Error(
                    "Couldn't find float texture \"%s\" for \"shadowalpha\" "
                    "parameter",
                    shadowAlphaTexName.c_str());
        }
        else if (params.FindOneFloat("shadowalpha", 1.f) == 0.f)
            shadowAlphaTex.reset(new ConstantTexture<float>(0.f));

        return CreateTriangleMesh(o2w, w2o, reverseOrientation,
            context.indexCtr / 3, context.indices,
            vertexCount, context.p, nullptr, context.n,
            context.uv, alphaTex, shadowAlphaTex,
            context.faceIndices, _resultOut);
    }


#pragma endregion

}

