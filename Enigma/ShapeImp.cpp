#include <algorithm>
#include <exception>
#include "eFloat.h"
#include "ShapeImp.h"
#include "Transform.h"
#include "ParameterSet.h"
#include "Interaction.h"
#include "MonteCarlo.h"
#include "BBox.h"



namespace RayTrace
{

	Sphere::Sphere(const Transform* _o2w,
		const Transform* _w2o, bool _reverseOrientation, 
		float _radius, float _zMin, float _zMax, float _phiMax) 
		: Shape( _o2w, _w2o, _reverseOrientation)
		, m_radius(  _radius )			
	{
		m_zMin = std::clamp(std::min(_zMin, _zMax), -m_radius, m_radius);
		m_zMax = std::clamp(std::max(_zMin, _zMax), -m_radius, m_radius);
		m_thetaMin = std::acosf(std::clamp(m_zMin / m_radius, -1.f, 1.f));
		m_thetaMax = std::acosf(std::clamp(m_zMax / m_radius, -1.f, 1.f));
		m_phiMax = ToRadians(std::clamp(_phiMax, 0.0f, 360.0f));
	}

	BBox3f Sphere::objectBound() const
	{
		return BBox3f( Vector3f( -m_radius, -m_radius, m_zMin ),
					   Vector3f( m_radius, m_radius, m_zMax ) ); 

	}

    bool Sphere::intersect(const Ray& _ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    {
        float phi;
        Vector3f pHit;
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
        Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);// (*WorldToObject)(r, &oErr, &dErr);

        // Compute quadratic sphere coefficients

        // Initialize _EFloat_ ray coordinate values
        EFloat ox(ray.m_origin.x, oErr.x), oy(ray.m_origin.y, oErr.y), oz(ray.m_origin.z, oErr.z);
        EFloat dx(ray.m_dir.x, dErr.x), dy(ray.m_dir.y, dErr.y), dz(ray.m_dir.z, dErr.z);

        EFloat a = dx * dx + dy * dy + dz * dz;
        EFloat b = 2 * (dx * ox + dy * oy + dz * oz);
        EFloat c = ox * ox + oy * oy + oz * oz - EFloat(m_radius) * EFloat(m_radius);

        // Solve quadratic equation for _t_ values
        EFloat t0, t1;
        if (!Quadratic(a, b, c, &t0, &t1)) return false;

        // Check quadric shape _t0_ and _t1_ for nearest intersection
        if (t0.UpperBound() > ray.m_maxT || t1.LowerBound() <= 0) return false;
        EFloat tShapeHit = t0;
        if (tShapeHit.LowerBound() <= 0) {
            tShapeHit = t1;
            if (tShapeHit.UpperBound() > ray.m_maxT) return false;
        }

        // Compute sphere hit position and $\phi$
        pHit = ray.scale((float)tShapeHit);

        // Refine sphere intersection point
        pHit *= m_radius / Distance(pHit, Vector3f(0, 0, 0));
        if (pHit.x == 0 && pHit.y == 0) pHit.x = 1e-5f * m_radius;
        phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 * PI;

        // Test sphere intersection against clipping parameters
        if ((m_zMin > -m_radius && pHit.z < m_zMin) || (m_zMax < m_radius && pHit.z > m_zMax) ||
            phi > m_phiMax) {
            if (tShapeHit == t1) return false;
            if (t1.UpperBound() > ray.m_maxT) return false;
            tShapeHit = t1;
            // Compute sphere hit position and $\phi$
            pHit = ray.scale((float)tShapeHit);

            // Refine sphere intersection point
            pHit *= m_radius / Distance(pHit, Vector3f(0, 0, 0));
            if (pHit.x == 0 && pHit.y == 0) pHit.x = 1e-5f * m_radius;
            phi = std::atan2(pHit.y, pHit.x);
            if (phi < 0) phi += 2 * PI;
            if ((m_zMin > -m_radius && pHit.z < m_zMin) ||
                (m_zMax < m_radius && pHit.z > m_zMax) || phi > m_phiMax)
                return false;
        }

        // Find parametric representation of sphere hit
        float u = phi / m_phiMax;
        float theta = std::acos(std::clamp(pHit.z / m_radius, -1.0f, 1.0f));
        float v = (theta - m_thetaMin) / (m_thetaMax - m_thetaMin);

        // Compute sphere $\dpdu$ and $\dpdv$
        float zRadius = std::sqrt(pHit.x * pHit.x + pHit.y * pHit.y);
        float invZRadius = 1 / zRadius;
        float cosPhi = pHit.x * invZRadius;
        float sinPhi = pHit.y * invZRadius;
        Vector3f dpdu(-m_phiMax * pHit.y, m_phiMax * pHit.x, 0);
        Vector3f dpdv =
            (m_thetaMax - m_thetaMin) *
            Vector3f(pHit.z * cosPhi, pHit.z * sinPhi, -m_radius * std::sin(theta));

        // Compute sphere $\dndu$ and $\dndv$
        Vector3f d2Pduu = -m_phiMax * m_phiMax * Vector3f(pHit.x, pHit.y, 0);
        Vector3f d2Pduv =
            (m_thetaMax - m_thetaMin) * pHit.z * m_phiMax * Vector3f(-sinPhi, cosPhi, 0.);
        Vector3f d2Pdvv = -(m_thetaMax - m_thetaMin) * (m_thetaMax - m_thetaMin) *
            Vector3f(pHit.x, pHit.y, pHit.z);

        // Compute coefficients for fundamental forms
        float E = Dot(dpdu, dpdu);
        float F = Dot(dpdu, dpdv);
        float G = Dot(dpdv, dpdv);
        Vector3f N = Normalize(Cross(dpdu, dpdv));
        float e = Dot(N, d2Pduu);
        float f = Dot(N, d2Pduv);
        float g = Dot(N, d2Pdvv);

        // Compute $\dndu$ and $\dndv$ from fundamental form coefficients
        float invEGF2 = 1 / (E * G - F * F);
		Vector3f dndu((f * F - e * G) * invEGF2 * dpdu +
            (e * F - f * E) * invEGF2 * dpdv);
		Vector3f dndv((g * F - f * G) * invEGF2 * dpdu +
            (f * F - g * E) * invEGF2 * dpdv);

        // Compute error bounds for sphere intersection
        Vector3f pError = gamma(5) * Abs(pHit); 

        // Initialize _SurfaceInteraction_ from parametric information
        *isect = m_objectToWorld->transformSurfInteraction(SurfaceInteraction(pHit, pError, Vector2f(u, v),
            -ray.m_dir, dpdu, dpdv, dndu, dndv,
            ray.m_time, this));

        // Update _tHit_ for quadric intersection
        *tHit = (float)tShapeHit;
        return true;
    }

    bool Sphere::intersectP(const Ray& _ray, bool testAlphaTexture) const
    {
        float phi;
        Vector3f pHit;
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
		Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);// (*WorldToObject)(r, &oErr, &dErr);

        // Compute quadratic sphere coefficients

        // Initialize _EFloat_ ray coordinate values
        EFloat ox(ray.m_origin.x, oErr.x), oy(ray.m_origin.y, oErr.y), oz(ray.m_origin.z, oErr.z);
        EFloat dx(ray.m_dir.x, dErr.x), dy(ray.m_dir.y, dErr.y), dz(ray.m_dir.z, dErr.z);
        
		EFloat a = dx * dx + dy * dy + dz * dz;
        EFloat b = 2 * (dx * ox + dy * oy + dz * oz);
        EFloat c = ox * ox + oy * oy + oz * oz - EFloat(m_radius) * EFloat(m_radius);

        // Solve quadratic equation for _t_ values
        EFloat t0, t1;
        if (!Quadratic(a, b, c, &t0, &t1)) return false;

        // Check quadric shape _t0_ and _t1_ for nearest intersection
        if (t0.UpperBound() > ray.m_maxT || t1.LowerBound() <= 0) return false;
        EFloat tShapeHit = t0;
        if (tShapeHit.LowerBound() <= 0) {
            tShapeHit = t1;
            if (tShapeHit.UpperBound() > ray.m_maxT) return false;
        }

        // Compute sphere hit position and $\phi$
		pHit = ray.scale((float)tShapeHit);

        // Refine sphere intersection point
        pHit *= m_radius / Distance(pHit, Vector3f(0, 0, 0));
        if (pHit.x == 0 && pHit.y == 0) pHit.x = 1e-5f * m_radius;
        phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 * PI;

        // Test sphere intersection against clipping parameters
        if ((m_zMin > -m_radius && pHit.z < m_zMin) || (m_zMax < m_radius && pHit.z > m_zMax) ||
            phi > m_phiMax) {
            if (tShapeHit == t1) return false;
            if (t1.UpperBound() > ray.m_maxT) return false;
            tShapeHit = t1;
            // Compute sphere hit position and $\phi$
            pHit = ray.scale((float)tShapeHit);

            // Refine sphere intersection point
            pHit *= m_radius / Distance(pHit, Vector3f(0, 0, 0));
            if (pHit.x == 0 && pHit.y == 0) pHit.x = 1e-5f * m_radius;
            phi = std::atan2(pHit.y, pHit.x);
            if (phi < 0) phi += 2 * PI;
            if ((m_zMin > -m_radius && pHit.z < m_zMin) ||
                (m_zMax < m_radius && pHit.z > m_zMax) || phi > m_phiMax)
                return false;
        }
        return true;
    }

    float Sphere::area() const
	{
		return m_phiMax * m_radius * (m_zMax - m_zMin);
	}

    Interaction Sphere::sample(const Vector2f& u, float* pdf) const
    {
        Vector3f pObj = Vector3f(0, 0, 0) + m_radius * UniformSampleSphere(u);
        Interaction it;
		it.m_n = Normalize( m_objectToWorld->transformPoint(pObj));
        if (m_reverseOrientation) it.m_n *= -1;
        // Reproject _pObj_ to sphere surface and compute _pObjError_
        pObj *= m_radius / Distance(pObj, Vector3f(0, 0, 0));
        Vector3f pObjError = gamma(5) *  Abs( pObj);
		it.m_p = m_objectToWorld->transformPoint(pObj, pObjError, &it.m_pError); 
        *pdf = 1 / area();
        return it;
    }

    Interaction Sphere::sample(const Interaction& ref, const Vector2f& u, float* pdf) const
    {
		Vector3f pCenter = m_objectToWorld->transformPoint(Vector3f(0.0f)); 

		// Sample uniformly on sphere if $\pt{}$ is inside it
		Vector3f pOrigin =
			OffsetRayOrigin(ref.m_p, ref.m_pError, ref.m_n, pCenter - ref.m_p);
		if (DistanceSqr(pOrigin, pCenter) <= m_radius * m_radius) {
			Interaction intr = sample(u, pdf);
			Vector3f wi = intr.m_p - ref.m_p;
			if (LengthSqr(wi) == 0)
				*pdf = 0;
			else {
				// Convert from area measure returned by Sample() call above to
				// solid angle measure.
				wi = Normalize(wi);
				*pdf *= DistanceSqr(ref.m_p, intr.m_p) / AbsDot(intr.m_n, -wi);
			}
			if (std::isinf(*pdf)) *pdf = 0.f;
			return intr;
		}

		// Sample sphere uniformly inside subtended cone

		// Compute coordinate system for sphere sampling
        float dc = Distance(ref.m_p, pCenter);
        float invDc = 1 / dc;
		Vector3f wc = (pCenter - ref.m_p) * invDc;
		Vector3f wcX, wcY;
		CoordinateSystem(wc, &wcX, &wcY);

		// Compute $\theta$ and $\phi$ values for sample in cone
        float sinThetaMax = m_radius * invDc;
        float sinThetaMax2 = sinThetaMax * sinThetaMax;
        float invSinThetaMax = 1 / sinThetaMax;
        float cosThetaMax = std::sqrt(std::max((float)0.f, 1 - sinThetaMax2));

        float cosTheta = (cosThetaMax - 1) * u[0] + 1;
        float sinTheta2 = 1 - cosTheta * cosTheta;

		if (sinThetaMax2 < 0.00068523f /* sin^2(1.5 deg) */) {
			/* Fall back to a Taylor series expansion for small angles, where
			   the standard approach suffers from severe cancellation errors */
			sinTheta2 = sinThetaMax2 * u[0];
			cosTheta = std::sqrt(1 - sinTheta2);
		}

		// Compute angle $\alpha$ from center of sphere to sampled point on surface
		float cosAlpha = sinTheta2 * invSinThetaMax +
			cosTheta * std::sqrt(std::max(0.f, 1.f - sinTheta2 * invSinThetaMax * invSinThetaMax));
		float sinAlpha = std::sqrt(std::max(0.f, 1.f - cosAlpha * cosAlpha));
		float phi = u[1] * 2 * PI;

		// Compute surface normal and sampled point on sphere
		Vector3f nWorld = SphericalDirection(sinAlpha, cosAlpha, phi, -wcX, -wcY, -wc);
		Vector3f pWorld = pCenter + m_radius * Vector3f(nWorld.x, nWorld.y, nWorld.z);

		// Return _Interaction_ for sampled point on sphere
		Interaction it;
		it.m_p = pWorld;
        it.m_pError = gamma(5) * Abs(pWorld);
		it.m_n = nWorld;
		if (m_reverseOrientation) 
			it.m_n *= -1;

		// Uniform cone PDF.
		*pdf = 1 / (2 * PI * (1 - cosThetaMax));

		return it;
    }



    float Sphere::pdf(const Interaction& ref, const Vector3f& wi) const
    {
        auto pCenter = m_objectToWorld->transformPoint(Vector3f(0, 0, 0));
        // Return uniform PDF if point is inside sphere
         Vector3f pOrigin =
            OffsetRayOrigin(ref.m_p, ref.m_pError, ref.m_n, pCenter - ref.m_p);
        if (DistanceSqr(pOrigin, pCenter) <= m_radius * m_radius)
            return Shape::pdf(ref, wi);

        // Compute general sphere PDF
        float sinThetaMax2 = m_radius * m_radius / DistanceSqr(ref.m_p, pCenter);
        float cosThetaMax = std::sqrt(std::max((float)0, 1 - sinThetaMax2));
        return UniformConePdf(cosThetaMax);
    }

    float Sphere::solidAngle(const Vector3f& p, int nSamples) const
    {
        auto pCenter = m_objectToWorld->transformPoint(Vector3f(0, 0, 0));
        if (DistanceSqr(p, pCenter) <= m_radius * m_radius)
            return 4 * PI;
        float sinTheta2 = m_radius * m_radius / DistanceSqr(p, pCenter);
        float cosTheta = std::sqrt(std::max((float)0, 1.0f - sinTheta2));
        return (2 * PI * (1 - cosTheta));
    }

    Disk::Disk(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, 
        float _radius, float _innerRadius /*= 0.f*/, float _phiMax /*= 0.f*/, float _height /*= 0.f*/)
		: Shape(_o2w, _w2o, _reverseOrientation)
		, m_radius(_radius)
		, m_innerRadius(_innerRadius)
		, m_phiMax(ToRadians(std::clamp(_phiMax, 0.0f, 360.0f)))
		, m_height(_height)
	{

	}


    BBox3f Disk::objectBound() const
	{
		return BBox3f(Vector3f(-m_radius, -m_radius, m_height), 
			          Vector3f(m_radius, m_radius, m_height));
	}

    bool Disk::intersect(const Ray& _ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    {
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
        Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);

        // Compute plane intersection for disk

        // Reject disk intersections for rays parallel to the disk's plane
        if (ray.m_dir.z == 0) return false;
        float tShapeHit = (m_height - ray.m_origin.z) / ray.m_dir.z;
        if (tShapeHit <= 0 || tShapeHit >= ray.m_maxT) return false;

        // See if hit point is inside disk radii and $\phimax$
        Vector3f pHit = ray.scale(tShapeHit);
        float dist2 = pHit.x * pHit.x + pHit.y * pHit.y;
        if (dist2 > m_radius * m_radius || dist2 < m_innerRadius * m_innerRadius)
            return false;

        // Test disk $\phi$ value against $\phimax$
        float phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 * PI;
        if (phi > m_phiMax) return false;
        return true;

        // Find parametric representation of disk hit
        float u = phi / m_phiMax;
        float rHit = std::sqrt(dist2);
        float v = (m_radius - rHit) / (m_radius - m_innerRadius);
        Vector3f dpdu(-m_phiMax * pHit.y, m_phiMax * pHit.x, 0);
        Vector3f dpdv =
            Vector3f(pHit.x, pHit.y, 0.) * (m_innerRadius - m_radius) / rHit;
        
        Vector3f dndu(0, 0, 0), 
                 dndv(0, 0, 0);

        // Refine disk intersection point
        pHit.z = m_height;

        // Compute error bounds for disk intersection
        Vector3f pError(0, 0, 0);

        // Initialize _SurfaceInteraction_ from parametric information
        *isect = m_objectToWorld->transformSurfInteraction(SurfaceInteraction(pHit, pError, Vector2f(u, v),
            -ray.m_dir, dpdu, dpdv, dndu, dndv,
            ray.m_time, this));

        // Update _tHit_ for quadric intersection
        *tHit = (float)tShapeHit;
        return true;
    }


    bool Disk::intersectP(const Ray& _ray, bool _testAlphaTexture) const
    {
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
        Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);

        // Compute plane intersection for disk

        // Reject disk intersections for rays parallel to the disk's plane
        if (ray.m_dir.z == 0) return false;
        float tShapeHit = (m_height - ray.m_origin.z) / ray.m_dir.z;
        if (tShapeHit <= 0 || tShapeHit >= ray.m_maxT) return false;

        // See if hit point is inside disk radii and $\phimax$
        Vector3f pHit = ray.scale(tShapeHit);
        float dist2 = pHit.x * pHit.x + pHit.y * pHit.y;
        if (dist2 > m_radius * m_radius || dist2 < m_innerRadius * m_innerRadius)
            return false;

        // Test disk $\phi$ value against $\phimax$
        float phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 *PI;
        if (phi > m_phiMax) return false;
        return true;
    }

    float Disk::area() const
	{
		return m_phiMax * 0.5f * (m_radius * m_radius - m_innerRadius * m_innerRadius);
	}

    Interaction Disk::sample(const Vector2f& u, float* pdf) const
    {
        Vector2f pd = ConcentricSampleDisk(u);
        Vector3f pObj(pd.x * m_radius, pd.y * m_radius, m_height);
        Interaction it;
        it.m_n = Normalize(m_objectToWorld->transformNormal(Vector3f(0, 0, 1)));
        if (m_reverseOrientation) it.m_n *= -1;
        it.m_p = m_objectToWorld->transformPoint(pObj, Vector3f(0, 0, 0), &it.m_pError);
        *pdf = 1 / area();
        return it;
    }

	Cylinder::Cylinder(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, 
		float _radius, float _zMin, float _zMax, float _phiMax)
		: Shape(_o2w, _w2o, _reverseOrientation)
		, m_radius(_radius)
		, m_phiMax(ToRadians(std::clamp(_phiMax, 0.0f, 360.0f)))
		, m_zMin(_zMin)
		, m_zMax(_zMax)
	{

	}
      


    BBox3f Cylinder::objectBound() const
	{
		return BBox3f(Vector3f(-m_radius, -m_radius, m_zMin), Vector3f(m_radius, m_radius, m_zMax));
	}	

    bool Cylinder::intersect(const Ray& _ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    {
        float phi;
        Vector3f pHit;
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
        Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);

        // Compute quadratic cylinder coefficients

        // Initialize _EFloat_ ray coordinate values
        EFloat ox(ray.m_origin.x, oErr.x), oy(ray.m_origin.y, oErr.y), oz(ray.m_origin.z, oErr.z);
        EFloat dx(ray.m_dir.x, dErr.x), dy(ray.m_dir.y, dErr.y), dz(ray.m_dir.z, dErr.z);
        EFloat a = dx * dx + dy * dy;
        EFloat b = 2 * (dx * ox + dy * oy);
        EFloat c = ox * ox + oy * oy - EFloat(m_radius) * EFloat(m_radius);

        // Solve quadratic equation for _t_ values
        EFloat t0, t1;
        if (!Quadratic(a, b, c, &t0, &t1)) return false;

        // Check quadric shape _t0_ and _t1_ for nearest intersection
        if (t0.UpperBound() > ray.m_maxT || t1.LowerBound() <= 0) return false;
        EFloat tShapeHit = t0;
        if (tShapeHit.LowerBound() <= 0) {
            tShapeHit = t1;
            if (tShapeHit.UpperBound() > ray.m_maxT) return false;
        }

        // Compute cylinder hit point and $\phi$
        pHit = ray.scale((float)tShapeHit);

        // Refine cylinder intersection point
        float hitRad = std::sqrt(pHit.x * pHit.x + pHit.y * pHit.y);
        pHit.x *= m_radius / hitRad;
        pHit.y *= m_radius / hitRad;
        phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 * PI;

        // Test cylinder intersection against clipping parameters
        if (pHit.z < m_zMin || pHit.z > m_zMax || phi > m_phiMax) {
            if (tShapeHit == t1) return false;
            tShapeHit = t1;
            if (t1.UpperBound() > ray.m_maxT) return false;
            // Compute cylinder hit point and $\phi$
            pHit = ray.scale((float)tShapeHit);

            // Refine cylinder intersection point
            float hitRad = std::sqrt(pHit.x * pHit.x + pHit.y * pHit.y);
            pHit.x *= m_radius / hitRad;
            pHit.y *= m_radius / hitRad;
            phi = std::atan2(pHit.y, pHit.x);
            if (phi < 0) phi += 2 * PI;
            if (pHit.z < m_zMin || pHit.z > m_zMax || phi > m_phiMax) return false;
        }

        // Find parametric representation of cylinder hit
        float u = phi / m_phiMax;
        float v = (pHit.z - m_zMin) / (m_zMax - m_zMin);

        // Compute cylinder $\dpdu$ and $\dpdv$
        Vector3f dpdu(-m_phiMax * pHit.y, m_phiMax * pHit.x, 0);
        Vector3f dpdv(0, 0, m_zMax - m_zMin);

        // Compute cylinder $\dndu$ and $\dndv$
        Vector3f d2Pduu = -m_phiMax * m_phiMax * Vector3f(pHit.x, pHit.y, 0);
        Vector3f d2Pduv(0, 0, 0), d2Pdvv(0, 0, 0);

        // Compute coefficients for fundamental forms
        float E = Dot(dpdu, dpdu);
        float F = Dot(dpdu, dpdv);
        float G = Dot(dpdv, dpdv);
        Vector3f N = Normalize(Cross(dpdu, dpdv));
        float e = Dot(N, d2Pduu);
        float f = Dot(N, d2Pduv);
        float g = Dot(N, d2Pdvv);

        // Compute $\dndu$ and $\dndv$ from fundamental form coefficients
        float invEGF2 = 1 / (E * G - F * F);
        Vector3f dndu = Vector3f((f * F - e * G) * invEGF2 * dpdu +
            (e * F - f * E) * invEGF2 * dpdv);
        Vector3f dndv = Vector3f((g * F - f * G) * invEGF2 * dpdu +
            (f * F - g * E) * invEGF2 * dpdv);

        // Compute error bounds for cylinder intersection
        Vector3f pError = gamma(3) * Abs(Vector3f(pHit.x, pHit.y, 0));

        // Initialize _SurfaceInteraction_ from parametric information
        *isect = m_objectToWorld->transformSurfInteraction(SurfaceInteraction(pHit, pError, Vector2f(u, v),
            -ray.m_dir, dpdu, dpdv, dndu, dndv,
            ray.m_time, this));

        // Update _tHit_ for quadric intersection
        *tHit = (float)tShapeHit;
        return true;
    }

    bool Cylinder::intersectP(const Ray& _ray, bool testAlphaTexture ) const
	{
        float phi;
        Vector3f pHit;
        // Transform _Ray_ to object space
        Vector3f oErr, dErr;
        Ray ray = m_worldToObject->transformRay(_ray, &oErr, &dErr);

        // Compute quadratic cylinder coefficients

        // Initialize _EFloat_ ray coordinate values
        EFloat ox(ray.m_origin.x, oErr.x), oy(ray.m_origin.y, oErr.y), oz(ray.m_origin.z, oErr.z);
        EFloat dx(ray.m_dir.x, dErr.x), dy(ray.m_dir.y, dErr.y), dz(ray.m_dir.z, dErr.z);
        EFloat a = dx * dx + dy * dy;
        EFloat b = 2 * (dx * ox + dy * oy);
        EFloat c = ox * ox + oy * oy - EFloat(m_radius) * EFloat(m_radius);

        // Solve quadratic equation for _t_ values
        EFloat t0, t1;
        if (!Quadratic(a, b, c, &t0, &t1)) return false;

        // Check quadric shape _t0_ and _t1_ for nearest intersection
        if (t0.UpperBound() > ray.m_maxT || t1.LowerBound() <= 0) return false;
        EFloat tShapeHit = t0;
        if (tShapeHit.LowerBound() <= 0) {
            tShapeHit = t1;
            if (tShapeHit.UpperBound() > ray.m_maxT) return false;
        }

        // Compute cylinder hit point and $\phi$
        pHit = ray.scale((float)tShapeHit);

        // Refine cylinder intersection point
        float hitRad = std::sqrt(pHit.x * pHit.x + pHit.y * pHit.y);
        pHit.x *= m_radius / hitRad;
        pHit.y *= m_radius / hitRad;
        phi = std::atan2(pHit.y, pHit.x);
        if (phi < 0) phi += 2 * PI;

        // Test cylinder intersection against clipping parameters
        if (pHit.z < m_zMin || pHit.z > m_zMax || phi > m_phiMax) {
            if (tShapeHit == t1) return false;
            tShapeHit = t1;
            if (t1.UpperBound() > ray.m_maxT) return false;
            // Compute cylinder hit point and $\phi$
            pHit = ray.scale((float)tShapeHit);

            // Refine cylinder intersection point
            float hitRad = std::sqrt(pHit.x * pHit.x + pHit.y * pHit.y);
            pHit.x *= m_radius / hitRad;
            pHit.y *= m_radius / hitRad;
            phi = std::atan2(pHit.y, pHit.x);
            if (phi < 0) phi += 2 * PI;
            if (pHit.z < m_zMin || pHit.z > m_zMax || phi > m_phiMax) return false;
        }
        return true;		
	}

    

    float Cylinder::area() const
	{
		return (m_zMax - m_zMin) * m_phiMax * m_radius;
	}

    Interaction Cylinder::sample(const Vector2f& u, float* pdf) const
    {
        float z = Lerp(m_zMin, m_zMax, u[0]);
        float phi = u[1] * m_phiMax;
        Vector3f pObj = Vector3f(m_radius * std::cos(phi), m_radius * std::sin(phi), z);
        Interaction it;
        it.m_n = Normalize(m_objectToWorld->transformNormal( Vector3f( pObj.x, pObj.x, 0)));
        if (m_reverseOrientation) it.m_n *= -1;
        // Reproject _pObj_ to cylinder surface and compute _pObjError_
        float hitRad = std::sqrt(pObj.x * pObj.x + pObj.y * pObj.y);
        pObj.x *= m_radius / hitRad;
        pObj.y *= m_radius / hitRad;
        Vector3f pObjError = gamma(3) * Abs( Vector3f(pObj.x, pObj.y, 0) );
        it.m_p = m_objectToWorld->transformPoint(pObj, pObjError, &it.m_pError);
        *pdf = 1 / area();
        return it;
    }


    //Cone::Cone(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, 
    //    float _radius, float _phiMax /*= 0.f*/, float _height /*= 0.f*/)
    //    : Shape(_o2w, _w2o, _reverseOrientation)
    //    , m_radius(_radius )
    //    , m_phiMax(_phiMax)
    //    , m_height(_height)
    //{

    //}

    //BBox3f Cone::objectBound() const
    //{
    //    return BBox3f(Vector3f(-m_radius, -m_radius, m_height),
    //                  Vector3f(m_radius, m_radius, m_height));
    //}

    //bool Cone::intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    //{

    //}

    //bool Cone::intersectP(const Ray& ray, bool testAlphaTexture) const
    //{

    //}

    //float Cone::area() const
    //{
    //    return m_radius * std::sqrt((m_height * m_height) + (m_radius * m_radius)) * m_phiMax / 2.0f;
    //}

    //Interaction Cone::sample(const Vector2f& u, float* pdf) const
    //{

    //}


    //BBox3f Paraboloid::objectBound() const
    //{

    //}

    //bool Paraboloid::intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    //{

    //}

    //bool Paraboloid::intersectP(const Ray& ray, bool testAlphaTexture) const
    //{

    //}

    //float Paraboloid::area() const
    //{

    //}

    //Interaction Paraboloid::sample(const Vector2f& u, float* pdf) const
    //{

    //}

    //BBox3f Hyperboloid::objectBound() const
    //{

    //}

    //bool Hyperboloid::intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, bool testAlphaTexture) const
    //{

    //}

    //bool Hyperboloid::intersectP(const Ray& ray, bool testAlphaTexture) const
    //{

    //}

    //float Hyperboloid::area() const
    //{

    //}

    //Interaction Hyperboloid::sample(const Vector2f& u, float* pdf) const
    //{

    //}


    std::shared_ptr<Shape> CreateSphereShape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, const ParamSet& _set)
    {
		//return nullptr;
        float radius = _set.FindOneFloat("radius", 1.f);
        float zmin   = _set.FindOneFloat("zmin", -radius);
        float zmax   = _set.FindOneFloat("zmax", radius);
        float phimax = _set.FindOneFloat("phimax", 360.f);
        return std::make_shared<Sphere>(_o2w, _w2o, _reverseOrientation, radius, zmin,
            zmax, phimax);
    }

    std::shared_ptr<Shape> CreateCylinderShape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, const ParamSet& _set)
    {
        float radius = _set.FindOneFloat("radius", 1);
        float zmin   = _set.FindOneFloat("zmin", -1);
        float zmax   = _set.FindOneFloat("zmax", 1);
        float phimax = _set.FindOneFloat("phimax", 360);
        return std::make_shared<Cylinder>(_o2w, _w2o, _reverseOrientation, radius,
            zmin, zmax, phimax);
    }

    std::shared_ptr<Shape> CreateDiskShape(const Transform* _o2w, const Transform* _w2o, bool _reverseOrientation, const ParamSet& _set)
    {
        float height = _set.FindOneFloat("height", 0.);
        float radius = _set.FindOneFloat("radius", 1);
        float inner_radius = _set.FindOneFloat("innerradius", 0);
        float phimax = _set.FindOneFloat("phimax", 360);
        return std::make_shared<Disk>(_o2w, _w2o, _reverseOrientation, height, radius, inner_radius, phimax);
    }
}

