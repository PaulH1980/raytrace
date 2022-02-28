#include "Context.h"
#include "SceneManager.h"
#include "HWVertexBuffer.h"
#include "LayerBase.h"
#include "SceneItemCollector.h"
#include "ResourceManager.h"
#include "WrappedEntity.h"
#include "HWShader.h"
#include "HWMesh.h"
#include "HWRenderer.h"
#include "SceneView.h"
#include "HWRenderer.h"
#include "HWMaterial.h"
#include "HWFrameBuffer.h"
#include "HWTexture.h"
#include "HWLight.h"
#include "Common.h"
#include "Geometry.h"
#include "HWPass.h"



namespace RayTrace
{

   

    HWPass::HWPass(Context* _pContext, const std::string& _name, SceneItemCollectorUPtr _pCollector /*= nullptr*/, RenderState* _state /*= nullptr*/)
        : m_pContext(_pContext)
        , m_name( _name )
        , m_collector(std::move(_pCollector))
        , m_activeState( _state )
        , m_renderer( _pContext->GetRenderer() )
    {
     // m_pContext->GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    }

    void HWPass::Begin(eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera )
    {
        m_activeView   = _view;
        m_pActiveCamera = _pCamera;
        
        assert(m_passSceneItems.empty());      
        if (m_clearFrameBuffer) {
        }       
     
        if (m_collector) {
            m_passSceneItems = m_collector->GetItems( &m_pContext->GetSceneManager() );
        }
    }

    void HWPass::Process()
    {
        if (m_activeState)
            m_renderer.PushState(*m_activeState);


        for (auto& item : m_passSceneItems)
        {
            if (item.m_pEntity && item.m_material && item.m_activeShader)
            {
                if (item.m_pProcessEntityFun) {                 
                    item.m_activeShader->Bind();
                    item.m_material->Bind(item.m_activeShader);
                    item.m_pProcessEntityFun( &m_renderer, item.m_pEntity, item.m_material, item.m_activeShader);
                    item.m_material->UnBind();
                    item.m_activeShader->UnBind();
                }
            }
        }

        if (m_activeState)
            m_renderer.PopState();

        {
         //   GeometryBase baseGeo(m_pContext);
        //    ModifierStack modStack(m_pContext);
        //    auto* newMode = new PlaneModifier(&modStack);
        //    modStack.AddModifier(std::unique_ptr<ModifierBase>(newMode));
        //    modStack.Apply();

        //    auto& renderer = m_pContext->GetRenderer();
        //    auto activeState = renderer.GetInitialState();
        //    activeState.enable.field.cull_face = !true;
        //    renderer.PushState(activeState);
        //    auto geom = modStack.GetGeometry();

        //   // auto texPtr = m_pContext->GetResourceManager().GetFrameBuffer("testbuf")->GetTextures()[0].get();
        //    auto shader = m_pContext->GetResourceManager().GetShader("__DefaultDebugGeometryShader");
        //    int texUnit = 0;
        //  //  texPtr->Bind(texUnit);
        //    shader->Bind();
        //  //  shader->SetInt("albedo", &texUnit);

        //    renderer.DrawTriangles( eDrawMode::DRAWMODE_TRIANGLES, geom->m_verts.data(), geom->m_verts.size(), shader,
        //        geom->m_normals.data(), nullptr, geom->m_uvs.data());

        //    shader->UnBind();
        ////    texPtr->UnBind();
        //    renderer.PopState();
        }


    }

    void HWPass::End()
    {
        m_passSceneItems.clear();       
    }

    void HWPass::SetEnabled(bool _enabled)
    {
        m_enabled = _enabled;
    }

    bool HWPass::IsEnabled() const
    {
        return m_enabled;
    }

    void HWPass::SetClearFrameBuffer(bool _clear)
    {
        m_clearFrameBuffer = _clear;
    }

    void HWPass::SetShader(ShaderProgram* _pActiveShader)
    {
        m_activeShader = _pActiveShader;
    }

    void HWPass::SetMaterial(HWMaterial* _pMaterial)
    {
        m_activeMaterial = _pMaterial;
    }

    struct  DepthPrePass::DataImpl
    {
        DataImpl() {
            Vector2f uvs[4]
            {
                {1.f, 1.f}, //top right
                {0.f, 1.f}, //top left
                {0.f, 0.f}, //bottom left
                {1.f, 0.f}, //bottom right            
            };

            uint32_t indices[6]{
                0, 1, 2,
                2, 3, 0
            };

           m_verts      = std::unique_ptr<Vec2fBuffer>(CreateVec2fBuffer(nullptr, 4, 1, eVertexBufferUsage::VERTEX_BUFFER_USAGE_DYNAMIC_DRAW));
           m_uvs        = std::unique_ptr<Vec2fBuffer>(CreateVec2fBuffer(uvs, 4, 1));
           m_indices    = std::unique_ptr <IndexBuffer> (CreateIndexBuffer(indices, 6));

           HWBufferBase* buffers[2] ={
               m_verts.get(), m_uvs.get() };

           m_vao = std::make_unique<VertexArrayObject>(buffers, 2);           

        }

        ~DataImpl() {

        }

        std::unique_ptr<Vec2fBuffer>    m_verts,
                                        m_uvs;
        std::unique_ptr <IndexBuffer>   m_indices;
        std::unique_ptr<VertexArrayObject> m_vao;
    };



   //////////////////////////////////////////////////////////////////////////
   //DepthPrePass Implementation
   //////////////////////////////////////////////////////////////////////////
   DepthPrePass::DepthPrePass(Context* _pContext,const std::string& _name, SceneItemCollectorUPtr _pCollector,  RenderState* _state)
        : HWPass( _pContext, _name, std::move(_pCollector), _state)
        , m_pImpl( std::make_unique<DataImpl>())
        , m_width( 0 )
        , m_height(0)
    {
       m_depthFrameBuffer = m_pContext->GetResourceManager().GetFrameBuffer("__DefaultDepthOnlyFrameBuffer");
      // assert(m_activeShader);
    }

    void DepthPrePass::Begin(eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera)
    {
        
        const int left   = (int)0.0f;
        const int top    = (int)0.0f;
        const int right  = (int)_view.m_size.x;
        const int bottom = (int)_view.m_size.y;
        //update quad
        if (m_width != right || m_height != bottom) {
            m_width = right;
            m_height = bottom;

            const Vector2f verts[4] = {
                Vector2f(right, top),
                Vector2f(left, top),
                Vector2f(left,bottom),
                Vector2f(right, bottom)
            };

            m_pImpl->m_verts->Bind();
            m_pImpl->m_verts->UpdateTyped(verts, 4);
            m_pImpl->m_verts->UnBind();
        }
        //change framebuffer
        auto viewport = _view;
        viewport.SetFrameBuffer(m_depthFrameBuffer);
        // fetch renderable items
        HWPass::Begin(_layer, viewport, _pCamera );
    }

    void DepthPrePass::Process()
    {
      
        if (m_activeState)
            m_renderer.PushState(*m_activeState);
        assert(m_activeView.m_frameBuffer == m_pContext->GetResourceManager().GetFrameBuffer("__DefaultDepthOnlyFrameBuffer"));
               
        m_renderer.PushViewport(m_activeView); //push current fbo
        m_activeView.Resize(Vector2i(m_width, m_height));

        BinItems();
        DrawOpaque();
        DrawMasked();

         m_renderer.PopViewport(); //back to default fbo

        if (m_activeState)
           m_renderer.PopState();      
#if 1
        //Draw quad with depth values coverign the viewport    
        const auto& depthTex = m_activeView.GetFrameBuffer()->GetDepthTexture();
        auto writeDepthShader = m_pContext->GetResourceManager().GetShader("__DefaultWriteDepthShader");


        auto state = m_pContext->GetRenderer().GetInitialState();
        state.enable.field.depth_test = false;
        state.enable.field.cull_face    = false;

        m_pContext->GetRenderer().PushState(state);     
        m_pImpl->m_vao->Bind();
        m_pImpl->m_indices->Bind();
        writeDepthShader->Bind();
        int texUnit = 0;
        depthTex->Bind(texUnit);
        writeDepthShader->SetInt("depthTex", &texUnit);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        depthTex->UnBind();
        writeDepthShader->UnBind();
        m_pImpl->m_indices->UnBind();
        m_pImpl->m_vao->UnBind();
    
        m_pContext->GetRenderer().PopState();
#endif
    }

    void DepthPrePass::End()
    {
        HWPass::End();
        m_opaqueItems.clear();
        m_maskedItems.clear();       
        m_activeShader = nullptr;
    }
       

    void DepthPrePass::DrawOpaque()
    {
      
        auto writeDepthShader = m_pContext->GetResourceManager().GetShader("__DefaultDepthOnlyShader");
        auto nullMaterial     = m_pContext->GetResourceManager().GetMaterial("__DefaultNullMaterial");
        
        writeDepthShader->Bind();
        for (const auto& item : m_opaqueItems)
        {
            if (item.m_pProcessEntityFun)
               item.m_pProcessEntityFun(&m_renderer, item.m_pEntity, nullMaterial, writeDepthShader);            
        }
        writeDepthShader->UnBind();
    }

    void DepthPrePass::DrawMasked()
    {
         
        auto writeDepthShader = m_pContext->GetResourceManager().GetShader("__DefaultDepthOnlyWithTexMaskShader");
      
        writeDepthShader->Bind();
        for (auto& item : m_maskedItems)
        {
            if (item.m_pProcessEntityFun)
            {
                auto texUnit = 0;
                item.m_material->m_textures[ALBEDO]->Bind(texUnit);
                writeDepthShader->SetInt("texMask", &texUnit);

                item.m_pProcessEntityFun(&m_renderer, item.m_pEntity, item.m_material, writeDepthShader);

                item.m_material->m_textures[ALBEDO]->UnBind();
            }
               
        }
        writeDepthShader->UnBind();
    }

    void DepthPrePass::BinItems()
    {
        for (auto& curItem : m_passSceneItems)
        {
            bool isOpaque = true;
            if (curItem.m_material)
            {
                if (curItem.m_material->HasAlphaMask())
                    isOpaque = false;
            }
            if (isOpaque)
                m_opaqueItems.push_back(curItem);
            else
                m_maskedItems.push_back(curItem);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //EditorPass
    //////////////////////////////////////////////////////////////////////////
    EditorPass::EditorPass(
        Context* _pContext, const std::string& _name, SceneItemCollectorUPtr _pCollector, RenderState* _state)
        : HWPass(_pContext, _name, std::move(_pCollector), _state)
    {
        m_activeShader          = m_pContext->GetResourceManager().GetShader("__DefaultWriteObjectIdsShader");    
        m_objectIdsFrameBuffer  = m_pContext->GetResourceManager().GetFrameBuffer("__DefaultPickingFrameBuffer");
        assert(m_activeShader && m_objectIdsFrameBuffer);
    }

    void EditorPass::Begin(eLayer _layer, const HWViewport& _view, const HWCamera* _pCamera)
    {
       
        auto viewport = _view;
        //change framebuffer & resize
        viewport.SetFrameBuffer(m_objectIdsFrameBuffer);    
        viewport.Resize(max(_view.m_size / m_downScaleRatio, Vector2i(2, 2)));
        viewport.m_clearData.m_rgba = Vector4f(1.f, 1.f, 1.f, 1.f);
        HWPass::Begin(_layer, viewport, _pCamera );
        BinItems();

    }

    void EditorPass::Process()
    {
        auto nullMat = m_pContext->GetResourceManager().GetMaterial("__DefaultNullMaterial");
       
        if (m_activeState)
            m_renderer.PushState(*m_activeState);
        m_renderer.PushViewport(m_activeView); 
        DrawToObjectIdBuffer();    
        CopyPixelsFromGpu();

        m_renderer.PopViewport();

        if (m_activeState)
            m_renderer.PopState();
      
       // DrawGrid();
        DrawEditorSprites();//#todo assign proper render state etc draw selected objects
        DrawSelectedMeshes();
    }

    void EditorPass::End()
    {
        HWPass::End();
        Clear();
    }

    const uint32_t* EditorPass::GetObjectIdPixels() const
    {
        return m_objIdPixels.data();
    }

    const float* EditorPass::GetDepthPixels() const
    {
        return m_depthPixels.data();
    }

    int EditorPass::GetViewportDownScale() const
    {
       return m_downScaleRatio;
    }

    void EditorPass::BinItems()
    {
        for (const auto& item : m_passSceneItems)
        {
             auto* pEnt = item.m_pEntity;
            
             if (pEnt->HasComponent<MeshComponent>() || pEnt->HasComponent<IndexedMeshComponent>() )
                 m_meshes.push_back(item);    
             else if (auto spriteComp = pEnt->GetComponent<EditorSpriteComponent>()) {

                 Vector3f worldPos  = pEnt->GetPosition();
                 Vector3f iconColor = pEnt->GetIconColor();                
                 
                 spriteComp->m_spriteColor = iconColor;

                 if (!CreateViewportAllignedQuad(m_pActiveCamera, worldPos,
                     spriteComp->m_scale, spriteComp->m_coords, spriteComp->m_uvs))
                     continue;
                 m_sprites.push_back(item);
             }
        }
    }

    void EditorPass::DrawToObjectIdBuffer()
    {
        auto nullMat = m_pContext->GetResourceManager().GetMaterial("__DefaultNullMaterial");
        auto shader  = m_pContext->GetResourceManager().GetShader("__DefaultWriteObjectIdsShader");
      
        shader->Bind();
        for (const auto& mesh : m_meshes) {
            auto objId = mesh.m_pEntity->GetObjectId();
            shader->SetUint("objectId", &objId);
            mesh.m_pProcessEntityFun(&m_renderer, mesh.m_pEntity, nullMat, shader);
        }

        shader->SetMatrix4x4("model", &Identity4x4);
        for (const auto& sprite : m_sprites)
        {
            auto objId      = sprite.m_pEntity->GetObjectId();
            auto spriteComp = sprite.m_pEntity->GetComponent<EditorSpriteComponent>();
            shader->SetUint("objectId", &objId);
            m_renderer.DrawQuad(spriteComp->m_coords, spriteComp->m_uvs, 4, shader);
        }
        shader->UnBind();

    }

    void EditorPass::CopyPixelsFromGpu()
    {
        auto hwTex    = m_activeView.GetFrameBuffer()->GetTexture(0);
        auto depthTex = m_activeView.GetFrameBuffer()->GetDepthTexture().get();
        const auto numPixels = m_activeView.m_size.x * m_activeView.m_size.y;
        m_objIdPixels.resize(numPixels);
        m_depthPixels.resize(numPixels);

        memcpy(m_objIdPixels.data(), GetTextureData(hwTex).data(), numPixels * sizeof(uint32_t));
        memcpy(m_depthPixels.data(), GetTextureData(depthTex).data(), numPixels * sizeof(float));
    }

    void EditorPass::DrawGrid()
    {
        auto shader  = m_pContext->GetResourceManager().GetShader("__DefaultGridShader");
        auto nullMat = m_pContext->GetResourceManager().GetMaterial("__DefaultNullMaterial");
        auto gridState = m_pContext->GetResourceManager().GetRenderState("__EditorGridState");
        m_renderer.PushState(*gridState);
        shader->Bind();
        m_renderer.DrawQuad(0, m_activeView.m_size.x, m_activeView.m_size.y, 0);
        shader->UnBind();
        m_renderer.PopState();
    }

    void EditorPass::DrawEditorSprites()
    {
        auto shader = m_pContext->GetResourceManager().GetShader("__DefaultIconShader");
        shader->Bind();
        for (const auto& sprite : m_sprites )
        {
            auto comp = sprite.m_pEntity->GetComponent<EditorSpriteComponent>();
            auto spriteTex = comp->m_texId.m_pTexture;
            if (spriteTex) {
                int texUnit = 0;
                spriteTex->Bind(0);
                shader->SetInt("tex", &texUnit);
            }

            const int isSelected = sprite.m_pEntity->GetObjectFlags().m_field.m_selected ? 1 : 0;
            const Vector3f color = isSelected ? Vector3f(1.0f, 0.5f, 0.25f) : comp->m_spriteColor.m_rgb;

            shader->SetInt( "isSelected", &isSelected);           
            shader->SetVec3f("iconColor", &comp->m_spriteColor.m_rgb);

            m_renderer.DrawTriangles(eDrawMode::DRAWMODE_TRIANGLE_STRIP, comp->m_coords, 4, 
                shader, nullptr, nullptr, comp->m_uvs);

            if (spriteTex)
                spriteTex->UnBind();           
        }
        shader->UnBind();
    };
   

    void EditorPass::DrawSelectedMeshes()
    {
        auto nullMat = m_pContext->GetResourceManager().GetMaterial("__DefaultNullMaterial");
        auto shader = m_pContext->GetResourceManager().GetShader("__DefaultSelectionShader");
        auto blendState = m_pContext->GetResourceManager().GetRenderState("__BlendState");
        m_renderer.PushState(*blendState);      
        int isSelected = 1;
        shader->Bind();
        shader->SetInt("isSelected", &isSelected);
        for (auto mesh : m_meshes)
        {
            if( !mesh.m_pEntity->GetObjectFlags().m_field.m_selected )
                continue;            
            mesh.m_pProcessEntityFun(&m_renderer, mesh.m_pEntity, nullMat, shader);
        }
        shader->UnBind();
        m_renderer.PopState();
    }

   

    void EditorPass::Clear()
    {
        m_meshes.clear();
        m_sprites.clear();
    }

}