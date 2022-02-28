#include "Context.h"
#include "HWVertexBuffer.h"
#include "Controller.h"
#include "HWCamera.h"
#include "SceneView.h"
#include "HWShader.h"
#include "ResourceManager.h"
#include "HWRenderer.h"
#include "HWStates.h"
#include "OverlayLayer.h"

namespace RayTrace
{
    
    OverlayLayer::OverlayLayer(Context* _pContext)
        : LayerBase( _pContext )
    {
        m_layerName = "Overlay";
        m_layer = eLayer::LAYER_OVERLAY;      

    }

    

    void OverlayLayer::Draw()
    {
        auto v  = CreateVec2fBuffer(QuadVertices, 4, 0);
        auto uv = CreateVec2fBuffer(QuadTexCoords, 4, 1);
        auto i  = CreateIndexBuffer(QuadIndices, 6);
        HWBufferBase* buffers[2]{
            v, uv
        };

        auto state = m_pContext->GetRenderer().GetInitialState();
        state.enable.field.depth_test = false;
        state.enable.field.cull_face  = false;
    
        m_pContext->GetRenderer().PushState(state);

        auto vao = VertexArrayObject(buffers, 2);
        auto shader = m_pContext->GetResourceManager().GetShader("__DefaultOverlayShader");
        vao.Bind();

        shader->Bind(); 
        i->Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        shader->UnBind();
        vao.UnBind();
        m_pContext->GetRenderer().PopState();

        delete v;
        delete uv;
        delete i;

    }



}

