#include <numeric>
#include "MonteCarlo.h"
#include "MathCommon.h"
#include "Scene.h"

#include "BBox.h"
#include "Error.h"
#include "Integrator.h"
#include "Sampler.h"
#include "Lights.h"

#include "LightDist.h"




namespace RayTrace
{

    std::unique_ptr<LightDistribution> CreateLightSampleDistribution(const std::string& name, const Scene& scene)
    {
        if (name == "uniform" || scene.getNumLights() == 1)
            return std::unique_ptr<LightDistribution>{
            new UniformLightDistribution(scene)};
        else if (name == "power")
            return std::unique_ptr<LightDistribution>{
            new PowerLightDistribution(scene)};
        else if (name == "spatial")
            return std::unique_ptr<LightDistribution>{
            new SpatialLightDistribution(scene)};
        else {
            Error(
                "Light sample distribution type \"%s\" unknown. Using \"spatial\".",
                name.c_str());
            return std::unique_ptr<LightDistribution>{
                new SpatialLightDistribution(scene)};
        }
    }

    LightDistribution::~LightDistribution()
    {

    }

    //////////////////////////////////////////////////////////////////////////
    //UniformLightDistribution
    //////////////////////////////////////////////////////////////////////////
    UniformLightDistribution::UniformLightDistribution(const Scene& scene)
    {
        FloatVector prob(scene.getNumLights(), 1.0f);
        m_distrib.reset(new Distribution1D(&prob[0], int(prob.size())));
    }

    const Distribution1D* UniformLightDistribution::Lookup(const Vector3f& _p) const
    {
        return m_distrib.get();
    }

    //////////////////////////////////////////////////////////////////////////
    //PowerLightDistribution
    //////////////////////////////////////////////////////////////////////////
    PowerLightDistribution::PowerLightDistribution(const Scene& scene)
        : m_distrib(ComputeLightPowerDistribution(scene))
    {
       
    }

    const Distribution1D* PowerLightDistribution::Lookup(const Vector3f& _p) const
    {
        return m_distrib.get();
    }


    static const uint64_t invalidPackedPos = 0xffffffffffffffff;


    //////////////////////////////////////////////////////////////////////////
    //SpatialLightDistribution
    //////////////////////////////////////////////////////////////////////////
    SpatialLightDistribution::SpatialLightDistribution(const Scene& scene, int maxVoxels /*= 64*/)
        : m_scene( scene )
        
    {
        // Compute the number of voxels so that the widest scene bounding box
    // dimension has maxVoxels voxels and the other dimensions have a number
    // of voxels so that voxels are roughly cube shaped.
        const auto& b = scene.worldBound();
        Vector3f diag = b.diagonal();
        float bmax    = diag[b.maximumExtent()];
        for (int i = 0; i < 3; ++i) {
            m_nVoxels[i] = std::max(1, int(std::round(diag[i] / bmax * maxVoxels)));
            // In the Lookup() method, we require that 20 or fewer bits be
            // sufficient to represent each coordinate value. It's fairly hard
            // to imagine that this would ever be a problem.
            //CHECK_LT(nVoxels[i], 1 << 20);
        }

        m_hashTableSize = 4 * m_nVoxels[0] * m_nVoxels[1] * m_nVoxels[2];
        m_hashTable.reset(new HashEntry[m_hashTableSize]);
        for (int i = 0; i < m_hashTableSize; ++i) {
            m_hashTable[i].m_packedPos.store(invalidPackedPos);
            m_hashTable[i].m_distribution.store(nullptr);
        }

    }

    SpatialLightDistribution::~SpatialLightDistribution()
    {
        for (size_t i = 0; i < m_hashTableSize; ++i) {
            HashEntry& entry = m_hashTable[i];
            if (entry.m_distribution.load())
                delete entry.m_distribution.load();
        }
    }

    const Distribution1D* SpatialLightDistribution::Lookup(const Vector3f& _p) const
    {
        Vector3f offset = m_scene.worldBound().offset(_p);  // offset in [0,1].
        Vector3i pi;
        for (int i = 0; i < 3; ++i)
            // The clamp should almost never be necessary, but is there to be
            // robust to computed intersection points being slightly outside
            // the scene bounds due to floating-point roundoff error.
            pi[i] = std::clamp(int(offset[i] * m_nVoxels[i]), 0, m_nVoxels[i] - 1);

        // Pack the 3D integer voxel coordinates into a single 64-bit value.
        uint64_t packedPos = (uint64_t(pi[0]) << 40) | (uint64_t(pi[1]) << 20) | pi[2];
        //CHECK_NE(packedPos, invalidPackedPos);

        // Compute a hash value from the packed voxel coordinates.  We could
        // just take packedPos mod the hash table size, but since packedPos
        // isn't necessarily well distributed on its own, it's worthwhile to do
        // a little work to make sure that its bits values are individually
        // fairly random. For details of and motivation for the following, see:
        // http://zimbry.blogspot.ch/2011/09/better-bit-mixing-improving-on.html
        uint64_t hash = packedPos;
        hash ^= (hash >> 31);
        hash *= 0x7fb5d329728ea185;
        hash ^= (hash >> 27);
        hash *= 0x81dadef4bc2dd44d;
        hash ^= (hash >> 33);
        hash %= m_hashTableSize;
        //CHECK_GE(hash, 0);

        // Now, see if the hash table already has an entry for the voxel. We'll
        // use quadratic probing when the hash table entry is already used for
        // another value; step stores the square root of the probe step.
        int step = 1;
        int nProbes = 0;
        while (true) {
            ++nProbes;
            HashEntry& entry = m_hashTable[hash];
            // Does the hash table entry at offset |hash| match the current point?
            uint64_t entryPackedPos = entry.m_packedPos.load(std::memory_order_acquire);
            if (entryPackedPos == packedPos) {
                // Yes! Most of the time, there should already by a light
                // sampling distribution available.
                Distribution1D* dist = entry.m_distribution.load(std::memory_order_acquire);
                if (dist == nullptr) {
                    // Rarely, another thread will have already done a lookup
                    // at this point, found that there isn't a sampling
                    // distribution, and will already be computing the
                    // distribution for the point.  In this case, we spin until
                    // the sampling distribution is ready.  We assume that this
                    // is a rare case, so don't do anything more sophisticated
                    // than spinning.
                  //  ProfilePhase _(Prof::LightDistribSpinWait);
                    while ((dist = entry.m_distribution.load(std::memory_order_acquire)) ==
                        nullptr)
                        // spin :-(. If we were fancy, we'd have any threads
                        // that hit this instead help out with computing the
                        // distribution for the voxel...
                        ;
                }
                // We have a valid sampling distribution.
               // ReportValue(nProbesPerLookup, nProbes);
                return dist;
            }
            else if (entryPackedPos != invalidPackedPos) {
                // The hash table entry we're checking has already been
                // allocated for another voxel. Advance to the next entry with
                // quadratic probing.
                hash += step * step;
                if (hash >= m_hashTableSize)
                    hash %= m_hashTableSize;
                ++step;
            }
            else {
                // We have found an invalid entry. (Though this may have
                // changed since the load into entryPackedPos above.)  Use an
                // atomic compare/exchange to try to claim this entry for the
                // current position.
                uint64_t invalid = invalidPackedPos;
                if (entry.m_packedPos.compare_exchange_weak(invalid, packedPos)) {
                    // Success; we've claimed this position for this voxel's
                    // distribution. Now compute the sampling distribution and
                    // add it to the hash table. As long as packedPos has been
                    // set but the entry's distribution pointer is nullptr, any
                    // other threads looking up the distribution for this voxel
                    // will spin wait until the distribution pointer is
                    // written.
                    Distribution1D* dist = ComputeDistribution(pi);
                    entry.m_distribution.store(dist, std::memory_order_release);
                   // ReportValue(nProbesPerLookup, nProbes);
                    return dist;
                }
            }
        }
    }

    Distribution1D* SpatialLightDistribution::ComputeDistribution(const Vector3i& pi) const
    {
        Vector3f p0(float(pi[0]) / float(m_nVoxels[0]),
                    float(pi[1]) / float(m_nVoxels[1]),
                    float(pi[2]) / float(m_nVoxels[2]));
        Vector3f p1(float(pi[0] + 1) / float(m_nVoxels[0]),
                    float(pi[1] + 1) / float(m_nVoxels[1]),
                    float(pi[2] + 1) / float(m_nVoxels[2]));
        BBox3f voxelBounds(m_scene.worldBound().lerp(p0), m_scene.worldBound().lerp(p1));

        // Compute the sampling distribution. Sample a number of points inside
        // voxelBounds using a 3D Halton sequence; at each one, sample each
        // light source and compute a weight based on Li/pdf for the light's
        // sample (ignoring visibility between the point in the voxel and the
        // point on the light source) as an approximation to how much the light
        // is likely to contribute to illumination in the voxel.
        int nSamples = 128;
        std::vector<float> lightContrib(m_scene.getNumLights(), float(0));
        for (int i = 0; i < nSamples; ++i) {
            Vector3f po = voxelBounds.lerp(Vector3f(
                RadicalInverse(0, i), RadicalInverse(1, i), RadicalInverse(2, i)));
            Interaction intr(po, Vector3f(), Vector3f(), Vector3f(1, 0, 0),
                0 /* time */, MediumInterface());

            // Use the next two Halton dimensions to sample a point on the
            // light source.
            Vector2f u(RadicalInverse(3, i), RadicalInverse(4, i));
            for (size_t j = 0; j < m_scene.getNumLights(); ++j) {
                float pdf;
                Vector3f wi;
                VisibilityTester vis;
                Spectrum Li = m_scene.getLight(j)->Sample_Li(intr, u, &wi, &pdf, &vis);
                if (pdf > 0) {
                    // TODO: look at tracing shadow rays / computing beam
                    // transmittance.  Probably shouldn't give those full weight
                    // but instead e.g. have an occluded shadow ray scale down
                    // the contribution by 10 or something.
                    lightContrib[j] += Li.y() / pdf;
                }
            }
        }

        // We don't want to leave any lights with a zero probability; it's
        // possible that a light contributes to points in the voxel even though
        // we didn't find such a point when sampling above.  Therefore, compute
        // a minimum (small) weight and ensure that all lights are given at
        // least the corresponding probability.
        float sumContrib = std::accumulate(lightContrib.begin(), lightContrib.end(), float(0));
        float avgContrib = sumContrib / (nSamples * lightContrib.size());
        float minContrib = (avgContrib > 0) ? .001 * avgContrib : 1;
        for (size_t i = 0; i < lightContrib.size(); ++i) {
            lightContrib[i] = std::max(lightContrib[i], minContrib);
        }
     
        // Compute a sampling distribution from the accumulated contributions.
        return new Distribution1D(&lightContrib[0], int(lightContrib.size()));
    }

}

