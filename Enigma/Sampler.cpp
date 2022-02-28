#include "SamplerInclude.h"

namespace RayTrace
{

   

    //////////////////////////////////////////////////////////////////////////
    //SamplerBase
    //////////////////////////////////////////////////////////////////////////
    Sampler::Sampler(int64_t samplesPerPixel)
        : m_samplesPerPixel(samplesPerPixel)
    {

    }

    Sampler::~Sampler()
    {

    }

    void Sampler::StartPixel(const Vector2i& p)
    {
        m_currentPixel = p;
        m_currentPixelSampleIndex = 0;
        // Reset array offsets for next pixel sample
        m_array1DOffset = m_array2DOffset = 0;
    }

    CameraSample Sampler::GetCameraSample(const Vector2i& pRaster)
    {
        CameraSample cs;
        cs.m_image = Vector2f( pRaster.x, pRaster.y ) + Get2D();
        cs.m_time = Get1D();
        cs.m_lens = Get2D();
        return cs;
    }

    void Sampler::Request1DArray(int n)
    {
        assert(RoundCount(n) == n);
        m_samples1DArraySizes.push_back(n);
        m_sampleArray1D.push_back(std::vector<float>(n * m_samplesPerPixel));
    }

    void Sampler::Request2DArray(int n)
    {
        assert(RoundCount(n) == n);
        m_samples2DArraySizes.push_back(n);
        m_sampleArray2D.push_back(std::vector<Vector2f>(n * m_samplesPerPixel));
    }

    const float* Sampler::Get1DArray(int n)
    {
        if (m_array1DOffset == m_sampleArray1D.size()) 
            return nullptr;
        assert(m_samples1DArraySizes[m_array1DOffset] == n);
        assert(m_currentPixelSampleIndex < m_samplesPerPixel);
        return &m_sampleArray1D[m_array1DOffset++][m_currentPixelSampleIndex * n];
    }

    const Vector2f* Sampler::Get2DArray(int n)
    {
        if (m_array1DOffset == m_sampleArray2D.size())
            return nullptr;
        assert(m_samples1DArraySizes[m_array2DOffset] == n);
        assert(m_currentPixelSampleIndex < m_samplesPerPixel);
        return &m_sampleArray2D[m_array2DOffset++][m_currentPixelSampleIndex * n];
    }

    bool Sampler::StartNextSample()
    {
        m_array1DOffset = m_array2DOffset = 0;
        return ++m_currentPixelSampleIndex < m_samplesPerPixel;
    }

    bool Sampler::SetSampleNumber(int64_t sampleNum)
    {
        // Reset array offsets for next pixel sample
        m_array1DOffset = m_array2DOffset = 0;
        m_currentPixelSampleIndex = sampleNum;
        return m_currentPixelSampleIndex < m_samplesPerPixel;
    }


    //////////////////////////////////////////////////////////////////////////
    //PixelSampler
    //////////////////////////////////////////////////////////////////////////
    PixelSampler::PixelSampler(int64_t samplesPerPixel, int nSampledDimensions)
        : Sampler(samplesPerPixel)
    {
        for (int i = 0; i < nSampledDimensions; ++i) {
            m_samples1D.push_back(FloatVector(samplesPerPixel));
            m_samples2D.push_back(Vec2fVector(samplesPerPixel));
        }
    }

    PixelSampler::PixelSampler(const PixelSampler& _rhs)
        : Sampler( _rhs.m_samplesPerPixel )
        , m_samples1D( _rhs.m_samples1D )
        , m_samples2D( _rhs.m_samples2D )
        , m_current1DDimension( _rhs.m_current1DDimension )
        , m_current2DDimension( _rhs.m_current2DDimension )
    {        
    }

    bool PixelSampler::StartNextSample()
    {
        m_current1DDimension = m_current2DDimension = 0;
        return Sampler::StartNextSample();
    }

    bool PixelSampler::SetSampleNumber(int64_t _num)
    {
        m_current1DDimension = m_current2DDimension = 0;
        return Sampler::SetSampleNumber(_num);
    }

    float PixelSampler::Get1D()
    {
        if (m_current1DDimension < m_samples1D.size())
            return m_samples1D[m_current1DDimension++][m_currentPixelSampleIndex];
        else
            return m_rng.randomFloat(0.f, OneMinusEpsilon );
    }

    Vector2f PixelSampler::Get2D()
    {
        if (m_current2DDimension < m_samples2D.size())
            return m_samples2D[m_current2DDimension++][m_currentPixelSampleIndex];
        else
            return  Vector2f(m_rng.randomFloat(0.f, OneMinusEpsilon), m_rng.randomFloat(0.f, OneMinusEpsilon) );
    }


    //////////////////////////////////////////////////////////////////////////
    //GlobalSampler
    //////////////////////////////////////////////////////////////////////////   
    GlobalSampler::GlobalSampler(int64_t samplesPerPixel) 
        : Sampler(samplesPerPixel)
    {

    }

    bool GlobalSampler::StartNextSample()
    {
        m_dimension = 0;
        m_intervalSampleIndex = GetIndexForSample(m_currentPixelSampleIndex + 1);
        return Sampler::StartNextSample();
    }

    void GlobalSampler::StartPixel(const Vector2i& p)
    {
        Sampler::StartPixel(p);
        m_dimension = 0;
        m_intervalSampleIndex = GetIndexForSample(0);
        // Compute _arrayEndDim_ for dimensions used for array samples
        m_arrayEndDim =
            s_arrayStartDim + m_sampleArray1D.size() + 2 * m_sampleArray2D.size();

        // Compute 1D array samples for _GlobalSampler_
        for (size_t i = 0; i < m_samples1DArraySizes.size(); ++i) {
            int nSamples = m_samples1DArraySizes[i] * m_samplesPerPixel;
            for (int j = 0; j < nSamples; ++j) {
                int64_t index = GetIndexForSample(j);
                m_sampleArray1D[i][j] = SampleDimension(index, s_arrayStartDim + i);
            }
        }

        // Compute 2D array samples for _GlobalSampler_
        int dim = s_arrayStartDim + m_samples1DArraySizes.size();
        for (size_t i = 0; i < m_samples2DArraySizes.size(); ++i) {
            int nSamples = m_samples2DArraySizes[i] * m_samplesPerPixel;
            for (int j = 0; j < nSamples; ++j) {
                int64_t idx = GetIndexForSample(j);
                m_sampleArray2D[i][j].x = SampleDimension(idx, dim);
                m_sampleArray2D[i][j].y = SampleDimension(idx, dim + 1);
            }
            dim += 2;
        }
        assert(m_arrayEndDim ==  dim);
    }

    bool GlobalSampler::SetSampleNumber(int64_t sampleNum)
    {
        m_dimension = 0;
        m_intervalSampleIndex = GetIndexForSample(sampleNum);
        return Sampler::SetSampleNumber(sampleNum);
    }

    float GlobalSampler::Get1D()
    {
        if (m_dimension >= s_arrayStartDim && m_dimension < m_arrayEndDim)
            m_dimension = m_arrayEndDim;
        return SampleDimension(m_intervalSampleIndex, m_dimension++);
    }

    Vector2f GlobalSampler::Get2D()
    {
        if (m_dimension + 1 >= s_arrayStartDim && m_dimension < m_arrayEndDim)
            m_dimension = m_arrayEndDim;
       Vector2f p(SampleDimension(m_intervalSampleIndex, m_dimension),
                 SampleDimension(m_intervalSampleIndex, m_dimension + 1));
        m_dimension += 2;
        return p;
    }

    //////////////////////////////////////////////////////////////////////////
    //SobolSampler
    //////////////////////////////////////////////////////////////////////////
    SobolSampler::SobolSampler(int64_t samplesPerPixel, const BBox2i& sampleBounds) 
        : GlobalSampler(RoundUpPow2(samplesPerPixel))
        , m_sampleBounds(sampleBounds)
    {
        if (!IsPowerOf2(m_samplesPerPixel)) {
            Warning("Non power-of-two sample count rounded up to % for SobolSampler", this->m_samplesPerPixel);
        }
        m_resolution = RoundUpPow2( std::max(m_sampleBounds.diagonal().x, m_sampleBounds.diagonal().y ) );
        m_log2Resolution = Log2Int(m_resolution);
        if (m_resolution > 0) 
            assert(1 << m_log2Resolution == m_resolution );
    }

    int64_t SobolSampler::GetIndexForSample(int64_t sampleNum) const
    {
        return SobolIntervalToIndex( m_log2Resolution, sampleNum, Vector2i( m_currentPixel - m_sampleBounds.m_min ) );
    }

    float SobolSampler::SampleDimension(int64_t index, int dimension) const
    {
       assert(dimension < NumSobolDimensions);
       
        float s = SobolSample(index, dimension);
        // Remap Sobol$'$ dimensions used for pixel samples
        if (dimension == 0 || dimension == 1) {
            s = s * m_resolution + m_sampleBounds.m_min[dimension];
            s =  std::clamp( s - m_currentPixel[dimension], 0.f, OneMinusEpsilon);
        }
        return s;
    }

    std::unique_ptr<Sampler> SobolSampler::Clone(int seed) const
    {
        return std::unique_ptr<Sampler>(new SobolSampler(*this));
    }




   

    //////////////////////////////////////////////////////////////////////////
    //StratifiedSampler3
    //////////////////////////////////////////////////////////////////////////

    StratifiedSampler::StratifiedSampler(int xPixelSamples, int yPixelSamples, bool jitterSamples, int nSampledDimensions) 
        : PixelSampler(xPixelSamples* yPixelSamples, nSampledDimensions),
        m_xPixelSamples(xPixelSamples),
        m_yPixelSamples(yPixelSamples),
        m_jitterSamples(jitterSamples)
    {

    }
    void StratifiedSampler::StartPixel(const Vector2i& p)
    {
        // Generate single stratified samples for the pixel
        for (size_t i = 0; i < m_samples1D.size(); ++i) {
            StratifiedSample1D(&m_samples1D[i][0], m_xPixelSamples * m_yPixelSamples, m_rng,
                m_jitterSamples);
            Shuffle(&m_samples1D[i][0], m_xPixelSamples * m_yPixelSamples, 1, m_rng);
        }
        for (size_t i = 0; i < m_samples2D.size(); ++i) {
            StratifiedSample2D(&m_samples2D[i][0], m_xPixelSamples, m_yPixelSamples, m_rng,
                m_jitterSamples);
            Shuffle(&m_samples2D[i][0], m_xPixelSamples * m_yPixelSamples, 1, m_rng);
        }

        // Generate arrays of stratified samples for the pixel
        for (size_t i = 0; i < m_samples1DArraySizes.size(); ++i)
            for (int64_t j = 0; j < m_samplesPerPixel; ++j) {
                int count = m_samples1DArraySizes[i];
                StratifiedSample1D(&m_sampleArray1D[i][j * count], count, m_rng,
                    m_jitterSamples);
                Shuffle(&m_sampleArray1D[i][j * count], count, 1, m_rng);
            }
        for (size_t i = 0; i < m_samples2DArraySizes.size(); ++i)
            for (int64_t j = 0; j < m_samplesPerPixel; ++j) {
                int count = m_samples2DArraySizes[i];
                LatinHypercube(&m_sampleArray2D[i][j * count].x, count, 2, m_rng);
            }
        PixelSampler::StartPixel(p);
    }

    std::unique_ptr<Sampler> StratifiedSampler::Clone(int seed) const
    {
        auto s = std::make_unique<StratifiedSampler>(*this);
        s->m_rng.setSeed(seed);
        return s;
    }

    //////////////////////////////////////////////////////////////////////////
    //HaltonSampler3
    //////////////////////////////////////////////////////////////////////////

    HaltonSampler::HaltonSampler(int nsamp, const BBox2i& sampleBounds, bool sampleAtCenter /*= false*/)
        : GlobalSampler(nsamp)
        , m_sampleAtPixelCenter(sampleAtCenter)
    {
        // Generate random digit permutations for Halton sampler
        if (s_radicalInversePermutations.empty()) {
            RNG rng;
            s_radicalInversePermutations = ComputeRadicalInversePermutations(rng);
        }

        // Find radical inverse base scales and exponents that cover sampling area
        Vector2i res = sampleBounds.m_max - sampleBounds.m_min;
        for (int i = 0; i < 2; ++i) {
            int base = (i == 0) ? 2 : 3;
            int scale = 1, exp = 0;
            while (scale < std::min(res[i], kMaxResolution)) {
                scale *= base;
                ++exp;
            }
            m_baseScales[i] = scale;
            m_baseExponents[i] = exp;
        }

        // Compute stride in samples for visiting each pixel area
        m_sampleStride = m_baseScales[0] * m_baseScales[1];

        // Compute multiplicative inverses for _baseScales_
        m_multInverse[0] = multiplicativeInverse(m_baseScales[1], m_baseScales[0]);
        m_multInverse[1] = multiplicativeInverse(m_baseScales[0], m_baseScales[1]);
    }

    int64_t HaltonSampler::GetIndexForSample(int64_t sampleNum) const
    {
        if (m_currentPixel != m_pixelForOffset) {
            // Compute Halton sample offset for _currentPixel_
            m_offsetForCurrentPixel = 0;
            if (m_sampleStride > 1) {
                Vector2i pm(Mod(m_currentPixel[0], kMaxResolution),
                            Mod(m_currentPixel[1], kMaxResolution));
                for (int i = 0; i < 2; ++i) {
                    uint64_t dimOffset =
                        (i == 0)
                        ? InverseRadicalInverse<2>(pm[i], m_baseExponents[i])
                        : InverseRadicalInverse<3>(pm[i], m_baseExponents[i]);
                    m_offsetForCurrentPixel += dimOffset * (m_sampleStride / m_baseScales[i]) * m_multInverse[i];
                }
                m_offsetForCurrentPixel %= m_sampleStride;
            }
            m_pixelForOffset = m_currentPixel;
        }
        return m_offsetForCurrentPixel + sampleNum * m_sampleStride;
    }

    float HaltonSampler::SampleDimension(int64_t index, int dim) const
    {
        if (m_sampleAtPixelCenter && (dim == 0 || dim == 1)) return 0.5f;
        if (dim == 0)
            return RadicalInverse(dim, index >> m_baseExponents[0]);
        else if (dim == 1)
            return RadicalInverse(dim, index / m_baseScales[1]);
        else
            return ScrambledRadicalInverse(dim, index, PermutationForDimension(dim));
    }

    std::unique_ptr<Sampler> HaltonSampler::Clone(int seed) const
    {
        auto s = std::make_unique<HaltonSampler>(*this);
        return s;
    }

    std::vector<uint16_t> HaltonSampler::s_radicalInversePermutations;

    RandomSampler::RandomSampler(int ns, int seed /*= 0*/)
        : Sampler( ns )
    {
        m_rng.setSeed(seed);
    }

    RandomSampler::RandomSampler(const RandomSampler& _rhs)
        : Sampler( _rhs.m_samplesPerPixel )
    {

    }

    void RandomSampler::StartPixel(const Vector2i& p)
    {
        for (size_t i = 0; i < m_sampleArray1D.size(); ++i)
            for (size_t j = 0; j < m_sampleArray1D[i].size(); ++j)
                m_sampleArray1D[i][j] = m_rng.uniformFloat();

        for (size_t i = 0; i < m_sampleArray2D.size(); ++i)
            for (size_t j = 0; j < m_sampleArray2D[i].size(); ++j)
                m_sampleArray2D[i][j] = { m_rng.uniformFloat(), m_rng.uniformFloat() };
        Sampler::StartPixel(p);
    }

    float RandomSampler::Get1D()
    {
        return m_rng.uniformFloat();
    }

    Vector2f RandomSampler::Get2D()
    {
        return Vector2f(m_rng.uniformFloat(), m_rng.uniformFloat());
    }

    std::unique_ptr<Sampler> RandomSampler::Clone(int seed) const
    {
        auto s = std::make_unique<RandomSampler>(*this);
        s->m_rng.setSeed(seed);
        return s;
    }

    ZeroTwoSequenceSampler::ZeroTwoSequenceSampler(int64_t samplesPerPixel, int nSampledDimensions /*= 4*/)
        : PixelSampler(RoundUpPow2(samplesPerPixel), nSampledDimensions )
    {

    }

    void ZeroTwoSequenceSampler::StartPixel(const Vector2i& p)
    {
        // Generate 1D and 2D pixel sample components using $(0,2)$-sequence
        for (size_t i = 0; i < m_samples1D.size(); ++i)
            VanDerCorput(1, m_samplesPerPixel, &m_samples1D[i][0], m_rng);
        for (size_t i = 0; i < m_samples2D.size(); ++i)
            Sobol2D(1, m_samplesPerPixel, &m_samples2D[i][0], m_rng);

        // Generate 1D and 2D array samples using $(0,2)$-sequence
        for (size_t i = 0; i < m_samples1DArraySizes.size(); ++i)
            VanDerCorput(m_samples1DArraySizes[i], m_samplesPerPixel, &m_sampleArray1D[i][0], m_rng);
        for (size_t i = 0; i < m_samples2DArraySizes.size(); ++i)
            Sobol2D(m_samples2DArraySizes[i], m_samplesPerPixel, &m_sampleArray2D[i][0],
                m_rng);
        PixelSampler::StartPixel(p);
    }

    std::unique_ptr<Sampler> ZeroTwoSequenceSampler::Clone(int seed) const
    {
        auto s = std::make_unique<ZeroTwoSequenceSampler>(*this);
        s->m_rng.setSeed(seed);
        return s;
    }

    int ZeroTwoSequenceSampler::RoundCount(int count) const
    {
        return RoundUpPow2(count);
    }

   

    MaxMinDistSampler::MaxMinDistSampler(int64_t samplesPerPixel, int nSampledDimensions)
        : PixelSampler(GetSPP(samplesPerPixel), nSampledDimensions)
    {
        int Cindex = Log2Int(samplesPerPixel);
        /* CHECK(Cindex >= 0 &&
               Cindex < (sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0])));*/
        m_CPixel = CMaxMinDist[Cindex];
    }

    void MaxMinDistSampler::StartPixel(const Vector2i& p)
    {
        float invSPP = (float)1 / m_samplesPerPixel;
        for (int i = 0; i < m_samplesPerPixel; ++i)
            m_samples2D[0][i] = Vector2f(i * invSPP, SampleGeneratorMatrix(m_CPixel, i));
        Shuffle(&m_samples2D[0][0], m_samplesPerPixel, 1, m_rng);
        // Generate remaining samples for _MaxMinDistSampler_
        for (size_t i = 0; i < m_samples1D.size(); ++i)
            VanDerCorput(1, m_samplesPerPixel, &m_samples1D[i][0], m_rng);

        for (size_t i = 1; i < m_samples2D.size(); ++i)
            Sobol2D(1, m_samplesPerPixel, &m_samples2D[i][0], m_rng);

        for (size_t i = 0; i < m_samples1DArraySizes.size(); ++i) {
            int count = m_samples1DArraySizes[i];
            VanDerCorput(count, m_samplesPerPixel, &m_sampleArray1D[i][0], m_rng);
        }
        for (size_t i = 0; i < m_samples2DArraySizes.size(); ++i) {
            int count = m_samples2DArraySizes[i];
            Sobol2D(count, m_samplesPerPixel, &m_sampleArray2D[i][0], m_rng);
        }
        PixelSampler::StartPixel(p);
    }

    std::unique_ptr<Sampler> MaxMinDistSampler::Clone(int seed) const
    {
        auto s = std::make_unique < MaxMinDistSampler>(*this);
        s->m_rng.setSeed(seed);
        return s;

    }

    int MaxMinDistSampler::RoundCount(int count) const
    {
        return RoundUpPow2(count);
    }

    int MaxMinDistSampler::GetSPP(int64_t spp)
    {
        int Cindex = Log2Int(spp);
        if (Cindex >= sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0])) {
            Warning(
                "No more than %d samples per pixel are supported with "
                "MaxMinDistSampler. Rounding down.",
                (1 << int(sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0]))) -
                1);
            spp = (1 << (sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0]))) - 1;
        }
        if (!IsPowerOf2(spp)) {
            spp = RoundUpPow2(spp);

        }
        return spp;
    }
    

#pragma region CreateSamplers
    Sampler* CreateHaltonSampler(const ParamSet& _params, const Camera& _camera)
    {
        int nsamp = _params.FindOneInt("pixelsamples", 16);
        if (PbrtOptions.quickRender) 
            nsamp = 1;
        bool sampleAtCenter = _params.FindOneBool("samplepixelcenter", false);
        return new HaltonSampler(nsamp, _camera.m_film->GetSampleBounds(), sampleAtCenter);
    }

    Sampler* CreateLowDiscrepancySampler(const ParamSet& _params, const Camera& _camera)
    {
        int nsamp = _params.FindOneInt("pixelsamples", 16);
        int sd    = _params.FindOneInt("dimensions", 4);
        if (PbrtOptions.quickRender) nsamp = 1;
        return new ZeroTwoSequenceSampler(nsamp, sd);
    }

    Sampler* CreateRandomSampler(const ParamSet& _params, const Camera& _camera)
    {
        int ns = _params.FindOneInt("pixelsamples", 4);
        return new RandomSampler(ns);
    }

    Sampler* CreateStratifiedSampler(const ParamSet& _params, const Camera& _camera)
    {
        bool jitter = _params.FindOneBool("jitter", true);
        int xsamp   = _params.FindOneInt("xsamples", 4);
        int ysamp   = _params.FindOneInt("ysamples", 4);
        int sd      = _params.FindOneInt("dimensions", 4);
        if (PbrtOptions.quickRender) xsamp = ysamp = 1;
        return new StratifiedSampler(xsamp, ysamp, jitter, sd);
    }

    Sampler* CreateSobolSampler(const ParamSet& _params, const Camera& _camera)
    {
        int nsamp = _params.FindOneInt("pixelsamples", 16);
        if (PbrtOptions.quickRender) nsamp = 1;
        return new SobolSampler(nsamp, _camera.m_film->GetSampleBounds() );
    }

    Sampler* CreateMaxMinDistSampler(const ParamSet& _params, const Camera& _camera)
    {
        int nsamp = _params.FindOneInt("pixelsamples", 16);
        int sd = _params.FindOneInt("dimensions", 4);
        if (PbrtOptions.quickRender) nsamp = 1;
        return new MaxMinDistSampler(nsamp, sd);
    }

#pragma  endregion
}

