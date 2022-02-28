#pragma once
#include "Defines.h"
#include "BBox.h"
#include "RNG.h"
#include "Sample.h"
#include "Misc.h"

namespace RayTrace
{
    float RadicalInverse(int baseIndex, uint64_t a); //todo move to math
    
    // Sampler Declarations
    class Sampler {
    public:
        // Sampler Interface       
        Sampler(int64_t samplesPerPixel);
        virtual ~Sampler();
        virtual void            StartPixel(const Vector2i& p);
        virtual float           Get1D() = 0;
        virtual Vector2f        Get2D() = 0;
        CameraSample            GetCameraSample(const Vector2i& pRaster);
        void                    Request1DArray(int n);
        void                    Request2DArray(int n);
        virtual int             RoundCount(int n) const { return n; }
        const float*            Get1DArray(int n);
        const Vector2f*         Get2DArray(int n);
        virtual bool            StartNextSample();
        virtual std::unique_ptr<Sampler> Clone(int seed) const = 0;
        virtual bool            SetSampleNumber(int64_t sampleNum);
       
        int64_t                 CurrentSampleNumber() const { return m_currentPixelSampleIndex; }



        // Sampler Public Data
        const int64_t m_samplesPerPixel;

    protected:
        // Sampler Protected Data
        Vector2i   m_currentPixel;
        int64_t          m_currentPixelSampleIndex;
        std::vector<int> m_samples1DArraySizes, 
                         m_samples2DArraySizes;
        std::vector<std::vector<float>>          m_sampleArray1D;
        std::vector<std::vector<Vector2f>> m_sampleArray2D;

    private:
        // Sampler Private Data
        size_t m_array1DOffset, m_array2DOffset;
    };

    class PixelSampler : public Sampler {
    public:
        // PixelSampler Public Methods
        PixelSampler(int64_t samplesPerPixel, int nSampledDimensions);
        PixelSampler(const PixelSampler& _rhs);
        bool StartNextSample() override;
        bool SetSampleNumber(int64_t) override;
        float Get1D() override;
        Vector2f Get2D() override;

    protected:
        // PixelSampler Protected Data
        std::vector<std::vector<float>>             m_samples1D;
        std::vector<std::vector<Vector2f>>    m_samples2D;
        int                                         m_current1DDimension = 0, 
                                                    m_current2DDimension = 0;
        RNG                                         m_rng;
    };

    class GlobalSampler : public Sampler {
    public:
        GlobalSampler(int64_t samplesPerPixel);
        
        // GlobalSampler Public Methods
        bool            StartNextSample() override;
        void            StartPixel(const Vector2i&)  override;
        bool            SetSampleNumber(int64_t sampleNum)  override;
        float           Get1D() override;
        Vector2f  Get2D() override;
       
        virtual int64_t GetIndexForSample(int64_t sampleNum) const = 0;
        virtual float   SampleDimension(int64_t index, int dimension) const = 0;

    private:
        // GlobalSampler Private Data
        int              m_dimension;
        int              m_arrayEndDim;
        int64_t          m_intervalSampleIndex;
        static const int s_arrayStartDim = 5;
        
    };
    
    
   



    

    // StratifiedSampler Declarations
    class StratifiedSampler : public PixelSampler {
    public:
        // StratifiedSampler Public Methods
        StratifiedSampler(int xPixelSamples, int yPixelSamples, bool jitterSamples, int nSampledDimensions);
        void StartPixel(const Vector2i&) override;
        std::unique_ptr<Sampler> Clone(int seed) const override;

    private:
        // StratifiedSampler Private Data
        const int m_xPixelSamples, m_yPixelSamples;
        const bool m_jitterSamples;
    };

    class HaltonSampler : public GlobalSampler {
    public:

        HaltonSampler(int nsamp, const BBox2i& sampleBounds, bool sampleAtCenter = false);
        int64_t GetIndexForSample(int64_t sampleNum) const override;
        float   SampleDimension(int64_t index, int dimension) const override;
        std::unique_ptr<Sampler> Clone(int seed)const  override;

    private:
        // HaltonSampler Private Data
        static std::vector<uint16_t> s_radicalInversePermutations;
        Vector2i  m_baseScales, 
                        m_baseExponents;
        int             m_sampleStride;
        int             m_multInverse[2];
        mutable Vector2i  m_pixelForOffset = Vector2i(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        mutable int64_t         m_offsetForCurrentPixel;
        // Added after book publication: force all image samples to be at the
        // center of the pixel area.
        bool                    m_sampleAtPixelCenter;

        // HaltonSampler Private Methods
        const uint16_t* PermutationForDimension(int dim) const {
            assert(dim < PrimeTableSize);
            return &s_radicalInversePermutations[PrimeSums[dim]];
        }
    };

    class SobolSampler : public GlobalSampler
    {
    public:
        // SobolSampler Public Methods      
        SobolSampler(int64_t samplesPerPixel, const BBox2i& sampleBounds);
        int64_t GetIndexForSample(int64_t sampleNum) const override;
        float   SampleDimension(int64_t index, int dimension) const override;

        std::unique_ptr<Sampler> Clone(int seed) const override;


        // SobolSampler Private Data
        const BBox2i m_sampleBounds;
        int                m_resolution, 
                           m_log2Resolution;
    };

    class RandomSampler : public Sampler 
    {
    public:
        RandomSampler(int ns, int seed = 0);
        RandomSampler(const RandomSampler& _rhs);
        void                         StartPixel(const Vector2i&);
        float                        Get1D() override;
        Vector2f                     Get2D() override;
        std::unique_ptr<Sampler>     Clone(int seed) const override;

    private:
        RNG m_rng;
    };

    // ZeroTwoSequenceSampler Declarations
    class ZeroTwoSequenceSampler : public PixelSampler {
    public:
        // ZeroTwoSequenceSampler Public Methods
        ZeroTwoSequenceSampler(int64_t samplesPerPixel, int nSampledDimensions = 4);
        void     StartPixel(const Vector2i&) override;
        std::unique_ptr<Sampler> Clone(int seed) const override;
        int     RoundCount(int count) const override;
    };


    // MaxMinDistSampler Declarations
class MaxMinDistSampler : public PixelSampler {
  public:

      MaxMinDistSampler(int64_t samplesPerPixel, int nSampledDimensions);

    // MaxMinDistSampler Public Methods
    void StartPixel(const Vector2i &) override;
    std::unique_ptr<Sampler> Clone(int seed) const override;
    int RoundCount(int count) const override;
   
    static int GetSPP(int64_t spp);

  private:
    // MaxMinDistSampler Private Data
    const uint32_t *m_CPixel = nullptr;
};

   


    Sampler* CreateHaltonSampler(const ParamSet& _params, const Camera& _camera);
    Sampler* CreateLowDiscrepancySampler(const ParamSet& _params, const Camera& _camera);
    Sampler* CreateRandomSampler(const ParamSet& _params, const Camera& _camera);
    Sampler* CreateStratifiedSampler(const ParamSet& _params, const Camera& _camera);
    Sampler* CreateSobolSampler(const ParamSet& _params, const Camera& _camera);
    Sampler* CreateMaxMinDistSampler(const ParamSet& _params, const Camera& _camera);
}