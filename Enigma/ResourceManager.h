#pragma once

#include "Defines.h"
#include "FrontEndDef.h"
#include "SystemBase.h"

namespace RayTrace
{
    class ResourceManager : public SystemBase
    {
        RT_OBJECT(ResourceManager, SystemBase)

    public:

        ResourceManager(Context* _pContext);
        

        bool                Clear() override;
      
        //Shader Stuff
        ShaderProgram*      GetShader(const std::string& _name) const;
        bool                AddShader(const std::string& _name, ShaderProgramUPtr _shader);
        bool                ContainsShader(const std::string& _name) const;

        //Camera Stuff
        HWCamera*           GetCamera(const std::string& _name) const;
        bool                AddCamera(const std::string& _name, HWCameraUPtr _cam);
        bool                ContainsCamera(const std::string& _name) const;

        //Texture Stuff
        HWTexture*          GetTexture(const std::string& _name) const;
        bool                AddTexture(const std::string& _name, HWTextureUPtr _tex);
        bool                ContainsTexture(const std::string& _name) const;

        //Controller Stuff
        ControllerBase*     GetController(const std::string& _name) const;
        bool                AddController(const std::string& _name, ControllerUPtr _con);
        bool                ContainsController(const std::string& _name) const;

        //FrameBuffer Stuff
        HWFrameBuffer*      GetFrameBuffer(const std::string& _name) const;
        bool                AddFrameBuffer(const std::string& _name, HWFrameBufferUPtr _frameBuffer);
        bool                ContainsFrameBuffer(const std::string& _name) const;

        //SceneView Stuff
        SceneView*          GetSceneView(const std::string& _name) const;
        bool                AddSceneView(const std::string& _name, SceneViewUPtr _view);
        bool                ContainsSceneView(const std::string& _name) const;

        //Buffer Stuff
        HWBufferBase*       GetBuffer(const std::string& _name) const;
        bool                AddBuffer(const std::string& _name, HWBufferBaseUPtr _buffer);
        bool                ContainsBuffer(const std::string& _name) const;

        //Material Stuff
        HWMaterial*         GetMaterial(const std::string& _name) const;
        bool                AddMaterial(const std::string& _name,  HWMaterialUPtr _mat);
        bool                ContainsMaterial(const std::string& _name) const;

        //Pass Stuff
        HWPass*             GetPass(const std::string& _name) const;
        bool                AddPass(const std::string& _name, HWPassUPtr _pass);
        bool                ContainsPass(const std::string& _name) const;
        
        //States Stuff
        RenderState*        GetRenderState(const std::string& _name) const;
        bool                AddRenderState(const std::string& _name, HWRenderStateUPtr  _state);
        bool                ContainsRenderState(const std::string& _name) const;

        SceneItemCollector* GetSceneCollector(const std::string& _name) const;
        bool                AddSceneCollector(const std::string& _name, SceneItemCollectorUPtr  _collector);
        bool                ContainsSceneCollector(const std::string& _name) const;

        ModifierStack*      GetModifier(const std::string& _name) const;
        bool                AddModifier(const std::string& _name, ModifierStackUPtr  _modifier);
        bool                ContainsModifier(const std::string& _name) const;

        DrawableBase *      GetDrawable(const std::string& _name) const;
        bool                AddDrawable(const std::string& _name, DrawableBaseUPtr  _modifier);
        bool                ContainsDrawable(const std::string& _name) const;

        std::vector<HWTexture*> GetTextures(bool _regularTextures, bool _fboTextures = true) const;



        using ShaderMap = std::map<std::string, ShaderProgramUPtr>;
        using CameraMap = std::map< std::string, HWCameraUPtr>;
        using TextureMap = std::map<std::string, HWTextureUPtr>;
        using ControllerMap = std::map<std::string, ControllerUPtr >;
        using FrameBufferMap = std::map<std::string, HWFrameBufferUPtr>;
        using SceneViewMap = std::map<std::string, SceneViewUPtr>;
        using BufferMap = std::map<std::string, HWBufferBaseUPtr>;
        using MaterialMap = std::map<std::string, HWMaterialUPtr>;
        using PassMap = std::map<std::string, HWPassUPtr>;
        using RenderStateMap = std::map<std::string, HWRenderStateUPtr>;
        using ItemCollectorMap = std::map<std::string, SceneItemCollectorUPtr>;
        using ModifierStackMap = std::map<std::string, ModifierStackUPtr>;
        using DrawableMap = std::map<std::string, DrawableBaseUPtr>;

#if _DEBUG
        TextureMap&     GetTextureMap();
        FrameBufferMap& GetFrameBufferMap();
#endif

    private:

        CameraMap        m_cameras;
        ShaderMap        m_shaders;
        TextureMap       m_textures;
        ControllerMap    m_controllers;
        FrameBufferMap   m_frameBuffers;
        SceneViewMap     m_sceneViews;
        BufferMap        m_buffers;   
        MaterialMap      m_materials;
        PassMap          m_passes;
        RenderStateMap   m_renderStates;
        ItemCollectorMap m_collectors;
        ModifierStackMap m_modifiers;
        DrawableMap      m_drawables;

        mutable std::mutex m_cameraMutex;
        mutable std::mutex m_shaderMutex;
        mutable std::mutex m_textureMutex;
        mutable std::mutex m_controllerMutex;
        mutable std::mutex m_frameBufferMutex;
        mutable std::mutex m_sceneViewMutex;
        mutable std::mutex m_bufferMutex;
        mutable std::mutex m_materialMutex;
        mutable std::mutex m_passeMutex;
        mutable std::mutex m_renderStateMutex;
        mutable std::mutex m_collectorMutex;
        mutable std::mutex m_modifierMutex;
        mutable std::mutex m_drawableMutex;

    };
}