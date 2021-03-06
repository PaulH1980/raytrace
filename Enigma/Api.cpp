#include "Includes.h"
#include "Api.h"
#define STILL_TODO 0




namespace RayTrace
{
     Options PbrtOptions;
     APIState currentApiState = APIState::Uninitialized;
     TransformSet curTransform;
     uint32_t activeTransformBits = AllTransformsBits;
     std::map<std::string, TransformSet> namedCoordinateSystems;
     std::unique_ptr<RenderOptions> renderOptions;
     GraphicsState graphicsState;
     std::vector<GraphicsState> pushedGraphicsStates;
     std::vector<TransformSet> pushedTransforms;
     std::vector<uint32_t> pushedActiveTransformBits;
     TransformCache transformCache;





    int catIndentCount = 0;

    // API Forward Declarations
    std::vector<std::shared_ptr<Shape>> MakeShapes(const std::string& name,
        const Transform* ObjectToWorld,
        const Transform* WorldToObject,
        bool reverseOrientation,
        const ParamSet& paramSet);

    // API Macros
#define VERIFY_INITIALIZED(func)                           \
    if (!(PbrtOptions.cat || PbrtOptions.toPly) &&           \
        currentApiState == APIState::Uninitialized) {        \
        Error(                                             \
            "pbrtInit() must be before calling \"%s()\". " \
            "Ignoring.",                                   \
            func);                                         \
        return;                                            \
    } else /* swallow trailing semicolon */
#define VERIFY_OPTIONS(func)                             \
    VERIFY_INITIALIZED(func);                            \
    if (!(PbrtOptions.cat || PbrtOptions.toPly) &&       \
        currentApiState == APIState::WorldBlock) {       \
        Error(                                           \
            "Options cannot be set inside world block; " \
            "\"%s\" not allowed.  Ignoring.",            \
            func);                                       \
        return;                                          \
    } else /* swallow trailing semicolon */
#define VERIFY_WORLD(func)                                   \
    VERIFY_INITIALIZED(func);                                \
    if (!(PbrtOptions.cat || PbrtOptions.toPly) &&           \
        currentApiState == APIState::OptionsBlock) {         \
        Error(                                               \
            "Scene description must be inside world block; " \
            "\"%s\" not allowed. Ignoring.",                 \
            func);                                           \
        return;                                              \
    } else /* swallow trailing semicolon */
#define FOR_ACTIVE_TRANSFORMS(expr)           \
    for (int i = 0; i < MaxTransforms; ++i)   \
        if (activeTransformBits & (1 << i)) { \
            expr                              \
        }
#define WARN_IF_ANIMATED_TRANSFORM(func)                             \
    do {                                                             \
        if (curTransform.IsAnimated())                               \
            Warning(                                                 \
                "Animated transformations set; ignoring for \"%s\" " \
                "and using the start transform only",                \
                func);                                               \
    } while (false) /* swallow trailing semicolon */

// Object Creation Function Definitions
    std::vector<std::shared_ptr<Shape>> MakeShapes(const std::string& name,
        const Transform* object2world,
        const Transform* world2object,
        bool reverseOrientation,
        const ParamSet& paramSet) {
        std::vector<std::shared_ptr<Shape>> shapes;
        TriangleMeshPtr meshPtr;
        std::shared_ptr<Shape> s;
        if (name == "sphere")
            s = CreateSphereShape(object2world, world2object, reverseOrientation,
                paramSet);
        // Create remaining single _Shape_ types
        else if (name == "cylinder")
            s = CreateCylinderShape(object2world, world2object, reverseOrientation,
                paramSet);
        else if (name == "disk")
            s = CreateDiskShape(object2world, world2object, reverseOrientation,
                paramSet);
        /*  else if (name == "cone")
              s = CreateConeShape(object2world, world2object, reverseOrientation,
                  paramSet);
          else if (name == "paraboloid")
              s = CreateParaboloidShape(object2world, world2object,
                  reverseOrientation, paramSet);
          else if (name == "hyperboloid")
              s = CreateHyperboloidShape(object2world, world2object,
                  reverseOrientation, paramSet);*/
        if (s != nullptr)
            shapes.push_back(s);
        else if (name == "plymesh")
            shapes = CreatePLYMesh(object2world, world2object, 
                reverseOrientation, paramSet, &*graphicsState.floatTextures, &meshPtr );
        // Create multiple-_Shape_ types
        else if (name == "curve")
            shapes = CreateCurveShape(object2world, world2object, reverseOrientation, paramSet);
        else if (name == "trianglemesh") {            
                shapes = CreateTriangleMeshShape(object2world, world2object,
                    reverseOrientation, paramSet, &*graphicsState.floatTextures, &meshPtr);
        }       
        else if (name == "loopsubdiv")
            shapes = CreateLoopSubdiv(object2world, world2object, reverseOrientation, paramSet);
        else if (name == "nurbs")
            shapes = CreateNURBSShape(object2world, world2object, reverseOrientation, paramSet);
        else
            Warning("Shape \"%s\" unknown.", name.c_str());
        if (meshPtr)
            renderOptions->m_meshes.push_back(meshPtr);
        return shapes;
    }

  //  STAT_COUNTER("Scene/Materials created", nMaterialsCreated);

    std::shared_ptr<Material> MakeMaterial(const std::string& name,
        const TextureParams& mp) {
        Material* material = nullptr;
        if (name == "" || name == "none")
            return nullptr;
        else if (name == "matte")
            material = CreateMatteMaterial(mp);
        else if (name == "plastic")
            material = CreatePlasticMaterial(mp);
        else if (name == "translucent")
            material = CreateTranslucentMaterial(mp);
        else if (name == "glass")
            material = CreateGlassMaterial(mp);
        else if (name == "mirror")
            material = CreateMirrorMaterial(mp);
        else if (name == "fourier")
            material = CreateFourierMaterial(mp);
      /*  else if (name == "hair")
            material = CreateHairMaterial(mp);
        else if (name == "disney")
            material = CreateDisneyMaterial(mp);*/
        else if (name == "mix") {
            std::string m1 = mp.FindString("namedmaterial1", "");
            std::string m2 = mp.FindString("namedmaterial2", "");
            std::shared_ptr<Material> mat1, mat2;
            if (graphicsState.namedMaterials->find(m1) ==
                graphicsState.namedMaterials->end()) {
                Error("Named material \"%s\" undefined.  Using \"matte\"",
                    m1.c_str());
                mat1 = MakeMaterial("matte", mp);
            }
            else
                mat1 = (*graphicsState.namedMaterials)[m1]->material;

            if (graphicsState.namedMaterials->find(m2) ==
                graphicsState.namedMaterials->end()) {
                Error("Named material \"%s\" undefined.  Using \"matte\"",
                    m2.c_str());
                mat2 = MakeMaterial("matte", mp);
            }
            else
                mat2 = (*graphicsState.namedMaterials)[m2]->material;

            material = CreateMixMaterial(mp, mat1, mat2);
        }
        else if (name == "metal")
            material = CreateMetalMaterial(mp);
        else if (name == "substrate")
            material = CreateSubstrateMaterial(mp);
        else if (name == "uber")
            material = CreateUberMaterial(mp);
        else if (name == "subsurface")
            material = CreateSubsurfaceMaterial(mp);
        else if (name == "kdsubsurface")
            material = CreateKdSubsurfaceMaterial(mp);
        else if (name == "fourier")
            material = CreateFourierMaterial(mp);
        else {
            Warning("Material \"%s\" unknown. Using \"matte\".", name.c_str());
            material = CreateMatteMaterial(mp);
        }

        if ((name == "subsurface" || name == "kdsubsurface") &&
            (renderOptions->IntegratorName != "path" &&
                (renderOptions->IntegratorName != "volpath")))
            Warning(
                "Subsurface scattering material \"%s\" used, but \"%s\" "
                "integrator doesn't support subsurface scattering. "
                "Use \"path\" or \"volpath\".",
                name.c_str(), renderOptions->IntegratorName.c_str());

        mp.ReportUnused();
        if (!material) 
            Error("Unable to create material \"%s\"", name.c_str());
        
        return std::shared_ptr<Material>(material);
    }

    std::shared_ptr<Texture<float>> MakeFloatTexture(const std::string& name,
        const Transform& tex2world,
        const TextureParams& tp) {
        Texture<float>* tex = nullptr;
        if (name == "constant")
            tex = CreateConstantFloatTexture(tex2world, tp);
        else if (name == "scale")
            tex = CreateScaleFloatTexture(tex2world, tp);
        else if (name == "mix")
            tex = CreateMixFloatTexture(tex2world, tp);
        else if (name == "bilerp")
            tex = CreateBilerpFloatTexture(tex2world, tp);
        else if (name == "imagemap")
            tex = CreateImageFloatTexture(tex2world, tp);
        else if (name == "uv")
            tex = CreateUVFloatTexture(tex2world, tp);
        else if (name == "checkerboard")
            tex = CreateCheckerboardFloatTexture(tex2world, tp);
        else if (name == "dots")
            tex = CreateDotsFloatTexture(tex2world, tp);
        else if (name == "fbm")
            tex = CreateFBmFloatTexture(tex2world, tp);
        else if (name == "wrinkled")
            tex = CreateWrinkledFloatTexture(tex2world, tp);
        else if (name == "marble")
            tex = CreateMarbleFloatTexture(tex2world, tp);
        else if (name == "windy")
            tex = CreateWindyFloatTexture(tex2world, tp);
        else if (name == "ptex")
            tex = CreatePtexFloatTexture(tex2world, tp);
        else
            Warning("float texture \"%s\" unknown.", name.c_str());
        tp.ReportUnused();
        return std::shared_ptr<Texture<float>>(tex);
    }

    std::shared_ptr<Texture<Spectrum>> MakeSpectrumTexture(
        const std::string& name, const Transform& tex2world,
        const TextureParams& tp) {
        Texture<Spectrum>* tex = nullptr;
        if (name == "constant")
            tex = CreateConstantSpectrumTexture(tex2world, tp);
        else if (name == "scale")
            tex = CreateScaleSpectrumTexture(tex2world, tp);
        else if (name == "mix")
            tex = CreateMixSpectrumTexture(tex2world, tp);
        else if (name == "bilerp")
            tex = CreateBilerpSpectrumTexture(tex2world, tp);
        else if (name == "imagemap")
            tex = CreateImageSpectrumTexture(tex2world, tp);
        else if (name == "uv")
            tex = CreateUVSpectrumTexture(tex2world, tp);
        else if (name == "checkerboard")
            tex = CreateCheckerboardSpectrumTexture(tex2world, tp);
        else if (name == "dots")
            tex = CreateDotsSpectrumTexture(tex2world, tp);
        else if (name == "fbm")
            tex = CreateFBmSpectrumTexture(tex2world, tp);
        else if (name == "wrinkled")
            tex = CreateWrinkledSpectrumTexture(tex2world, tp);
        else if (name == "marble")
            tex = CreateMarbleSpectrumTexture(tex2world, tp);
        else if (name == "windy")
            tex = CreateWindySpectrumTexture(tex2world, tp);
        else if (name == "ptex")
            tex = CreatePtexSpectrumTexture(tex2world, tp);
        else
            Warning("Spectrum texture \"%s\" unknown.", name.c_str());
        tp.ReportUnused();
        return std::shared_ptr<Texture<Spectrum>>(tex);
    }

    std::shared_ptr<Medium> MakeMedium(const std::string& name,
        const ParamSet& paramSet,
        const Transform& medium2world) {
        float sig_a_rgb[3] = { .0011f, .0024f, .014f },
              sig_s_rgb[3] = { 2.55f, 3.21f, 3.77f };
        Spectrum sig_a = Spectrum::FromRGB(sig_a_rgb),
                 sig_s = Spectrum::FromRGB(sig_s_rgb);

        std::string preset = paramSet.FindOneString("preset", "");
        bool found = GetMediumScatteringProperties(preset, &sig_a, &sig_s);
        if (preset != "" && !found)
            Warning("Material preset \"%s\" not found.  Using defaults.",
                preset.c_str());
        float scale = paramSet.FindOneFloat("scale", 1.f);
        float g = paramSet.FindOneFloat("g", 0.0f); 
        sig_a = paramSet.FindOneSpectrum("sigma_a", sig_a) * scale;
        sig_s = paramSet.FindOneSpectrum("sigma_s", sig_s) * scale;
        Medium* m = NULL;
        if (name == "homogeneous") {
            m = new HomogeneousMedium(sig_a, sig_s, g);
        }
        else if (name == "heterogeneous") {
            int nitems;
            const float* data = paramSet.FindFloat("density", &nitems);
            if (!data) {
                Error("No \"density\" values provided for heterogeneous medium?");
                return NULL;
            }
            int nx = paramSet.FindOneInt("nx", 1);
            int ny = paramSet.FindOneInt("ny", 1);
            int nz = paramSet.FindOneInt("nz", 1);
            Vector3f p0 = paramSet.FindOnePoint3f("p0", Vector3f(0.f, 0.f, 0.f));
            Vector3f p1 = paramSet.FindOnePoint3f("p1", Vector3f(1.f, 1.f, 1.f));
            if (nitems != nx * ny * nz) {
                Error(
                    "GridDensityMedium has %d density values; expected nx*ny*nz = "
                    "%d",
                    nitems, nx * ny * nz);
                return NULL;
            }
            Transform data2Medium = Translate(Vector3f(p0)) *
                Scale(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
            m = new GridDensityMedium(sig_a, sig_s, g, nx, ny, nz,
                medium2world * data2Medium, data);
        }
        else
            Warning("Medium \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        return std::shared_ptr<Medium>(m);
    }

    std::shared_ptr<Light> MakeLight(const std::string& name,
        const ParamSet& paramSet,
        const Transform& light2world,
        const MediumInterface& mediumInterface) {


        UNUSED(mediumInterface)

        std::shared_ptr<Light> light;
        if (name == "point")
            light =
            CreatePointLight(light2world,  paramSet);
        else if (name == "spot")
            light = CreateSpotLight(light2world, paramSet);
        else if (name == "goniometric")
            light = CreateGoniometricLight(light2world,paramSet);
        else if (name == "projection")
            light = CreateProjectionLight(light2world, paramSet);
        else if (name == "distant")
            light = CreateDistantLight(light2world, paramSet);
        else if (name == "infinite" || name == "exinfinite")
            light = CreateInfiniteLight(light2world, paramSet);
        else
            Warning("Light \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        return light;
    }

    std::shared_ptr<AreaLight> MakeAreaLight(const std::string& name,
        const Transform& light2world,
        const MediumInterface& mediumInterface,
        const ParamSet& paramSet,
        const std::shared_ptr<Shape>& shape) {

        UNUSED(mediumInterface)

        std::shared_ptr<AreaLight> area;
        if (name == "area" || name == "diffuse")
            area = std::dynamic_pointer_cast<AreaLight>( CreateDiffuseAreaLight(light2world, paramSet, shape) );
        else
            Warning("Area light \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        assert(area != nullptr);
        return area;
    }

    std::shared_ptr<Primitive> MakeAccelerator(
        const std::string& name,
        PrimitiveVector prims,
        const ParamSet& paramSet) {
        std::shared_ptr<Primitive> accel;
        if (name == "bvh")
            accel = CreateBVHAccelerator(std::move(prims), paramSet);
        /* else if (name == "kdtree")
             accel = CreateKdTreeAccelerator(std::move(prims), paramSet);*/
        else
            Warning("Accelerator \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        return accel;
    }

    Camera* MakeCamera(const std::string& name, const ParamSet& paramSet,
        const TransformSet& cam2worldSet, float transformStart,
        float transformEnd, Film* film) {
        Camera* camera = nullptr;
        MediumInterface mediumInterface = graphicsState.CreateMediumInterface();
        static_assert(MaxTransforms == 2,
            "TransformCache assumes only two transforms");
        Transform* cam2world[2] = {
            transformCache.Lookup(cam2worldSet[0]),
            transformCache.Lookup(cam2worldSet[1])
        };
        AnimatedTransform animatedCam2World(cam2world[0], transformStart,
            cam2world[1], transformEnd);
        if (name == "perspective")
            camera = CreatePerspectiveCamera(paramSet, animatedCam2World, film, mediumInterface.outside);
        else if (name == "orthographic")
            camera = CreateOrthographicCamera(paramSet, animatedCam2World, film, mediumInterface.outside);
        else if (name == "realistic")
            camera = CreateRealisticCamera(paramSet, animatedCam2World, film, mediumInterface.outside);
        else if (name == "environment")
            camera = CreateEnvironmentCamera(paramSet, animatedCam2World, film, mediumInterface.outside);
        else
            Warning("Camera \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        return camera;
    }

    std::shared_ptr<Sampler> MakeSampler(const std::string& name,
        const ParamSet& paramSet,
        const Camera* _camera) {
        Sampler* sampler = nullptr;
        if (name == "lowdiscrepancy" || name == "02sequence")
            sampler = CreateLowDiscrepancySampler(paramSet, *_camera);
        else if(name == "halton")
            sampler = CreateHaltonSampler(paramSet, *_camera);        
        else if (name == "random")
            sampler = CreateRandomSampler(paramSet, * _camera);       
        else if (name == "stratified")
            sampler = CreateStratifiedSampler(paramSet, *_camera);
        else if (name == "sobol")
            sampler = CreateSobolSampler(paramSet, *_camera);   
        else
            Warning("Sampler \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        if (!sampler)
            Error("Invalid Sampler");
        return std::shared_ptr<Sampler>(sampler);
    }

    std::unique_ptr<Filter> MakeFilter(const std::string& name,
        const ParamSet& paramSet) {
        Filter* filter = nullptr;
        if (name == "box")
            filter = CreateBoxFilter(paramSet);
        else if (name == "gaussian")
            filter = CreateGaussianFilter(paramSet);
        else if (name == "mitchell")
            filter = CreateMitchellFilter(paramSet);
        else if (name == "sinc")
            filter = CreateSincFilter(paramSet);
        else if (name == "triangle")
            filter = CreateTriangleFilter(paramSet);
        else {
            Error("Filter \"%s\" unknown.", name.c_str());
            exit(1);
        }
        paramSet.ReportUnused();
        return std::unique_ptr<Filter>(filter);
    }

    Film* MakeFilm(const std::string& name, const ParamSet& paramSet,
        std::unique_ptr<Filter> filter) 
    {
        if (PbrtOptions.film)
            return (Film*)PbrtOptions.film;
        
        
        Film* film = nullptr;
        if (name == "image")
            film = CreateFilm(paramSet, std::move(filter));
        else if(name == "sdl")
            film = CreateFilmSDL(paramSet, std::move(filter));
        else
            Warning("Film \"%s\" unknown.", name.c_str());
        paramSet.ReportUnused();
        return film;
    }

    // API Function Definitions
    void pbrtInit(const Options& opt) {
        PbrtOptions = opt;
        // API Initialization
        if (currentApiState != APIState::Uninitialized)
            Error("pbrtInit() has already been called.");
        currentApiState = APIState::OptionsBlock;
        renderOptions.reset(new RenderOptions);
        graphicsState = GraphicsState();
        catIndentCount = 0;

        // General \pbrt Initialization
        SampledSpectrum::Init();
    }

    void pbrtCleanup() {
        // API Cleanup
        if (currentApiState == APIState::Uninitialized)
            Error("pbrtCleanup() called without pbrtInit().");
        else if (currentApiState == APIState::WorldBlock)
            Error("pbrtCleanup() called while inside world block.");
        currentApiState = APIState::Uninitialized;      
    }

    void pbrtIdentity() {
        VERIFY_INITIALIZED("Identity");
        FOR_ACTIVE_TRANSFORMS(curTransform[i] = Transform();)
           
    }

    void pbrtTranslate(float dx, float dy, float dz) {
        VERIFY_INITIALIZED("Translate");
        FOR_ACTIVE_TRANSFORMS(curTransform[i] = curTransform[i] *
            Translate(Vector3f(dx, dy, dz));)
         
    }

    void pbrtTransform(float tr[16]) {
        VERIFY_INITIALIZED("Transform");
        FOR_ACTIVE_TRANSFORMS(
            curTransform[i] = Transform(Matrix4x4(
                tr[0], tr[4], tr[8], tr[12], tr[1], tr[5], tr[9], tr[13], tr[2],
                tr[6], tr[10], tr[14], tr[3], tr[7], tr[11], tr[15]));)
            
    }

    void pbrtConcatTransform(float tr[16]) {
        VERIFY_INITIALIZED("ConcatTransform");
        FOR_ACTIVE_TRANSFORMS(
            curTransform[i] =
            curTransform[i] *
            Transform(Matrix4x4(tr[0], tr[4], tr[8], tr[12], tr[1], tr[5],
                tr[9], tr[13], tr[2], tr[6], tr[10], tr[14],
                tr[3], tr[7], tr[11], tr[15]));)
           
    }

    void pbrtRotate(float angle, float dx, float dy, float dz) {
        VERIFY_INITIALIZED("Rotate");
        FOR_ACTIVE_TRANSFORMS(curTransform[i] =
            curTransform[i] *
            Rotate(angle, Vector3f(dx, dy, dz));)
            if (PbrtOptions.cat || PbrtOptions.toPly)
                printf("%*sRotate %.9g %.9g %.9g %.9g\n", catIndentCount, "", angle,
                    dx, dy, dz);
    }

    void pbrtScale(float sx, float sy, float sz) {
        VERIFY_INITIALIZED("Scale");
        FOR_ACTIVE_TRANSFORMS(curTransform[i] =
            curTransform[i] * Scale(sx, sy, sz);)
        
    }

    void pbrtLookAt(float ex, float ey, float ez, float lx, float ly, float lz,
        float ux, float uy, float uz) {
        VERIFY_INITIALIZED("LookAt");
        Transform lookAt =
            LookAt(Vector3f(ex, ey, ez), Vector3f(lx, ly, lz), Vector3f(ux, uy, uz));
        FOR_ACTIVE_TRANSFORMS(curTransform[i] = curTransform[i] * lookAt;);
       
    }

    void pbrtCoordinateSystem(const std::string& name) {
        VERIFY_INITIALIZED("CoordinateSystem");
        namedCoordinateSystems[name] = curTransform;
        
    }

    void pbrtCoordSysTransform(const std::string& name) {
        VERIFY_INITIALIZED("CoordSysTransform");
        if (namedCoordinateSystems.find(name) != namedCoordinateSystems.end())
            curTransform = namedCoordinateSystems[name];
        else
            Warning("Couldn't find named coordinate system \"%s\"", name.c_str());
    }

    void pbrtActiveTransformAll() {
        activeTransformBits = AllTransformsBits;
        if (PbrtOptions.cat || PbrtOptions.toPly)
            printf("%*sActiveTransform All\n", catIndentCount, "");
    }

    void pbrtActiveTransformEndTime() {
        activeTransformBits = EndTransformBits;
    }     

    void pbrtActiveTransformStartTime() {
        activeTransformBits = StartTransformBits;
    }

    void pbrtTransformTimes(float start, float end) {
        VERIFY_OPTIONS("TransformTimes");
        renderOptions->transformStartTime = start;
        renderOptions->transformEndTime = end;
       
    }

    void pbrtPixelFilter(const std::string& name, const ParamSet& params) {
        VERIFY_OPTIONS("PixelFilter");
        renderOptions->FilterName = name;
        renderOptions->FilterParams = params;
      
    }

    void pbrtFilm(const std::string& type, const ParamSet& params) {
        VERIFY_OPTIONS("Film");
        renderOptions->FilmParams = params;
        renderOptions->FilmName = type;
        
    }

    void pbrtSampler(const std::string& name, const ParamSet& params) {
        VERIFY_OPTIONS("Sampler");
        renderOptions->SamplerName = name;
        renderOptions->SamplerParams = params;
       
    }

    void pbrtAccelerator(const std::string& name, const ParamSet& params) {
        VERIFY_OPTIONS("Accelerator");
        renderOptions->AcceleratorName = name;
        renderOptions->AcceleratorParams = params;
       
    }

    void pbrtIntegrator(const std::string& name, const ParamSet& params) {
        VERIFY_OPTIONS("Integrator");
        renderOptions->IntegratorName = name;
        renderOptions->IntegratorParams = params;
      
    }

    void pbrtCamera(const std::string& name, const ParamSet& params) {
        VERIFY_OPTIONS("Camera");
        renderOptions->CameraName = name;
        renderOptions->CameraParams = params;
        renderOptions->CameraToWorld = Inverse(curTransform);
        namedCoordinateSystems["camera"] = renderOptions->CameraToWorld;
      
    }

    void pbrtMakeNamedMedium(const std::string& name, const ParamSet& params) {
        VERIFY_INITIALIZED("MakeNamedMedium");
        WARN_IF_ANIMATED_TRANSFORM("MakeNamedMedium");
        std::string type = params.FindOneString("type", "");
        if (type == "")
            Error("No parameter string \"type\" found in MakeNamedMedium");
        else {
            std::shared_ptr<Medium> medium =
                MakeMedium(type, params, curTransform[0]);
            if (medium) renderOptions->namedMedia[name] = medium;
        }       
    }

    void pbrtMediumInterface(const std::string& insideName,
        const std::string& outsideName) {
        VERIFY_INITIALIZED("MediumInterface");
        graphicsState.currentInsideMedium = insideName;
        graphicsState.currentOutsideMedium = outsideName;
        renderOptions->haveScatteringMedia = true;
       
    }

    void pbrtWorldBegin() {
        VERIFY_OPTIONS("WorldBegin");
        currentApiState = APIState::WorldBlock;
        for (int i = 0; i < MaxTransforms; ++i) curTransform[i] = Transform();
        activeTransformBits = AllTransformsBits;
        namedCoordinateSystems["world"] = curTransform;
        
    }

    void pbrtAttributeBegin() {
        VERIFY_WORLD("AttributeBegin");
        pushedGraphicsStates.push_back(graphicsState);
        graphicsState.floatTexturesShared = graphicsState.spectrumTexturesShared =
            graphicsState.namedMaterialsShared = true;
        pushedTransforms.push_back(curTransform);
        pushedActiveTransformBits.push_back(activeTransformBits);
       
    }

    void pbrtAttributeEnd() {
        VERIFY_WORLD("AttributeEnd");
        if (!pushedGraphicsStates.size()) {
            Error(
                "Unmatched pbrtAttributeEnd() encountered. "
                "Ignoring it.");
            return;
        }
        graphicsState = std::move(pushedGraphicsStates.back());
        pushedGraphicsStates.pop_back();
        curTransform = pushedTransforms.back();
        pushedTransforms.pop_back();
        activeTransformBits = pushedActiveTransformBits.back();
        pushedActiveTransformBits.pop_back();
       
    }

    void pbrtTransformBegin() {
        VERIFY_WORLD("TransformBegin");
        pushedTransforms.push_back(curTransform);
        pushedActiveTransformBits.push_back(activeTransformBits);
      
    }

    void pbrtTransformEnd() {
        VERIFY_WORLD("TransformEnd");
        if (!pushedTransforms.size()) {
            Error(
                "Unmatched pbrtTransformEnd() encountered. "
                "Ignoring it.");
            return;
        }
        curTransform = pushedTransforms.back();
        pushedTransforms.pop_back();
        activeTransformBits = pushedActiveTransformBits.back();
        pushedActiveTransformBits.pop_back();
       
    }

    void pbrtTexture(const std::string& name, const std::string& type,
        const std::string& texname, const ParamSet& params) {
        VERIFY_WORLD("Texture");
      

        TextureParams tp(params, params, *graphicsState.floatTextures,
            *graphicsState.spectrumTextures);
        if (type == "float") {
            // Create _Float_ texture and store in _floatTextures_
            if (graphicsState.floatTextures->find(name) !=
                graphicsState.floatTextures->end())
                Warning("Texture \"%s\" being redefined", name.c_str());
            WARN_IF_ANIMATED_TRANSFORM("Texture");
            std::shared_ptr<Texture<float>> ft =
                MakeFloatTexture(texname, curTransform[0], tp);
            if (ft) {
                // TODO: move this to be a GraphicsState method, also don't
                // provide direct floatTextures access?
                if (graphicsState.floatTexturesShared) {
                    graphicsState.floatTextures =
                        std::make_shared<FloatTextureMap>(*graphicsState.floatTextures);
                    graphicsState.floatTexturesShared = false;
                }
                (*graphicsState.floatTextures)[name] = ft;
            }
        }
        else if (type == "color" || type == "spectrum") {
            // Create _color_ texture and store in _spectrumTextures_
            if (graphicsState.spectrumTextures->find(name) !=
                graphicsState.spectrumTextures->end())
                Warning("Texture \"%s\" being redefined", name.c_str());
            WARN_IF_ANIMATED_TRANSFORM("Texture");
            std::shared_ptr<Texture<Spectrum>> st =
                MakeSpectrumTexture(texname, curTransform[0], tp);
            if (st) {
                if (graphicsState.spectrumTexturesShared) {
                    graphicsState.spectrumTextures =
                        std::make_shared<SpectrumTextureMap>(*graphicsState.spectrumTextures);
                    graphicsState.spectrumTexturesShared = false;
                }
                (*graphicsState.spectrumTextures)[name] = st;
            }
        }
        else
            Error("Texture type \"%s\" unknown.", type.c_str());
    }

    void pbrtMaterial(const std::string& name, const ParamSet& params) {
        VERIFY_WORLD("Material");
        ParamSet emptyParams;
        TextureParams mp(params, emptyParams, *graphicsState.floatTextures,
            *graphicsState.spectrumTextures);
        std::shared_ptr<Material> mtl = MakeMaterial(name, mp);
        graphicsState.currentMaterial =
            std::make_shared<MaterialInstance>(name, mtl, params);

       
    }

    void pbrtMakeNamedMaterial(const std::string& name, const ParamSet& params) {
        VERIFY_WORLD("MakeNamedMaterial");
        // error checking, warning if replace, what to use for transform?
        ParamSet emptyParams;
        TextureParams mp(params, emptyParams, *graphicsState.floatTextures,
            *graphicsState.spectrumTextures);
        std::string matName = mp.FindString("type");
        WARN_IF_ANIMATED_TRANSFORM("MakeNamedMaterial");
        if (matName == "")
            Error("No parameter string \"type\" found in MakeNamedMaterial");

        {
            std::shared_ptr<Material> mtl = MakeMaterial(matName, mp);
            if (graphicsState.namedMaterials->find(name) !=
                graphicsState.namedMaterials->end())
                Warning("Named material \"%s\" redefined.", name.c_str());
            if (graphicsState.namedMaterialsShared) {
                graphicsState.namedMaterials =
                    std::make_shared<GraphicsState::NamedMaterialMap>(*graphicsState.namedMaterials);
                graphicsState.namedMaterialsShared = false;
            }
            (*graphicsState.namedMaterials)[name] =
                std::make_shared<MaterialInstance>(matName, mtl, params);
        }
    }

    void pbrtNamedMaterial(const std::string& name) {
        VERIFY_WORLD("NamedMaterial");
        if (PbrtOptions.cat || PbrtOptions.toPly) {
            printf("%*sNamedMaterial \"%s\"\n", catIndentCount, "", name.c_str());
            return;
        }

        auto iter = graphicsState.namedMaterials->find(name);
        if (iter == graphicsState.namedMaterials->end()) {
            Error("NamedMaterial \"%s\" unknown.", name.c_str());
            return;
        }
        graphicsState.currentMaterial = iter->second;
    }

    void pbrtLightSource(const std::string& name, const ParamSet& params) {
        VERIFY_WORLD("LightSource");
        WARN_IF_ANIMATED_TRANSFORM("LightSource");
        MediumInterface mi = graphicsState.CreateMediumInterface();
        std::shared_ptr<Light> lt = MakeLight(name, params, curTransform[0], mi);
        if (!lt)
            Error("LightSource: light type \"%s\" unknown.", name.c_str());
        else
            renderOptions->lights.push_back(lt);
       
    }

    void pbrtAreaLightSource(const std::string& name, const ParamSet& params) {
        VERIFY_WORLD("AreaLightSource");
        graphicsState.areaLight = name;
        graphicsState.areaLightParams = params;
      
    }

    void pbrtShape(const std::string& name, const ParamSet& params) {
        VERIFY_WORLD("Shape");
        std::vector<std::shared_ptr<Primitive>> prims;
        std::vector<std::shared_ptr<AreaLight>> areaLights;
       

        if (!curTransform.IsAnimated()) {
            // Initialize _prims_ and _areaLights_ for static shape

            // Create shapes for shape _name_
            Transform* ObjToWorld = transformCache.Lookup(curTransform[0]);
            Transform* WorldToObj = transformCache.Lookup((curTransform[0]).inverted());
            std::vector<std::shared_ptr<Shape>> shapes =
                MakeShapes(name, ObjToWorld, WorldToObj,
                    graphicsState.reverseOrientation, params);
            if (shapes.empty()) return;
            std::shared_ptr<Material> mtl = graphicsState.GetMaterialForShape(params);
            params.ReportUnused();
            MediumInterface mi = graphicsState.CreateMediumInterface();
            prims.reserve(shapes.size());
            for (auto s : shapes) {
                // Possibly create area light for shape
                std::shared_ptr<AreaLight> area;
                if (graphicsState.areaLight != "") 
                {
                    area = MakeAreaLight(graphicsState.areaLight, curTransform[0],
                        mi, graphicsState.areaLightParams, s);
                    if (area) areaLights.push_back(area);
                }
                prims.push_back(
                    std::make_shared<GeometricPrimitive>(s, mtl, area, mi));
            }
        }
        else {
            // Initialize _prims_ and _areaLights_ for animated shape

            // Create initial shape or shapes for animated shape
            if (graphicsState.areaLight != "")
                Warning(
                    "Ignoring currently set area light when creating "
                    "animated shape");
            Transform* identity = transformCache.Lookup(Transform());
            std::vector<std::shared_ptr<Shape>> shapes = MakeShapes(
                name, identity, identity, graphicsState.reverseOrientation, params);
            if (shapes.empty()) return;

            // Create _GeometricPrimitive_(s) for animated shape
            std::shared_ptr<Material> mtl = graphicsState.GetMaterialForShape(params);
            params.ReportUnused();
            MediumInterface mi = graphicsState.CreateMediumInterface();
            prims.reserve(shapes.size());
            for (auto s : shapes)
                prims.push_back(
                    std::make_shared<GeometricPrimitive>(s, mtl, nullptr, mi));

            // Create single _TransformedPrimitive_ for _prims_

            // Get _animatedObjectToWorld_ transform for shape
            static_assert(MaxTransforms == 2,
                "TransformCache assumes only two transforms");
            Transform* ObjToWorld[2] = {
                transformCache.Lookup(curTransform[0]),
                transformCache.Lookup(curTransform[1])
            };
            AnimatedTransform animatedObjectToWorld(
                ObjToWorld[0], renderOptions->transformStartTime, ObjToWorld[1],
                renderOptions->transformEndTime);
            if (prims.size() > 1) {
                std::shared_ptr<Primitive> bvh = std::make_shared<BVHAccel>(prims);
                prims.clear();
                prims.push_back(bvh);
            }
            prims[0] = std::make_shared<TransformedPrimitive>(
                prims[0], animatedObjectToWorld);
        }
        // Add _prims_ and _areaLights_ to scene or current instance
        if (renderOptions->currentInstance) {
            if (areaLights.size())
                Warning("Area lights not supported with object instancing");
            renderOptions->currentInstance->insert(
                renderOptions->currentInstance->end(), prims.begin(), prims.end());
        }
        else {
            renderOptions->primitives.insert(renderOptions->primitives.end(),
                prims.begin(), prims.end());
            if (areaLights.size())
                renderOptions->lights.insert(renderOptions->lights.end(),
                    areaLights.begin(), areaLights.end());
        }
    }

    // Attempt to determine if the ParamSet for a shape may provide a value for
    // its material's parameters. Unfortunately, materials don't provide an
    // explicit representation of their parameters that we can query and
    // cross-reference with the parameter values available from the shape.
    //
    // Therefore, we'll apply some "heuristics".
    bool shapeMaySetMaterialParameters(const ParamSet& ps) {
        for (const auto& param : ps.textures)
            // Any texture other than one for an alpha mask is almost certainly
            // for a Material (or is unused!).
            if (param->name != "alpha" && param->name != "shadowalpha")
                return true;

        // Special case spheres, which are the most common non-mesh primitive.
        for (const auto& param : ps.floats)
            if (param->nValues == 1 && param->name != "radius")
                return true;

        // Extra special case strings, since plymesh uses "filename", curve "type",
        // and loopsubdiv "scheme".
        for (const auto& param : ps.strings)
            if (param->nValues == 1 && param->name != "filename" &&
                param->name != "type" && param->name != "scheme")
                return true;

        // For all other parameter types, if there is a single value of the
        // parameter, assume it may be for the material. This should be valid
        // (if conservative), since no materials currently take array
        // parameters.
        for (const auto& param : ps.bools)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.ints)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.point2fs)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.vector2fs)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.point3fs)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.vector3fs)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.normals)
            if (param->nValues == 1)
                return true;
        for (const auto& param : ps.spectra)
            if (param->nValues == 1)
                return true;

        return false;
    }

    GraphicsState::GraphicsState() : floatTextures(std::make_shared<FloatTextureMap>()),
        spectrumTextures(std::make_shared<SpectrumTextureMap>()),
        namedMaterials(std::make_shared<NamedMaterialMap>())
    {
        ParamSet empty;
        TextureParams tp(empty, empty, *floatTextures, *spectrumTextures);
        std::shared_ptr<Material> mtl(CreateMatteMaterial(tp));
        currentMaterial = std::make_shared<MaterialInstance>("matte", mtl, ParamSet());
    }

    std::shared_ptr<Material> GraphicsState::GetMaterialForShape(
        const ParamSet& shapeParams) {
        assert(currentMaterial != nullptr);
        
        if (shapeMaySetMaterialParameters(shapeParams)) {
            // Only create a unique material for the shape if the shape's
            // parameters are (apparently) going to provide values for some of
            // the material parameters.
            TextureParams mp(shapeParams, currentMaterial->params, *floatTextures,
                *spectrumTextures);
            return MakeMaterial(currentMaterial->name, mp);
        }
        else
            return currentMaterial->material;
    }

    MediumInterface GraphicsState::CreateMediumInterface() {
        MediumInterface m;
        if (currentInsideMedium != "") {
            if (renderOptions->namedMedia.find(currentInsideMedium) !=
                renderOptions->namedMedia.end())
                m.inside = renderOptions->namedMedia[currentInsideMedium].get();
            else
                Error("Named medium \"%s\" undefined.",
                    currentInsideMedium.c_str());
        }
        if (currentOutsideMedium != "") {
            if (renderOptions->namedMedia.find(currentOutsideMedium) !=
                renderOptions->namedMedia.end())
                m.outside = renderOptions->namedMedia[currentOutsideMedium].get();
            else
                Error("Named medium \"%s\" undefined.",
                    currentOutsideMedium.c_str());
        }
        return m;
    }

    void pbrtReverseOrientation() {
        VERIFY_WORLD("ReverseOrientation");
        graphicsState.reverseOrientation = !graphicsState.reverseOrientation;        
    }

    void pbrtObjectBegin(const std::string& name) {
        VERIFY_WORLD("ObjectBegin");
        pbrtAttributeBegin();
        if (renderOptions->currentInstance)
            Error("ObjectBegin called inside of instance definition");
        renderOptions->instances[name] = std::vector<std::shared_ptr<Primitive>>();
        renderOptions->currentInstance = &renderOptions->instances[name];
    }

  
    void pbrtObjectEnd() {
        VERIFY_WORLD("ObjectEnd");
        if (!renderOptions->currentInstance)
            Error("ObjectEnd called outside of instance definition");
        renderOptions->currentInstance = nullptr;
        pbrtAttributeEnd();      
    }

  //  STAT_COUNTER("Scene/Object instances used", nObjectInstancesUsed);

    void pbrtObjectInstance(const std::string& name) {
        VERIFY_WORLD("ObjectInstance");
        
        // Perform object instance error checking
        if (renderOptions->currentInstance) {
            Error("ObjectInstance can't be called inside instance definition");
            return;
        }
        if (renderOptions->instances.find(name) == renderOptions->instances.end()) {
            Error("Unable to find instance named \"%s\"", name.c_str());
            return;
        }
        std::vector<std::shared_ptr<Primitive>>& in =
            renderOptions->instances[name];
        if (in.empty()) return;
        if (in.size() > 1) {
            // Create aggregate for instance _Primitive_s
            std::shared_ptr<Primitive> accel(
                MakeAccelerator(renderOptions->AcceleratorName, std::move(in),
                    renderOptions->AcceleratorParams));
            if (!accel) accel = std::make_shared<BVHAccel>(in);
            in.clear();
            in.push_back(accel);
        }
        static_assert(MaxTransforms == 2,
            "TransformCache assumes only two transforms");
        // Create _animatedInstanceToWorld_ transform for instance
        Transform* InstanceToWorld[2] = {
            transformCache.Lookup(curTransform[0]),
            transformCache.Lookup(curTransform[1])
        };
        AnimatedTransform animatedInstanceToWorld(
            InstanceToWorld[0], renderOptions->transformStartTime,
            InstanceToWorld[1], renderOptions->transformEndTime);
        std::shared_ptr<Primitive> prim(
            std::make_shared<TransformedPrimitive>(in[0], animatedInstanceToWorld));
        renderOptions->primitives.push_back(prim);
    }

    void pbrtWorldEnd() {
        //VERIFY_WORLD("WorldEnd");
        //// Ensure there are no pushed graphics states
        while (pushedGraphicsStates.size()) {
            Warning("Missing end to pbrtAttributeBegin()");
            pushedGraphicsStates.pop_back();
            pushedTransforms.pop_back();
        }
        while (pushedTransforms.size()) {
            Warning("Missing end to pbrtTransformBegin()");
            pushedTransforms.pop_back();
        }

        // Create scene and render
       
       {
            
            renderOptions->m_integrator.reset(renderOptions->MakeIntegrator());
            renderOptions->m_scene.reset(renderOptions->MakeScene());
            
            
            
            //std::unique_ptr<Integrator> integrator(renderOptions->MakeIntegrator());
            //std::unique_ptr<Scene> scene(renderOptions->MakeScene());

            // This is kind of ugly; we directly override the current profiler
            // state to switch from parsing/scene construction related stuff to
            // rendering stuff and then switch it back below. The underlying
            // issue is that all the rest of the profiling system assumes
            // hierarchical inheritance of profiling state; this is the only
            // place where that isn't the case.
            //CHECK_EQ(CurrentProfilerState(), ProfToBits(Prof::SceneConstruction));
           // ProfilerState = ProfToBits(Prof::IntegratorRender);

           // if (scene && integrator) 
           //    integrator->Render(*scene);

            //CHECK_EQ(CurrentProfilerState(), ProfToBits(Prof::IntegratorRender));
            //ProfilerState = ProfToBits(Prof::SceneConstruction);
       }

       // // Clean up after rendering. Do this before reporting stats so that
       // // destructors can run and update stats as needed.
       // graphicsState = GraphicsState();
       // transformCache.Clear();
       // currentApiState = APIState::OptionsBlock;
       //// ImageTexture<float, float>::ClearCache();
       //// ImageTexture<RGBSpectrum, Spectrum>::ClearCache();
       // renderOptions.reset(new RenderOptions);

        /*if (!PbrtOptions.cat && !PbrtOptions.toPly) {
            MergeWorkerThreadStats();
            ReportThreadStats();
            if (!PbrtOptions.quiet) {
                PrintStats(stdout);
                ReportProfilerResults(stdout);
                ClearStats();
                ClearProfiler();
            }
        }*/

      /*  for (int i = 0; i < MaxTransforms; ++i) 
            curTransform[i] = Transform();
        activeTransformBits = AllTransformsBits;
        namedCoordinateSystems.erase(namedCoordinateSystems.begin(),
            namedCoordinateSystems.end());*/
    }

    Scene* RenderOptions::MakeScene() {
        std::shared_ptr<Primitive> accelerator =
            MakeAccelerator(AcceleratorName, std::move(primitives), AcceleratorParams);
        if (!accelerator) accelerator = std::make_shared<BVHAccel>(primitives);
        Scene* scene = new Scene(accelerator, lights);
        // Erase primitives and lights from _RenderOptions_
        primitives.clear();
        lights.clear();
        return scene;
    }

    Integrator* RenderOptions::MakeIntegrator() const 
    {
        std::shared_ptr<Camera> camera(MakeCamera());
        if (!camera) {
            Error("Unable to create camera");
            return nullptr;
        }

        std::shared_ptr<Sampler> sampler =
            MakeSampler(SamplerName, SamplerParams, camera.get() );
        if (!sampler) {
            Error("Unable to create sampler.");
            return nullptr;
        }

        Integrator* integrator = nullptr;
        if (IntegratorName == "whitted") {
            //integrator = CreateWhittedIntegrator(IntegratorParams, sampler, camera);
        }
        else if (IntegratorName == "directlighting") {
            integrator = CreateDirectLightingIntegrator(IntegratorParams,  camera, sampler);
        }
        else if (IntegratorName == "path") {
            integrator = CreatePathIntegrator(IntegratorParams, camera, sampler);
        }
        else if (IntegratorName == "volpath") {
            integrator = CreateVolPathIntegrator(IntegratorParams, sampler, camera);
        }
        else if (IntegratorName == "bdpt") {
            integrator = CreateBDPTIntegrator(IntegratorParams, sampler, camera);
        }
        else if (IntegratorName == "mlt") {
          //  integrator = CreateMLTIntegrator(IntegratorParams, camera);
        }
        else if (IntegratorName == "ambientocclusion") {
            //integrator = CreateAOIntegrator(IntegratorParams, sampler, camera);
        }
        else if (IntegratorName == "sppm") {
           // integrator = CreateSPPMIntegrator(IntegratorParams, camera);
        }
        else {
            Error("Integrator \"%s\" unknown.", IntegratorName.c_str());
            return nullptr;
        }

        if (renderOptions->haveScatteringMedia && IntegratorName != "volpath" &&
            IntegratorName != "bdpt" && IntegratorName != "mlt") {
            Warning(
                "Scene has scattering media but \"%s\" integrator doesn't support "
                "volume scattering. Consider using \"volpath\", \"bdpt\", or "
                "\"mlt\".", IntegratorName.c_str());
        }

        IntegratorParams.ReportUnused();
        // Warn if no light sources are defined
        if (lights.empty())
            Warning(
                "No light sources defined in scene; "
                "rendering a black image.");
        return integrator;
    }

    Camera* RenderOptions::MakeCamera() const {
        std::unique_ptr<Filter> filter = MakeFilter(FilterName, FilterParams);
        Film* film = MakeFilm(FilmName, FilmParams, std::move(filter));
        if (!film) {
            Error("Unable to create film.");
            return nullptr;
        }
        Camera* camera = RayTrace::MakeCamera(CameraName, CameraParams, CameraToWorld,
            renderOptions->transformStartTime,
            renderOptions->transformEndTime, film);
        return camera;
    }

    void TransformCache::Grow()
    {
        std::vector<Transform*> newTable(2 * hashTable.size());
        //LOG(INFO) << "Growing transform cache hash table to " << newTable.size();

        // Insert current elements into newTable.
        for (Transform* tEntry : hashTable) {
            if (!tEntry) continue;

            int baseOffset = Hash(*tEntry) & (hashTable.size() - 1);
            for (int nProbes = 0;; ++nProbes) {
                // Quadratic probing.
                int offset = (baseOffset + nProbes / 2 + nProbes * nProbes / 2) & (hashTable.size() - 1);
                if (newTable[offset] == nullptr) {
                    newTable[offset] = tEntry;
                    break;
                }
            }
        }

        std::swap(hashTable, newTable);
    }

    uint64_t TransformCache::Hash(const Transform& t)
    {
        const char* ptr = (const char*)(&t.GetMatrix());
        size_t size = sizeof(Matrix4x4);
        uint64_t hash = 14695981039346656037ull;
        while (size > 0) {
            hash ^= *ptr;
            hash *= 1099511628211ull;
            ++ptr;
            --size;
        }
        return hash;
    }

    TransformCache::TransformCache() : hashTable(512), hashTableOccupancy(0)
    {

    }

    Transform* TransformCache::Lookup(const Transform& t)
    {
        int offset = Hash(t) & (hashTable.size() - 1);
        int step = 1;
        while (true) {
            // Keep looking until we find the Transform or determine that
            // it's not present.
            if (!hashTable[offset] || *hashTable[offset] == t)
                break;
            // Advance using quadratic probing.
            offset = (offset + step * step) & (hashTable.size() - 1);
            ++step;
        }

        Transform* tCached = hashTable[offset];
        if (tCached) {
            //++nTransformCacheHits;
        }
        else {
            tCached = arena.Alloc<Transform>();
            *tCached = t;
            Insert(tCached);
        }
        return tCached;
    }

    void TransformCache::Clear()
    {
        //transformCacheBytes += arena.TotalAllocated() + hashTable.size() * sizeof(Transform*);
        hashTable.clear();
        hashTable.resize(512);
        hashTableOccupancy = 0;
        arena.Reset();
    }

    void TransformCache::Insert(Transform* tNew)
    {
        if (++hashTableOccupancy == hashTable.size() / 2)
            Grow();

        int baseOffset = Hash(*tNew) & (hashTable.size() - 1);
        for (int nProbes = 0;; ++nProbes) {
            // Quadratic probing.
            int offset = (baseOffset + nProbes / 2 + nProbes * nProbes / 2) & (hashTable.size() - 1);
            if (hashTable[offset] == nullptr) {
                hashTable[offset] = tNew;
                return;
            }
        }
    }

}

