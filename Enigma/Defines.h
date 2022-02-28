#pragma once
#include <typeinfo>
#include <variant>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>
#include <any>
#include <fmt/core.h>
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_XYZW_ONLY
#include <glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_interpolation.hpp> 
#include <glm/gtx/transform.hpp>
#include <glm/gtc/random.hpp>
#include <matrix.hpp>
#include <gtc/type_ptr.hpp>
#include <glm/detail/type_half.hpp>
#include <ext/matrix_clip_space.hpp>




// Platform-specific definitions
#if defined(_WIN32) || defined(_WIN64)
#define IS_WINDOWS

#endif

#if defined(_MSC_VER)
#define IS_MSVC
#if _MSC_VER == 1800
#define snprintf _snprintf
#endif
#endif

#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif

#include <stdint.h>
#include <cstdint>
#if defined(IS_MSVC)
#include <assert.h>
#include <float.h>
#include <intrin.h>
#pragma warning(disable : 4305)  // double constant assigned to float
#pragma warning(disable : 4244)  // int -> float conversion
#pragma warning(disable : 4843)  // double -> float conversion
#pragma warning(disable : 4267)  // size_t -> int
#pragma warning(disable : 4838)  // another double -> int
#endif

#define UNUSED(x) (void)x;

namespace RayTrace
{
    class Ray;
    class RayDifferential;
   

    template<typename T>
    using Vector2 = glm::vec<2, T, glm::defaultp>;
   

    template<typename T>
    using Vector3 = glm::vec<3, T, glm::defaultp>;
  

    template<typename T>
    using Vector4 = glm::vec<4, T, glm::defaultp>;
    



    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int>;
    using Point2f  = Vector2f;
    using Point2i  = Vector2i;

    using Vector3f = Vector3<float>;
    using Vector3i = Vector3<int>;

    using Point3f  = Vector3f;
    using Normal3f = Vector3f;

    using Vector4f = Vector4<float>;
    using Vector4i = Vector4<int>;

    using Quatf    = glm::qua<float, glm::defaultp>;

    using Matrix4x4 = glm::mat4;

    class Transform;
   // class Matrix4x4;
 
    template <typename T> class BBox2;
    using BBox2f = BBox2<float>;
    using BBox2i = BBox2<int>;

    template <typename T> class BBox3;
    using BBox3f = BBox3<float>;
    using BBox3i = BBox3<int>;

  
    struct Distribution1D;
    struct Distribution2D; 
  
    using Distribution1DUPtr = std::unique_ptr<Distribution1D>;

    enum eAxis
    {
        AXIS_X = 0,
        AXIS_Y,
        AXIS_Z
    };
}


namespace RayTrace
{
    class ParamSet;
    class TextureParams;
    class MemoryArena;
    class Film;
    class CameraSample;
    class Filter;
    class Camera;
    class Sampler;    
    class Scene; 
    class RNG;
    class Filter;   
    class FilmTile;

    class Medium;
    struct MediumInterface;  

    class Integrator;
    class Interaction;
    class MediumInteraction;
    class SurfaceInteraction;
    class LightDistribution;

    template <typename T> struct ParamSetItem;
    template <int nSamples> class CoefficientSpectrum;
    class RGBSpectrum;
    class SampledSpectrum;
    typedef RGBSpectrum Spectrum;

    using FilterUPtr = std::unique_ptr<Filter>;
    using SamplerPtr = std::shared_ptr<Sampler>;

   using PixelVector = std::vector<RGBSpectrum>;
   using CameraPtr   = std::shared_ptr<Camera>;


    enum eSpectrumType {
        SPECTRUM_REFLECTANCE,
        SPECTRUM_ILLUMINANT
    };
}

namespace RayTrace
{
    template <typename T> class Texture;
    template <typename T> class ConstantTexture;

    class BSDF;
    class BSSRDF;
    class AreaLight;
    class Material;
    class Light;
    class TextureMapping2D;
    class TextureMapping3D;
    class VolumeRegion;      
    
    class MicrofacetDistribution;
    class  VisibilityTester;

    struct  BSSRDFTable;
    struct  FourierBSDFTable;
    

    using FloatTexture       = Texture<float>;
    using SpectrumTexture    = Texture<Spectrum>;
    using FloatTexturePtr    = std::shared_ptr<FloatTexture>;
    using SpectrumTexturePtr = std::shared_ptr<SpectrumTexture>;

    using FloatTextureMap    = std::map<std::string, FloatTexturePtr>;
    using SpectrumTextureMap = std::map<std::string, SpectrumTexturePtr>;
   
    using TextureMapping2DPtr = std::shared_ptr<TextureMapping2D>;
    using TextureMapping3DPtr = std::shared_ptr<TextureMapping3D>;

    using MaterialPtr = std::shared_ptr<Material>;
    using MaterialVector = std::vector<MaterialPtr>;

    using LightPtr = std::shared_ptr<Light>;
  
    // TransportMode Declarations
    enum  eTransportMode { TRANSPORTMODE_RADIANCE, 
                           TRANSPORTMODE_IMPORTANCE };

    enum  eImageWrapMode
    {
        TEXTURE_WRAP_REPEAT = 0,
        TEXTURE_WRAP_BLACK,
        TEXTURE_WRAP_CLAMP
    };

    // BSDF Declarations
    enum eBxDFType {
        BSDF_UNDEFINED = 0,
        BSDF_REFLECTION = 1 << 0,
        BSDF_TRANSMISSION = 1 << 1,
        BSDF_DIFFUSE = 1 << 2,
        BSDF_GLOSSY = 1 << 3,
        BSDF_SPECULAR = 1 << 4,
        BSDF_ALL_TYPES = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR,
        BSDF_ALL_REFLECTION = BSDF_REFLECTION | BSDF_ALL_TYPES,
        BSDF_ALL_TRANSMISSION = BSDF_TRANSMISSION | BSDF_ALL_TYPES,
        BSDF_ALL = BSDF_ALL_REFLECTION | BSDF_ALL_TRANSMISSION
    };

    // LightFlags Declarations
    enum eLightFlags : int {
        LIGHTFLAG_DELTAPOSITION   = 1,
        LIGHTFLAG_DELTADIRECTION  = 2,
        LIGHTFLAG_AREA            = 4,
        LIGHTFLAG_INFINITE        = 8
    };

    // AAMethod Declaration
    enum class eAAMethod { None, ClosedForm };
}

namespace RayTrace
{
    class Primitive;
    class Shape;
    class ShapeSet;
    class GeometricPrimitive;
    class BVHAccel;  

    struct TriangleMesh;

    struct BVHBuildNode;
    struct BVHPrimitiveInfo;
    struct LinearBVHNode;
    struct MortonPrimitive; 
    using TriangleMeshPtr = std::shared_ptr<TriangleMesh>;


    using PrimInfoVector = std::vector<BVHPrimitiveInfo>;

    using ShapePtr     = std::shared_ptr<Shape>;
    using ShapesVector = std::vector<ShapePtr>;

    using PrimitivePtr          = std::shared_ptr<Primitive>;
    using PrimitiveVector       = std::vector<std::shared_ptr<Primitive>>;
    using ConstPrimitiveVector = std::vector<const std::shared_ptr<Primitive>>;
}

using FloatVector = std::vector<float>;
using U8Vector    = std::vector<uint8_t>;
using ShortVector = std::vector<short>;
using IntVector   = std::vector<int>;
using Vec2fVector = std::vector<RayTrace::Vector2f>;
using Vec3fVector = std::vector<RayTrace::Vector3f>;



static bool doBreak = false;
