#pragma once
#include "Defines.h"
#include "Primitive.h"

namespace RayTrace
{
  // BVHAccel Declarations
    class BVHAccel : public Aggregate 
    {
    public:
        // BVHAccel Public Types
        enum class SplitMethod { SAH, HLBVH, Middle, EqualCounts };

        // BVHAccel Public Methods
        BVHAccel(PrimitiveVector v,
            int maxPrimsInNode = 1,
            SplitMethod splitMethod = SplitMethod::SAH);
        BBox3f worldBound() const;
        ~BVHAccel();
        bool intersect(const Ray& ray, SurfaceInteraction* isect) const;
        bool intersectP(const Ray& ray) const;

    private:
        // BVHAccel Private Methods
        BVHBuildNode* recursiveBuild(
            MemoryArena& arena, PrimInfoVector& primitiveInfo,
            int start, int end, int* totalNodes,
            PrimitiveVector& orderedPrims);

        BVHBuildNode* HLBVHBuild(
            MemoryArena& arena, const PrimInfoVector& primitiveInfo,
            int* totalNodes,
            PrimitiveVector& orderedPrims) const;

        BVHBuildNode* emitLBVH(
            BVHBuildNode*& buildNodes,
            const PrimInfoVector& primitiveInfo,
            MortonPrimitive* mortonPrims, 
            int nPrimitives, int* totalNodes,
            PrimitiveVector& orderedPrims,
            std::atomic<int>* orderedPrimsOffset, int bitIndex) const;

        BVHBuildNode* buildUpperSAH(MemoryArena& arena,
            std::vector<BVHBuildNode*>& treeletRoots,
            int start, int end, int* totalNodes) const;

        int flattenBVHTree(BVHBuildNode* node, int* offset);

        // BVHAccel Private Data
        const int         maxPrimsInNode;
        const SplitMethod splitMethod;
        PrimitiveVector   primitives;
        LinearBVHNode*    nodes = nullptr;
    };


    //// BVHAccel Declarations
    //class BVHAccel : public Aggregate {
    //    

    //public:
    //    enum eSplitMethod { SPLIT_MIDDLE, SPLIT_EQUAL_COUNTS, SPLIT_SAH };

    //    // BVHAccel Public Methods
    //    BVHAccel( PrimitiveVector _primitives, uint32_t _maxPrims = 4, eSplitMethod _method = SPLIT_SAH );
    //    ~BVHAccel();
    //    BBox3f worldBound() const override;  
    //    bool         intersect(const Ray& ray, SurfaceInteraction* isect) const override;
    //    bool         intersectP(const Ray& ray) const override;

    //private:
    //    // BVHAccel Private Methods
    //    BVHBuildNode* recursiveBuild(MemoryArena& buildArena,
    //        std::vector<BVHPrimitiveInfo>& buildData, uint32_t start, uint32_t end,
    //        uint32_t* totalNodes, PrimitiveVector& orderedPrims);

    //    uint32_t      flattenBVHTree(BVHBuildNode* node, uint32_t* offset);

    //    // BVHAccel Private Data
    //    uint32_t             m_maxPrimsInNode;        
    //    eSplitMethod         m_splitMethod;
    //    PrimitiveVector      m_primitives;
    //    std::vector<LinearBVHNode> m_nodes;      
    //};

    std::shared_ptr<BVHAccel> CreateBVHAccelerator(PrimitiveVector _prims, const ParamSet& _param);
}