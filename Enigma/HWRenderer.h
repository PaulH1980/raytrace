#pragma once

#include <stack>

#include "HWFormats.h" //glew

#include <SDL.h>
#include <SDL_opengl.h>
#include "HWStates.h"
#include "FrontEndDef.h"
#include "Transform.h"
#include "SystemBase.h"

#include "HWViewport.h"
#include "HWCamera.h"


namespace RayTrace
{
    extern Vector2f QuadVertices[4];
    extern Vector2f QuadTexCoords[4];
    extern uint32_t QuadIndices[6];

    HWFrameBuffer* EquirectangularToCube(Context* _pContext, const std::string& _fileName, int _dims);
    HWFrameBuffer* ConvolveCubemap(Context* _pContext, HWTexture* _cubeTex, int _dims);
    HWFrameBuffer* FilterEnvIbl(Context* _pContext, HWTexture* _cubeTex, int _dims = 128);
    HWFrameBuffer* CreateBrdfLut(Context* _pContext, int _dims);
    /*
        @brief: Create viewport aligned quad in worlds space starting at  top right, and in CCW order
    */
    bool           CreateViewportAllignedQuad(const HWCamera* _pCamera, const Vector3f& _center, const Vector2f& _scale, Vector3f* _pos, Vector2f* _uvs = nullptr);


    bool           CreateEnvironmentData(Context* _pContext, const std::string& _fileName, int _dims, int _irradianceDims, int _reflDims );
    bool           CreateEnvironmentDataFromResource(Context* _pContext, const std::string& _cubemapResource, int _irradianceDims, int _reflDims);



    class HWRenderer : public SystemBase
    {
        RT_OBJECT(HWRenderer, SystemBase)

    public:
        HWRenderer(Context* _context);
        virtual ~HWRenderer();

        bool                    Clear() override;

        bool                    PostConstructor() override;

        const HWViewport&       GetViewport() const;
      
        void                    PushViewport(const HWViewport& _viewport);
        void                    PopViewport();

        void                    DrawArrays(eDrawMode _mode, uint32_t _index, uint32_t _count);
        void                    DrawIndexed(eDrawMode _mode, uint32_t _count);
       // void                    EquirectangularTextureToCube(HWTexture* _equi, HWFrameBuffer* _fbo);

        void                    DrawQuad(const Vector3f* _verts, const Vector2f* _uvs, uint32_t _count, ShaderProgram* _pShader);
        void                    DrawQuad( float _left, float _right, float _bottom, float _top);
        void                    DrawCube(HWTexture* _pCubeTex, ShaderProgram* _pShader);
        void                    DrawTriangles(eDrawMode _mode,  const Vector3f* _pVerts, uint32_t _numVerts, ShaderProgram* _pShader = nullptr, const Vector3f* _pNormals = nullptr,
            const Vector3f* _pTangents = nullptr, const Vector2f* _pUvs = nullptr);

        StateSystem&            GetStateSystem() { return m_states; }
        const StateSystem&      GetStateSystem() const { return m_states; }
        RenderState&            GetState();
        RenderState             GetInitialState() const;

        bool                    RendererValid() const;
    
        void                    BeginRender();
        void                    EndRender();
        void                    PushState( const RenderState& _state, bool _skipFbo = false);
        void                    PopState(bool _skipFbo = false);
        void                    Clear(const FrameBufferClear& _v);
        void                    ApplyViewport(const HWViewport& _viewprt);

        void                    BindVertexArrayObject(uint32_t _id);
        void                    BindIndexBuffer(uint32_t _id);
      
    private:
       
       

        std::stack<RenderState>         m_stateStack;
        std::stack<HWViewport>          m_viewportStack;
        RenderState                     m_initState;   
        StateSystem                     m_states;
        uint32_t                        m_defaultVao = INVALID_ID;
        CubemapMatrices                 m_cubemapMatrices;

        
        std::map<std::string, std::unique_ptr<DrawableObject>> m_drawables;       
      
    };

   
}