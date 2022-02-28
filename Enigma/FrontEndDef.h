#pragma once

#include "Defines.h"
#include "RNG.h"

#define  INVALID_ID  0xFFFFFFFF
namespace RayTrace
{
    class HWBufferBase;
    class VertexArrayObject;
    class ShaderProgram;
    class MeshBase;
    class DrawableBase;
    class HWCamera;
    class HWRenderer;
    class HWTexture;
    class HWFrameBuffer;
    class HWViewport;
    class HWMaterial;
    class HWPass;
    class EditorPass;
    class LightBase;
    class Context;
    class SystemBase;
    class SceneView;
    class ResourceManager;
    class ControllerBase;
    class CameraControllerBase;
    class InputHandler;
    class EventHandler;
    class LayerManager;
    class LayerBase;
    class EditorLayer;
    class Logger;
    class Window;
    class WindowManager;
    class ComponentSystem;
    class SceneManager;
    class ObjectBase;
    class HistoryStack;
    class SceneItemCollector;
    class GeometryBase;
    class ModifierBase;
    class WrappedEntity;
    class ModifierStack;
    class ToolBase;
    class ToolManager;
    class SceneSelection;
    class CopyPasteBuffer;
    class UiAction;

    struct EventBase;
    class  StateSystem;
    struct RenderState;
    struct PerLayer;
    struct Transformable;

    struct ImageExInfo;

    struct DrawableObject;
  
    using MeshPtr = std::shared_ptr<MeshBase>;
   
    template<typename T>
    class VertexBuffer;

    template<typename T>
    class Properties;

    using Vec2fBuffer = VertexBuffer<Vector2f>;
    using Vec3fBuffer = VertexBuffer<Vector3f>;
    using Vec4fBuffer = VertexBuffer<Vector4f>;
    using IndexBuffer = VertexBuffer<uint32_t>;

 
  
    //unique_ptr
    using ShaderProgramUPtr   = std::unique_ptr<ShaderProgram>;
    using HWTextureUPtr       = std::unique_ptr<HWTexture>;
    using HWCameraUPtr        = std::unique_ptr<HWCamera>;
    using HWFrameBufferUPtr   = std::unique_ptr<HWFrameBuffer>;
    using ControllerUPtr      = std::unique_ptr<ControllerBase>;
    using SceneViewUPtr       = std::unique_ptr<SceneView>;
    using HWViewportUPtr      = std::unique_ptr<HWViewport>;
    using ControllerBaseUPtr  = std::unique_ptr<ControllerBase>;
    using HWBufferBaseUPtr    = std::unique_ptr<HWBufferBase>;
    using HWMaterialUPtr      = std::unique_ptr<HWMaterial>;
    using HWPassUPtr          = std::unique_ptr<HWPass>;
    using HWRenderStateUPtr   = std::unique_ptr<RenderState>;
    using SceneItemCollectorUPtr = std::unique_ptr<SceneItemCollector>;
    using ModifierStackUPtr   = std::unique_ptr<ModifierStack>;
    using ToolBaseUPtr        = std::unique_ptr<ToolBase>;
    using WrappedEntityUPtr   = std::unique_ptr<WrappedEntity>;
    using DrawableBaseUPtr    = std::unique_ptr<DrawableBase>;

    using ObjVector = std::vector<ObjectBase*>;
    using EntityVector = std::vector<WrappedEntity*>;
    using PassVector = std::vector<HWPass*>;
    using StringVector = std::vector<std::string>;
    using AnyVector    = std::vector<std::any>;

    using ObjectUnderMouse = std::pair<uint32_t, Vector3f>;
    using EntityFilter     = std::function<bool(WrappedEntity* _pObj)>;

	using ActionMap = std::map<std::string, std::shared_ptr<UiAction>>;
    using ActionVector = std::vector<std::shared_ptr<UiAction>>;

    using UUID     = std::string;
    using UUIDVector = std::vector<UUID>;
  
    using EntityId = std::uint32_t;
    using WindowId = std::uint32_t;
    /*
        @brief: function pointer, used for drawing
    */
    using ProcessObjectFun = std::add_pointer<void(HWRenderer*, WrappedEntity*, HWMaterial*, ShaderProgram*)>::type;


    static RNG g_rng;


    enum eLayer : uint32_t
    {
        LAYER_UNDEFINED                 = 0x0,
        
        LAYER_PRE_RENDER                = 0x01,

        LAYER_START = LAYER_PRE_RENDER,
        LAYER_WORLD_SHADOWS             = 0x02,
        LAYER_WORLD_SOLID               = 0x04,
        LAYER_WORLD_TRANS               = 0x08,            
        LAYER_DEBUG_3D                  = 0x10,
        LAYER_EDITOR                    = 0x20,
        LAYER_SKYBOX                    = 0x40,
        LAYER_POST_RENDER               = 0x80,
        LAYER_DEBUG_2D                  = 0x100,
        LAYER_OVERLAY                   = 0x200,
        LAYER_UI                        = 0x400,
        
        LAYER_END                       = LAYER_UI,

        LAYER_ALL                       = 0x0000FFFF
    };

    enum class eLogLevel
    {
        LOG_LEVEL_INFO = 1,
        LOG_LEVEL_WARNING = 2,
        LOG_LEVEL_ERROR = 3,
        LOG_LEVEL_FATAL,
        LOG_LEVEL_DEBUG
    };

    enum class eLightType : uint32_t
    {
        UNDEFINED = 0x0,
        SPOT_LIGHT = 0x01,
        POINT_LIGHT = 0x02,
        DIR_LIGHT = 0x04,
        AREA_LIGHT = 0x08
    };



    

}

