#pragma once
#include <string>
#include "Defines.h"
#include "ParameterSet.h"
#include "Memory.h"
#include "Transform.h"

namespace RayTrace
{
     
    struct Options {
        Options() {
            cropWindow[0][0] = 0;
            cropWindow[0][1] = 1;
            cropWindow[1][0] = 0;
            cropWindow[1][1] = 1;
        }
        int nThreads = 0;
        bool quickRender = false;
        bool quiet = false;
        bool cat = false, toPly = false;
        std::string imageFile;
        void* film = nullptr;
        // x0, x1, y0, y1
        float cropWindow[2][2];
    };

    // API Local Classes
    constexpr int MaxTransforms = 2;
    constexpr int StartTransformBits = 1 << 0;
    constexpr int EndTransformBits = 1 << 1;
    constexpr int AllTransformsBits = (1 << MaxTransforms) - 1;

    struct TransformSet {
        // TransformSet Public Methods
        Transform& operator[](int i) {
            //CHECK_GE(i, 0);
           // CHECK_LT(i, MaxTransforms);
            assert(i >= 0 && i < MaxTransforms);
            return t[i];
        }
        const Transform& operator[](int i) const {
            // CHECK_GE(i, 0);
            // CHECK_LT(i, MaxTransforms);
            assert(i >= 0 && i < MaxTransforms);
            return t[i];
        }
        friend TransformSet Inverse(const TransformSet& ts) {
            TransformSet tInv;
            for (int i = 0; i < MaxTransforms; ++i)
                tInv.t[i] = ts.t[i].inverted();//Inverse(ts.t[i]);
            return tInv;
        }
        bool IsAnimated() const {
            for (int i = 0; i < MaxTransforms - 1; ++i)
                if (t[i] != t[i + 1]) return true;
            return false;
        }

    private:
        Transform t[MaxTransforms];
    };

    struct RenderOptions {
        // RenderOptions Public Methods
        Integrator* MakeIntegrator() const;
        Scene* MakeScene();
        Camera* MakeCamera() const;


        std::unique_ptr<Scene>      m_scene;
        std::unique_ptr<Integrator> m_integrator;

       // void Render();

        // RenderOptions Public Data
        float transformStartTime = 0, transformEndTime = 1;
        std::string FilterName = "box";
        ParamSet FilterParams;
        std::string FilmName = "image";
        ParamSet FilmParams;
        std::string SamplerName = "halton";
        ParamSet SamplerParams;
        std::string AcceleratorName = "bvh";
        ParamSet AcceleratorParams;
        std::string IntegratorName = "path";
        ParamSet IntegratorParams;
        std::string CameraName = "perspective";
        ParamSet CameraParams;
        TransformSet CameraToWorld;
        std::map<std::string, std::shared_ptr<Medium>> namedMedia;
        std::vector<std::shared_ptr<Light>> lights;
        std::vector<std::shared_ptr<Primitive>> primitives;
        std::map<std::string, std::vector<std::shared_ptr<Primitive>>> instances;
        
        std::vector<TriangleMeshPtr> m_meshes;

        std::vector<std::shared_ptr<Primitive>>* currentInstance = nullptr;
        bool haveScatteringMedia = false;
    };

    // MaterialInstance represents both an instance of a material as well as
    // the information required to create another instance of it (possibly with
    // different parameters from the shape).
    struct MaterialInstance {
        MaterialInstance() = default;
        MaterialInstance(const std::string& name, const std::shared_ptr<Material>& mtl,
            ParamSet params)
            : name(name), material(mtl), params(std::move(params)) {}

        std::string name;
        std::shared_ptr<Material> material;
        ParamSet params;
    };

    struct GraphicsState
    {
        // Graphics State Methods
        GraphicsState();
        std::shared_ptr<Material> GetMaterialForShape(const ParamSet& geomParams);
        MediumInterface CreateMediumInterface();

        
        // Graphics State
        std::string currentInsideMedium, currentOutsideMedium;

        // Updated after book publication: floatTextures, spectrumTextures, and
        // namedMaterials are all implemented using a "copy on write" approach
        // for more efficient GraphicsState management.  When state is pushed
        // in pbrtAttributeBegin(), we don't immediately make a copy of these
        // maps, but instead record that each one is shared.  Only if an item
        // is added to one is a unique copy actually made.
        std::shared_ptr<FloatTextureMap> floatTextures;
        bool floatTexturesShared = false;


        std::shared_ptr<SpectrumTextureMap> spectrumTextures;
        bool spectrumTexturesShared = false;

        using NamedMaterialMap = std::map<std::string, std::shared_ptr<MaterialInstance>>;
        std::shared_ptr<NamedMaterialMap> namedMaterials;
        bool namedMaterialsShared = false;

        std::shared_ptr<MaterialInstance> currentMaterial;
        ParamSet areaLightParams;
        std::string areaLight;
        bool reverseOrientation = false;
    };

    /*STAT_MEMORY_COUNTER("Memory/TransformCache", transformCacheBytes);
    STAT_PERCENT("Scene/TransformCache hits", nTransformCacheHits, nTransformCacheLookups);
    STAT_INT_DISTRIBUTION("Scene/Probes per TransformCache lookup", transformCacheProbes);*/

    // Note: TransformCache has been reimplemented and has a slightly different
    // interface compared to the version described in the third edition of
    // Physically Based Rendering.  The new version is more efficient in both
    // space and memory, which is helpful for highly complex scenes.
    //
    // The new implementation uses a hash table to store Transforms (rather
    // than a std::map, which generally uses a red-black tree).  Further,
    // it doesn't always store the inverse of the transform; if a caller
    // wants the inverse as well, they are responsible for storing it.
    //
    // The hash table size is always a power of two, allowing for the use of a
    // bitwise AND to turn hash values into table offsets.  Quadratic probing
    // is used when there is a hash collision.
    class TransformCache {
    public:
        TransformCache();

        // TransformCache Public Methods
        Transform* Lookup(const Transform& t);

        void Clear();

    private:
        void Insert(Transform* tNew);
        void Grow();

        static uint64_t Hash(const Transform& t);

        // TransformCache Private Data
        std::vector<Transform*> hashTable;
        int hashTableOccupancy;
        MemoryArena arena;
    };


    // API Static Data
    enum class APIState { Uninitialized, OptionsBlock, WorldBlock };
    // API Global Variables
    extern Options PbrtOptions;
    extern APIState currentApiState;
    extern TransformSet curTransform;
    extern uint32_t activeTransformBits;
    extern std::map<std::string, TransformSet> namedCoordinateSystems;
    extern std::unique_ptr<RenderOptions> renderOptions;
    extern GraphicsState graphicsState;
    extern std::vector<GraphicsState> pushedGraphicsStates;
    extern std::vector<TransformSet> pushedTransforms;
    extern std::vector<uint32_t> pushedActiveTransformBits;
    extern TransformCache transformCache;

    // API Function Declarations
    void pbrtInit(const Options& opt);
    void pbrtCleanup();
    void pbrtIdentity();
    void pbrtTranslate(float dx, float dy, float dz);
    void pbrtRotate(float angle, float ax, float ay, float az);
    void pbrtScale(float sx, float sy, float sz);
    void pbrtLookAt(float ex, float ey, float ez, float lx, float ly, float lz,
        float ux, float uy, float uz);
    void pbrtConcatTransform(float transform[16]);
    void pbrtTransform(float transform[16]);
    void pbrtCoordinateSystem(const std::string&);
    void pbrtCoordSysTransform(const std::string&);
    void pbrtActiveTransformAll();
    void pbrtActiveTransformEndTime();
    void pbrtActiveTransformStartTime();
    void pbrtTransformTimes(float start, float end);
    void pbrtPixelFilter(const std::string& name, const ParamSet& params);
    void pbrtFilm(const std::string& type, const ParamSet& params);
    void pbrtSampler(const std::string& name, const ParamSet& params);
    void pbrtAccelerator(const std::string& name, const ParamSet& params);
    void pbrtIntegrator(const std::string& name, const ParamSet& params);
    void pbrtCamera(const std::string&, const ParamSet& cameraParams);
    void pbrtMakeNamedMedium(const std::string& name, const ParamSet& params);
    void pbrtMediumInterface(const std::string& insideName,
        const std::string& outsideName);
    void pbrtWorldBegin();
    void pbrtAttributeBegin();
    void pbrtAttributeEnd();
    void pbrtTransformBegin();
    void pbrtTransformEnd();
    void pbrtTexture(const std::string& name, const std::string& type,
        const std::string& texname, const ParamSet& params);
    void pbrtMaterial(const std::string& name, const ParamSet& params);
    void pbrtMakeNamedMaterial(const std::string& name, const ParamSet& params);
    void pbrtNamedMaterial(const std::string& name);
    void pbrtLightSource(const std::string& name, const ParamSet& params);
    void pbrtAreaLightSource(const std::string& name, const ParamSet& params);
    void pbrtShape(const std::string& name, const ParamSet& params);
    void pbrtReverseOrientation();
    void pbrtObjectBegin(const std::string& name);
    void pbrtObjectEnd();
    void pbrtObjectInstance(const std::string& name);
    void pbrtWorldEnd();

    void pbrtParseFile(std::string filename);
    void pbrtParseString(std::string str);
}