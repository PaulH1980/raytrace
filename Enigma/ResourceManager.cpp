#include "HWRenderer.h"
#include "ShaderSources.h"
#include "Controller.h"
#include "Context.h"
#include "HWShader.h"
#include "HWMaterial.h"
#include "HWVertexBuffer.h"
#include "Geometry.h"
#include "SceneView.h"
#include "HWMesh.h"
#include "HWFrameBuffer.h"
#include "HWStates.h" //TODO make render system opaqe
#include "HWTexture.h"
#include "HWPass.h"
#include "Logger.h"
#include "SceneItemCollector.h"
#include "ResourceManager.h"

template<typename T, typename Container>
bool Contains(const std::string& _name, const Container& _c, std::mutex& _mutex )
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _c.find(_name) != std::end(_c);
}

template<typename T, typename Container>
bool Add(const std::string& _name,  Container& _c, std::unique_ptr<T> _v, std::mutex& _mutex)
{
    
    std::lock_guard<std::mutex> lock(_mutex);
    if (_c.find(_name) != std::end(_c))
        return false;    
  
    _c[_name] = std::move(_v);
    return true;
}

template<typename T, typename Container>
T* Get(const std::string& _name, const Container& _c, std::mutex& _mutex)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_c.find(_name) == std::end(_c))
        return nullptr;
    return _c.at(_name).get();
}



namespace RayTrace
{

    
    ResourceManager::ResourceManager(Context* _pContext) 
        : SystemBase(_pContext)
    {
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    }

    bool ResourceManager::Clear()
    {
        m_cameras.clear();
        m_shaders.clear();
        m_textures.clear();
        m_controllers.clear();
        m_frameBuffers.clear();
        m_sceneViews.clear();
        m_buffers.clear();
        m_materials.clear();
        m_passes.clear();
        m_renderStates.clear();
        m_collectors.clear();
        return true;
    }

  

    ShaderProgram* ResourceManager::GetShader(const std::string& _name) const
    {
        
        return Get<ShaderProgram,ShaderMap>(_name, m_shaders, m_shaderMutex );
    }

    bool ResourceManager::AddShader(const std::string& _name, ShaderProgramUPtr _shader)
    {
        return Add<ShaderProgram, ShaderMap>(_name, m_shaders, std::move(_shader ), m_shaderMutex);
    }

    bool ResourceManager::ContainsShader(const std::string& _name) const
    {
        return Contains<ShaderProgram, ShaderMap>(_name, m_shaders, m_shaderMutex);
    }

    HWCamera* ResourceManager::GetCamera(const std::string& _name) const
    {
        return Get<HWCamera, CameraMap>(_name, m_cameras, m_cameraMutex);
    }

    bool ResourceManager::AddCamera(const std::string& _name,  HWCameraUPtr _cam)
    {
        return Add<HWCamera, CameraMap>(_name, m_cameras, std::move(_cam), m_cameraMutex);
    }

    bool ResourceManager::ContainsCamera(const std::string& _name) const
    {
        return Contains<HWCamera, CameraMap>(_name, m_cameras, m_cameraMutex);
    }

    HWTexture* ResourceManager::GetTexture(const std::string& _name) const
    {
        return Get<HWTexture, TextureMap>(_name, m_textures, m_textureMutex);
    }

    bool ResourceManager::AddTexture(const std::string& _name,  HWTextureUPtr _tex)
    {
        _tex->m_texInfo.m_texName = _name;
        return Add<HWTexture, TextureMap>(_name, m_textures, std::move(_tex), m_textureMutex);
    }

    bool ResourceManager::ContainsTexture(const std::string& _name) const
    {
        return Contains<HWTexture, TextureMap>(_name, m_textures, m_textureMutex);
    }

    ControllerBase* ResourceManager::GetController(const std::string& _name) const
    {
        return Get<ControllerBase, ControllerMap>(_name, m_controllers, m_controllerMutex);
    }

    bool ResourceManager::AddController(const std::string& _name, ControllerUPtr _con)
    {
        return Add<ControllerBase, ControllerMap>(_name, m_controllers, std::move(_con), m_controllerMutex);
    }

    bool ResourceManager::ContainsController(const std::string& _name) const
    {
        return Contains<ControllerBase, ControllerMap>(_name, m_controllers, m_controllerMutex);
    }

    HWFrameBuffer* ResourceManager::GetFrameBuffer(const std::string& _name) const
    {
        return Get<HWFrameBuffer, FrameBufferMap>(_name, m_frameBuffers , m_frameBufferMutex);
    }

    bool ResourceManager::AddFrameBuffer(const std::string& _name, HWFrameBufferUPtr _frameBuffer)
    {
        return Add<HWFrameBuffer, FrameBufferMap>(_name, m_frameBuffers, std::move(_frameBuffer), m_frameBufferMutex);
    }

    bool ResourceManager::ContainsFrameBuffer(const std::string& _name) const
    {
        return Contains<HWFrameBuffer, FrameBufferMap>(_name, m_frameBuffers, m_frameBufferMutex);
    }

    SceneView* ResourceManager::GetSceneView(const std::string& _name) const
    {
        return Get<SceneView, SceneViewMap>(_name, m_sceneViews, m_sceneViewMutex);
    }

    bool ResourceManager::AddSceneView(const std::string& _name, SceneViewUPtr _view)
    {
        return Add<SceneView, SceneViewMap>(_name, m_sceneViews, std::move(_view), m_sceneViewMutex );
    }

    bool ResourceManager::ContainsSceneView(const std::string& _name) const
    {
        return Contains<SceneView, SceneViewMap>(_name, m_sceneViews, m_sceneViewMutex);
    }

    HWBufferBase* ResourceManager::GetBuffer(const std::string& _name) const
    {
        return Get<HWBufferBase, BufferMap>(_name, m_buffers, m_bufferMutex );
    }

    bool ResourceManager::AddBuffer(const std::string& _name, HWBufferBaseUPtr _buffer)
    {
        return Add<HWBufferBase, BufferMap>(_name, m_buffers, std::move(_buffer), m_bufferMutex );
    }

    bool ResourceManager::ContainsBuffer(const std::string& _name) const
    {
        return Contains<HWBufferBase, BufferMap>(_name, m_buffers, m_bufferMutex );
    }

    HWMaterial* ResourceManager::GetMaterial(const std::string& _name) const
    {
        return Get<HWMaterial, MaterialMap>(_name, m_materials, m_materialMutex );
    }

    bool ResourceManager::AddMaterial(const std::string& _name, HWMaterialUPtr _mat)
    {
        return Add<HWMaterial, MaterialMap>(_name, m_materials, std::move(_mat), m_materialMutex );
    }

    bool ResourceManager::ContainsMaterial(const std::string& _name) const
    {
        return Contains<HWMaterial, MaterialMap>(_name, m_materials, m_materialMutex);
    }

    HWPass* ResourceManager::GetPass(const std::string& _name) const
    {
        return Get<HWPass, PassMap>(_name, m_passes, m_passeMutex);
    }

    bool ResourceManager::AddPass(const std::string& _name, HWPassUPtr _pass)
    {
        return Add<HWPass, PassMap>(_name, m_passes, std::move(_pass), m_passeMutex);
    }

    bool ResourceManager::ContainsPass(const std::string& _name) const
    {
        return Contains<HWPass, PassMap>(_name, m_passes, m_passeMutex);
    }

    RenderState* ResourceManager::GetRenderState(const std::string& _name) const
    {
        return Get<RenderState, RenderStateMap>(_name, m_renderStates, m_renderStateMutex);
    }

    bool ResourceManager::AddRenderState(const std::string& _name, HWRenderStateUPtr _state)
    {
        return Add<RenderState, RenderStateMap>(_name, m_renderStates, std::move(_state), m_renderStateMutex);
    }

    bool ResourceManager::ContainsRenderState(const std::string& _name) const
    {
        return Contains<RenderState, RenderStateMap>(_name, m_renderStates, m_renderStateMutex);
    }

    SceneItemCollector* ResourceManager::GetSceneCollector(const std::string& _name) const
    {
        return Get<SceneItemCollector, ItemCollectorMap>(_name, m_collectors, m_collectorMutex);
    }

    bool ResourceManager::AddSceneCollector(const std::string& _name, SceneItemCollectorUPtr _collector)
    {
        return Add<SceneItemCollector, ItemCollectorMap>(_name, m_collectors, std::move(_collector ), m_collectorMutex);
    }

    bool ResourceManager::ContainsSceneCollector(const std::string& _name) const
    {
        return Contains<SceneItemCollector, ItemCollectorMap>(_name, m_collectors, m_collectorMutex);
    }

    ModifierStack* ResourceManager::GetModifier(const std::string& _name) const
    {
        return Get<ModifierStack, ModifierStackMap>(_name, m_modifiers, m_modifierMutex);
    }

    bool ResourceManager::AddModifier(const std::string& _name, ModifierStackUPtr _modifier)
    {
        return Add<ModifierStack, ModifierStackMap>(_name, m_modifiers, std::move(_modifier), m_modifierMutex);
    }

    bool ResourceManager::ContainsModifier(const std::string& _name) const
    {
        return Contains<ModifierStack, ModifierStackMap>(_name, m_modifiers, m_modifierMutex);
    }

    DrawableBase* ResourceManager::GetDrawable(const std::string& _name) const
    {
        return Get<DrawableBase, DrawableMap>(_name, m_drawables, m_drawableMutex);
    }

    bool ResourceManager::AddDrawable(const std::string& _name, DrawableBaseUPtr _drawable)
    {
        return Add<DrawableBase, DrawableMap>(_name, m_drawables, std::move(_drawable), m_drawableMutex);
    }

    bool ResourceManager::ContainsDrawable(const std::string& _name) const
    {
        return Contains<DrawableBase, DrawableMap>(_name, m_drawables, m_drawableMutex);
    }

    std::vector<HWTexture*> ResourceManager::GetTextures( bool _regularTextures, bool _fboTextures /*= true*/) const
    {
        std::vector<HWTexture*> retVal;
        if(_regularTextures)
        {
            std::lock_guard<std::mutex> lock(m_textureMutex);
            for (auto& [key, value] : m_textures) {
                retVal.push_back(value.get());
            }
        }
        if( _fboTextures )
        {
            std::lock_guard<std::mutex> lock(m_frameBufferMutex);
            for (auto& [key, value] : m_frameBuffers) {
                for (auto& texPtr : value->GetTextures()) {
                    retVal.push_back(texPtr.get());
                }
                if (auto& depthTex = value->GetDepthTexture()) {
                    retVal.push_back(depthTex.get());
                }
            }
        }
        return retVal;
    }

#if _DEBUG
    ResourceManager::TextureMap& ResourceManager::GetTextureMap()
    {
        return m_textures;
    }

    ResourceManager::FrameBufferMap& ResourceManager::GetFrameBufferMap()
    {
        return m_frameBuffers;
    }
#endif

}

