
#include "SceneView.h"
#include "Context.h"
#include "ResourceManager.h"
#include "TriangleMesh.h"
#include "ShaderSources.h"
#include "HWFrameBuffer.h"
#include "HWShader.h"
#include "HWVertexBuffer.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "HWTexture.h"
#include "Geometry.h"
#include "FilePaths.h"
#include "Logger.h"
#include "HWRenderer.h"

namespace RayTrace
{

    void GLAPIENTRY
        MessageCallback(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar* message,
            const void* userParam)
    {
        
        std::string _source;
        std::string _type;
        std::string _severity;
        std::string _message = message;


        switch (source) {
        case GL_DEBUG_SOURCE_API:
            _source = "API";
            break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            _source = "WINDOW SYSTEM";
            break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            _source = "SHADER COMPILER";
            break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
            _source = "THIRD PARTY";
            break;

        case GL_DEBUG_SOURCE_APPLICATION:
            _source = "APPLICATION";
            break;

        case GL_DEBUG_SOURCE_OTHER:
            _source = "UNKNOWN";
            break;

        default:
            _source = "UNKNOWN";
            break;
        }

        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            _type = "ERROR";
            break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            _type = "DEPRECATED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            _type = "UDEFINED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_PORTABILITY:
            _type = "PORTABILITY";
            break;

        case GL_DEBUG_TYPE_PERFORMANCE:
            _type = "PERFORMANCE";
            break;

        case GL_DEBUG_TYPE_OTHER:
            _type = "OTHER";
            break;

        case GL_DEBUG_TYPE_MARKER:
            _type = "MARKER";
            break;

        default:
            _type = "UNKNOWN";
            break;
        }

        switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            _severity = "HIGH";
            break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            _severity = "MEDIUM";
            break;

        case GL_DEBUG_SEVERITY_LOW:
            _severity = "LOW";
            break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
            _severity = "NOTIFICATION";
            break;

        default:
            _severity = "UNKNOWN";
            break;
        }

        auto context = (Context*)(userParam);
        if (context)
        {
            if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
            {
                AddLogMessage(context, _message, eLogLevel::LOG_LEVEL_WARNING);
            }
        }
    }

    
    Vector2f QuadVertices[4]
    {
        {-1.f, 1.f},   //top right
        {-1.f, -1.f},   //top left
        {1.f, 1.f}, //bottom left
        {1.f, -1.f}, //bottom right            
    };    

    Vector2f QuadTexCoords[4]
    {
        {0.f, 1.f}, //top right
        {0.f, 0.f}, //top left
        {1.f, 1.f}, //bottom left
        {1.f, 0.f}, //bottom right            
    };

    uint32_t QuadIndices[6]{
        0, 1, 2,
        2, 3, 0
    };

    HWFrameBuffer* EquirectangularToCube(Context* _pContext, const std::string& _fileName, int _dims)
    {
        HWTexInfo info;
        auto texPtr = CreateTexture(_pContext, _fileName, info);
        if (!texPtr)
        {
            AddLogMessage(_pContext, "Failed To Create Texture For Rendering", eLogLevel::LOG_LEVEL_WARNING);
            return nullptr;
        }

        ModifierStack modStack(_pContext);
        //std::unique_ptr<ModifierStack> cubeMode = std::make_unique<ModifierStack>(_pContext);
        modStack.AddModifier(std::make_unique<CubeModifier>(_pContext));
        modStack.Apply();

        auto geometry = modStack.GetGeometry();
     

        HWTexInfo fbParams;
        fbParams.m_target = eTarget::TARGET_TEXTURE_CUBE_MAP;
        fbParams.m_wrapR = fbParams.m_wrapS = fbParams.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
        fbParams.m_minFilter = eMinFilter::MIN_FILTER_LINEAR;
        fbParams.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
        fbParams.m_width = fbParams.m_height = _dims;
        fbParams.m_mips = false;

        auto& renderer = _pContext->GetRenderer();

        auto fboPtr = CreateFrameBuffer(_pContext,
            { eInternalFormat::INTERNAL_RGB16F,  eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, fbParams, false);

        
        auto shader = _pContext->GetResourceManager().GetShader("__DefaultEquirectangularShader");
        shader->Bind();

        auto viewport = HWViewport();
        viewport.m_frameBuffer = fboPtr;
        viewport.m_size = { fboPtr->GetWidth(), fboPtr->GetHeight() };
        renderer.PushViewport(viewport); 

        auto state = renderer.GetInitialState();
        state.enable.field.cull_face = false;
        state.enable.field.depth_test = false;
        state.depth.func = GL_ALWAYS;
        renderer.PushState(state);

        CubemapMatrices matrices;
        int texId = 0;
        texPtr->Bind(texId);
        shader->SetInt("tex", &texId);
        shader->SetMatrix4x4("proj", &matrices.m_proj);

        for (int i = 0; i < 6; ++i)
        {
            shader->SetMatrix4x4("view", &matrices.m_views[i]);            
            fboPtr->AttachCubemapFace(i, 0, 0);
            renderer.Clear(viewport.m_clearData);          
            renderer.DrawTriangles(eDrawMode::DRAWMODE_TRIANGLES, geometry->GetVertices(), geometry->GetVertexCount(),
                shader, nullptr, nullptr,nullptr);
        }

        renderer.PopState();
        texPtr->UnBind();
        shader->UnBind();
        renderer.PopViewport();

        return fboPtr;
    }

    HWFrameBuffer* ConvolveCubemap(Context* _pContext, HWTexture* _cubeTex, int _dims)
    {
        ModifierStack modStack(_pContext);
        //std::unique_ptr<ModifierStack> cubeMode = std::make_unique<ModifierStack>(_pContext);
        modStack.AddModifier(std::make_unique<CubeModifier>(_pContext));
        modStack.Apply();

        auto geometry = modStack.GetGeometry();


        HWTexInfo fbParams;
        fbParams.m_target = eTarget::TARGET_TEXTURE_CUBE_MAP;
        fbParams.m_wrapR = fbParams.m_wrapS = fbParams.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
        fbParams.m_minFilter = eMinFilter::MIN_FILTER_LINEAR;
        fbParams.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
        fbParams.m_width = fbParams.m_height = _dims;
        fbParams.m_mips = false;

        auto& renderer = _pContext->GetRenderer();

        auto fboPtr = CreateFrameBuffer(_pContext,
            { eInternalFormat::INTERNAL_RGB16F,  eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, fbParams, false);


        auto shader = _pContext->GetResourceManager().GetShader("__ConvoleCubemapShader");
        shader->Bind();

        auto viewport = HWViewport();
        viewport.m_frameBuffer = fboPtr;
        viewport.m_size = { fboPtr->GetWidth(), fboPtr->GetHeight() };
        renderer.PushViewport(viewport);

        auto state = renderer.GetInitialState();
        state.enable.field.cull_face = false;
        state.enable.field.depth_test = false;
        state.depth.func = GL_ALWAYS;
        renderer.PushState(state);

        CubemapMatrices matrices;
        int texId = 0;
        _cubeTex->Bind(texId);
        shader->SetInt("tex", &texId);
        shader->SetMatrix4x4("proj", &matrices.m_proj);

        for (int i = 0; i < 6; ++i)
        {
            shader->SetMatrix4x4("view", &matrices.m_views[i]);
            fboPtr->AttachCubemapFace(i, 0, 0);
            renderer.Clear(viewport.m_clearData);
            renderer.DrawTriangles( eDrawMode::DRAWMODE_TRIANGLES,  geometry->GetVertices(), geometry->GetVertexCount(),
                shader, nullptr, nullptr, nullptr);
        }

        renderer.PopState();
        _cubeTex->UnBind();
        shader->UnBind();
        renderer.PopViewport();

        return fboPtr;
    }

    HWFrameBuffer* FilterEnvIbl(Context* _pContext, HWTexture* _cubeTex, int _dims /*= 128*/)
    {
        ModifierStack modStack(_pContext);
        //std::unique_ptr<ModifierStack> cubeMode = std::make_unique<ModifierStack>(_pContext);
        modStack.AddModifier(std::make_unique<CubeModifier>(_pContext));
        modStack.Apply();

        auto geometry = modStack.GetGeometry();


        HWTexInfo fbParams;
        fbParams.m_target = eTarget::TARGET_TEXTURE_CUBE_MAP;
        fbParams.m_wrapR = fbParams.m_wrapS = fbParams.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
        fbParams.m_minFilter = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;
        fbParams.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
        fbParams.m_width = fbParams.m_height = _dims;
        fbParams.m_mips = true;

        auto& renderer = _pContext->GetRenderer();

        auto fboPtr = CreateFrameBuffer(_pContext,
            { eInternalFormat::INTERNAL_RGB16F,  eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, fbParams, false);


        auto shader = _pContext->GetResourceManager().GetShader("__DefaultPrefilterEnvIblShader");
        shader->Bind();

        auto viewport = HWViewport();
        viewport.m_frameBuffer = fboPtr;
        viewport.m_size = { fboPtr->GetWidth(), fboPtr->GetHeight() };
        renderer.PushViewport(viewport);

        auto state = renderer.GetInitialState();
        state.enable.field.cull_face = false;
        state.enable.field.depth_test = false;
        state.depth.func = GL_ALWAYS;
        renderer.PushState(state);

        CubemapMatrices matrices;
        int texId = 0;
        _cubeTex->Bind(texId);
        shader->SetInt("tex", &texId);
        shader->SetMatrix4x4("proj", &matrices.m_proj);

        const int MAX_MIP_LEVELS = 5;
        for (int mip = 0; mip < MAX_MIP_LEVELS; ++mip) {

            const uint32_t mipWidth  = _dims * std::pow(0.5, mip);
            const uint32_t mipHeight = _dims * std::pow(0.5, mip);

            HWViewport vp;
            vp.m_size.x = mipWidth;
            vp.m_size.y = mipHeight;

            renderer.ApplyViewport(vp);
            
            float roughness = (float)mip / (float)(MAX_MIP_LEVELS - 1);
            float resolution = mipWidth;
            shader->SetFloat("roughness", &roughness);
            shader->SetFloat("resolution", &resolution);

            for (int side = 0; side < 6; ++side)
            {
                shader->SetMatrix4x4("view", &matrices.m_views[side]);
                fboPtr->AttachCubemapFace(side, 0, mip);
                renderer.Clear(viewport.m_clearData);
                renderer.DrawTriangles( eDrawMode::DRAWMODE_TRIANGLES, geometry->GetVertices(), geometry->GetVertexCount(),
                    shader, nullptr, nullptr, nullptr);
            }
        }       

        renderer.PopState();
        _cubeTex->UnBind();
        shader->UnBind();
        renderer.PopViewport();

        return fboPtr;
        
    }

    HWFrameBuffer* CreateBrdfLut(Context* _pContext, int _dims)
    {

        HWTexInfo fbParams;
        fbParams.m_target = eTarget::TARGET_TEXTURE_2D;
        fbParams.m_wrapR = fbParams.m_wrapS = fbParams.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
        fbParams.m_minFilter = eMinFilter::MIN_FILTER_LINEAR;
        fbParams.m_magFilter = eMagFilter::MAG_FILTER_NEAREST;
        fbParams.m_width = fbParams.m_height = _dims;
        fbParams.m_mips = false;

        auto& renderer = _pContext->GetRenderer();

        auto fboPtr = CreateFrameBuffer(_pContext,
            { eInternalFormat::INTERNAL_RGBA16F,  eInternalFormat::INTERNAL_DEPTH24_STENCIL8 }, fbParams, false);


        auto shader = _pContext->GetResourceManager().GetShader("__DefaultBrdfLut");
        shader->Bind();
        
        auto viewport = HWViewport();
        viewport.m_frameBuffer = fboPtr;
        viewport.m_size = { fboPtr->GetWidth(), fboPtr->GetHeight() };
        renderer.PushViewport(viewport);
         
        renderer.DrawQuad(-1.0f, 1.0f,-1.0f, 1.0f);
        shader->UnBind();
        renderer.PopViewport();

        return fboPtr;
    }

    bool CreateViewportAllignedQuad(const HWCamera* _pCamera, const Vector3f& _center, 
        const Vector2f& _scale, Vector3f* _pos, Vector2f* _uvs)
    {
       static const int TOPRIGHT = 2;
       static const int TOPLEFT = 0;
       static const int BOTLEFT = 1;
       static const int BOTRIGHT = 3;
        
        
        auto projLightA = _pCamera->WorldToScreen(_center);
        auto projLightB = _pCamera->WorldToScreen(_center + _pCamera->GetRight());
        if (HasInfinities(projLightA) || HasNans(projLightA) || HasNegatives(projLightA))
            return false;


        auto width = projLightB.x - projLightA.x;
        auto scale = _scale / width;
        const Vector3f offsetRight = (_pCamera->GetRight() * scale.x) * 0.5f;
        const Vector3f offsetUp = (_pCamera->GetUp() * scale.y) * 0.5f;

        const Vector3f topRight  = _center + offsetRight + offsetUp;
        const Vector3f topLeft   = _center - offsetRight + offsetUp;
        const Vector3f botLeft   = _center - offsetRight - offsetUp;
        const Vector3f botRight  = _center + offsetRight - offsetUp;
     

        _pos[0] = topLeft;
        _pos[1] = botLeft;
        _pos[2] = topRight;
        _pos[3] = botRight;

        if (_uvs)
        {
            _uvs[0] = QuadTexCoords[TOPRIGHT ];
            _uvs[1] = QuadTexCoords[TOPLEFT  ];
            _uvs[2] = QuadTexCoords[BOTLEFT  ];
            _uvs[3] = QuadTexCoords[BOTRIGHT ];

            _uvs[0] = QuadTexCoords[TOPLEFT];
            _uvs[1] = QuadTexCoords[BOTLEFT];
            _uvs[2] = QuadTexCoords[TOPRIGHT];
            _uvs[3] = QuadTexCoords[BOTRIGHT];

        }


        return true;
    }

    bool CreateEnvironmentData(Context* _pContext, const std::string& _fileName, int _dims, int _irradianceDims, int _reflDims )
    {
        if (!Exists(_fileName))
        {
            AddLogMessage(_pContext, fmt::format("Environment Texture {} Not Found", _fileName), eLogLevel::LOG_LEVEL_WARNING);
            return false;
        }

        HWFrameBufferUPtr cubeMap(EquirectangularToCube(_pContext, _fileName, _dims));
        if (!cubeMap)
            return false;
        bool valid = true;
        valid &= _pContext->GetResourceManager().AddTexture("Env_Cube", std::move(cubeMap->MoveTextures()[0]));
        {
            auto cubeTex = _pContext->GetResourceManager().GetTexture("Env_Cube");
            HWFrameBufferUPtr convolveFbo(ConvolveCubemap(_pContext, cubeTex, _irradianceDims));
            HWFrameBufferUPtr prefilteredFbo(FilterEnvIbl(_pContext, cubeTex, _reflDims));

            valid &= _pContext->GetResourceManager().AddTexture("Env_Irradiance", std::move(convolveFbo->MoveTextures()[0]));
            valid &= _pContext->GetResourceManager().AddTexture("Env_PreFiltered", std::move(prefilteredFbo->MoveTextures()[0]));
        }
        return valid;
    }

    bool CreateEnvironmentDataFromResource(Context* _pContext, const std::string& _cubemapResource, int _irradianceDims, int _reflDims)
    {

        if (glGetError() != GL_NO_ERROR)
            throw std::exception("Texture Creation Failure");

        
        auto cubeTex = _pContext->GetResourceManager().GetTexture(_cubemapResource);
        assert(cubeTex);
        if (!cubeTex->IsCube())
        {
            AddLogMessage(_pContext, fmt::format("Texture {}, Is Not A Cubemap", _cubemapResource), eLogLevel::LOG_LEVEL_WARNING);
            return false;
        }

        HWFrameBufferUPtr convolveFbo(ConvolveCubemap(_pContext, cubeTex, _irradianceDims));
        HWFrameBufferUPtr prefilteredFbo(FilterEnvIbl(_pContext, cubeTex, _reflDims));
        
        bool valid = true;
        valid &= _pContext->GetResourceManager().AddTexture("Env_Irradiance", std::move(convolveFbo->MoveTextures()[0]));
        valid &= _pContext->GetResourceManager().AddTexture("Env_PreFiltered", std::move(prefilteredFbo->MoveTextures()[0]));
        
        return valid;
    }

    //Simple helper
    struct DrawableObject
    {
        DrawableObject(std::vector<HWBufferBase*> _buffers, HWBufferBase* _pIndexBuffer) 
        {
            m_vao.reset(new VertexArrayObject(_buffers.data(), _buffers.size()));
            
            for (auto pBuf : _buffers)
                m_vertexBuffers.push_back(std::unique_ptr<HWBufferBase>(pBuf));
            if( _pIndexBuffer )
                m_indexBuffer.reset(_pIndexBuffer);
        }
        
        
        std::vector<std::unique_ptr<HWBufferBase>>   m_vertexBuffers;
        std::unique_ptr<HWBufferBase>                m_indexBuffer;
        std::unique_ptr<VertexArrayObject>           m_vao;
    };

   
    
    
    ////////////////////////////////////////////////////////////////////////////
    ////HWRenderer Implementation
    ////////////////////////////////////////////////////////////////////////////
    HWRenderer::HWRenderer(Context* _context)
        : SystemBase( _context )
    {
        m_pContext->GetLogger().AddMessage( std::string("Created ") + GetTypeNameStatic() );    


        auto v  = CreateVec2fBuffer(QuadVertices, 4, 0, eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_DRAW);
        auto uv = CreateVec2fBuffer(QuadTexCoords, 4, 1);
        m_drawables["ViewportQuad"].reset(new DrawableObject({ v, uv }, nullptr));

        glFrontFace(GL_CCW);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

#if _DEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, _context);
#endif

        
    }

    HWRenderer::~HWRenderer()
    {

    }

   

    bool HWRenderer::PostConstructor()
    {
        m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
        
        glGenVertexArrays(1, &m_defaultVao);
        glBindVertexArray(m_defaultVao);

        const auto& viewport = m_pContext->GetResourceManager().GetSceneView("__DefaultSceneView")->m_viewport;

        ApplyViewport(viewport);
        m_viewportStack.push(viewport);
        //fetch & store 'default'state
        BeginRender();
        m_initState.getGL();
        m_initState.enable.field.depth_test = true;
        m_initState.enable.field.cull_face = true;
        m_pContext->GetResourceManager().AddRenderState("__DefaultRenderState", std::unique_ptr<RenderState>(m_initState.Clone()));
      
       
        m_initState.applyGL();
        m_stateStack.push(m_initState);
        EndRender();
        //add default render state      
      
        return true;
    }

    const HWViewport& HWRenderer::GetViewport() const
    {
        return m_viewportStack.top();
    }

    void HWRenderer::Clear(const FrameBufferClear& _v)
    {
        uint32_t mask = 0;
        if (_v.m_mask & CLEAR_COLOR)
            mask |= GL_COLOR_BUFFER_BIT;
        if (_v.m_mask & CLEAR_DEPTH)
            mask |= GL_DEPTH_BUFFER_BIT;
        if (_v.m_mask & CLEAR_STENCIL)
            mask |= GL_STENCIL_BUFFER_BIT;
        glClear(mask);
        if( mask& GL_COLOR_BUFFER_BIT )
            glClearColor(_v.m_rgba[0], _v.m_rgba[1], _v.m_rgba[2], _v.m_rgba[3]);
        if (mask & GL_DEPTH_BUFFER_BIT)
            glClearDepth(_v.m_depth);
        if( mask & GL_STENCIL_BUFFER_BIT )
            glClearStencil(_v.m_stencil);
    }

    bool HWRenderer::Clear()
    {
        m_stateStack    = std::stack<RenderState>();
        m_viewportStack = std::stack<HWViewport>();
        return true;
    }

    void HWRenderer::PushViewport(const HWViewport& _viewport)
    {
        auto& lastViewport = m_viewportStack.top();
        if (lastViewport.m_frameBuffer) {
            assert(lastViewport.m_frameBuffer->IsBound() == true);
            lastViewport.m_frameBuffer->UnBind();
        }
        
        m_viewportStack.push(_viewport);

        ApplyViewport(_viewport);   
        //aply clear mask
        if (_viewport.m_clearData.m_mask)
            Clear(_viewport.m_clearData);
    }

    void HWRenderer::PopViewport()
    {
        auto lastViewport = m_viewportStack.top(); 
        if (lastViewport.m_frameBuffer)
            lastViewport.m_frameBuffer->UnBind();
        m_viewportStack.pop();
        
        lastViewport = m_viewportStack.top();        
        ApplyViewport(lastViewport);
    }

    void HWRenderer::ApplyViewport(const HWViewport& _viewport)
    {
        
        glViewport( _viewport.m_pos.x, _viewport.m_pos.y, 
                    _viewport.m_size.x, _viewport.m_size.y );        
        if ( _viewport.m_frameBuffer ) {
             _viewport.m_frameBuffer->Bind();
        }
    }



    void HWRenderer::BindVertexArrayObject(uint32_t _id)
    {
        glBindVertexArray(_id);
    }

    void HWRenderer::BindIndexBuffer(uint32_t _id)
    {
        glBindBuffer(eVertexBufferTarget::VERTEX_BUFFER_INDEX, _id);
    }

    void HWRenderer::DrawArrays(eDrawMode _mode, uint32_t _index, uint32_t _count)
    {
        glDrawArrays((GLenum)_mode, _index, _count);
    }

    void HWRenderer::DrawIndexed(eDrawMode _mode, uint32_t _count)
    {
        glDrawElements((GLenum)_mode, _count, GL_UNSIGNED_INT, nullptr);
    }

   

    void HWRenderer::DrawQuad(float _left, float _right, float _bottom, float _top)
    {
              
        const Vector2f verts[4] = {
              Vector2f(_left,   _top),
              Vector2f(_left,   _bottom),
              Vector2f(_right,  _top),
              Vector2f(_right,  _bottom)
        };
        const auto& quadDrawable = m_drawables["ViewportQuad"];
        
        //update viewport data
        quadDrawable->m_vertexBuffers[0]->Bind();
        quadDrawable->m_vertexBuffers[0]->Update(verts, sizeof(Vector2f) * 4, 0);    
        quadDrawable->m_vertexBuffers[0]->UnBind();

        quadDrawable->m_vao->Bind();
       // quadDrawable->m_indexBuffer->Bind();

        DrawArrays(eDrawMode::DRAWMODE_TRIANGLE_STRIP, 0, 4);

        //quadDrawable->m_indexBuffer->UnBind();
        quadDrawable->m_vao->UnBind();        
    }

    void HWRenderer::DrawQuad(const Vector3f* _verts, const Vector2f* _uvs, uint32_t _vertCount, ShaderProgram* _pShader)
    {
        DrawTriangles(eDrawMode::DRAWMODE_TRIANGLE_STRIP, _verts, _vertCount, _pShader, nullptr, nullptr, _uvs);
    }

    void HWRenderer::DrawCube(HWTexture* _pCubeTex, ShaderProgram* _pShader)
    {
        const auto& curViewport = GetViewport();

        const auto left = curViewport.m_pos[0];
        const auto top = curViewport.m_pos[1];
        const auto right = left + curViewport.m_size[0];
        const auto bottom = top + curViewport.m_size[1];
        
        int texId = 0;
        _pShader->Bind();
        _pCubeTex->Bind(texId);
        _pShader->SetInt("tex_0", &texId);
        _pShader->SetMatrix4x4("proj", &m_cubemapMatrices.m_proj);
        for (int i = 0; i < 6; ++i) {
            _pShader->SetMatrix4x4("view", &m_cubemapMatrices.m_proj);
            DrawQuad(left, right, bottom, top);
        }
        _pCubeTex->UnBind();
        _pShader->UnBind();
    }

    void HWRenderer::DrawTriangles(eDrawMode _mode, const Vector3f* _pVerts, uint32_t _numVerts, ShaderProgram* _pShader,
        const Vector3f* _pNormals /*= nullptr*/, const Vector3f* _pTangents /*= nullptr*/, const Vector2f* _pUvs /*= nullptr*/)
    {
        assert(_pVerts);
        
        std::vector<HWBufferBase*> buffers;
        int bufIdx = 0;
        buffers.push_back(CreateVec3fBuffer(_pVerts, _numVerts, bufIdx++));
        if (_pNormals) 
            buffers.push_back(CreateVec3fBuffer(_pNormals, _numVerts, bufIdx++));     
        if (_pTangents)
            buffers.push_back(CreateVec3fBuffer(_pTangents,_numVerts, bufIdx++));        
        if (_pUvs) 
            buffers.push_back(CreateVec2fBuffer(_pUvs, _numVerts, bufIdx++));      

        auto vao = VertexArrayObject(buffers.data(), buffers.size() );

       


        bool unbind = false;
        if (!_pShader) {
            _pShader = m_pContext->GetResourceManager().GetShader("__DefaultDebugGeometryShader");
            _pShader->Bind();
            unbind = true;
        }
        vao.Bind();
        DrawArrays( _mode, 0, _numVerts );      
        vao.UnBind();
        if( unbind )
            _pShader->UnBind();

        for (auto buf : buffers)
            delete buf;
    }

    RenderState& HWRenderer::GetState()
    {
        return m_stateStack.top();
    }

    RenderState HWRenderer::GetInitialState() const
    {
        return m_initState;
    }

    bool HWRenderer::RendererValid() const
    {
        
        const auto errorCode = glGetError();
        if (errorCode != GL_NO_ERROR) {
            AddLogMessage(&GetContext(), fmt::format("Render Error Code: {}", errorCode), eLogLevel::LOG_LEVEL_ERROR);
            return false;
        }
        
        return true;
    }

    void HWRenderer::BeginRender()
    {
        BindVertexArrayObject(m_defaultVao);
    }
   
        
   

    void HWRenderer::EndRender()
    {
        BindVertexArrayObject(0);       //disable vao
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        assert(glGetError() == GL_NO_ERROR);
    }

    void HWRenderer::PushState( const RenderState& _newState, bool _skipFbo)
    {
        const auto& prevState = m_stateStack.top();
        StateSystem::StateDiff key;
        m_states.makeDiff( key, prevState, _newState );
        m_states.applyDiffGL(key, _newState, _skipFbo );
        m_stateStack.push(_newState);
    }

    void HWRenderer::PopState(bool _skipFbo)
    {
        const auto  curState  = m_stateStack.top(); m_stateStack.pop();
        const auto& prevState = m_stateStack.top();
        
        StateSystem::StateDiff key;
        m_states.makeDiff(key, curState, prevState);     
        m_states.applyDiffGL( key, prevState, _skipFbo );
    }

   
}

