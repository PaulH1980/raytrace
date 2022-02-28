#include "SceneView.h"
#include "Controller.h"
#include "Context.h"
#include "HWRenderer.h"
#include "Lights.h"
#include "HWPass.h"
#include "ResourceManager.h"
#include "HWVertexBuffer.h"

#include "LayerBase.h"

namespace RayTrace
{

    //////////////////////////////////////////////////////////////////////////
   //LayerBase Implementation
   //////////////////////////////////////////////////////////////////////////
    LayerBase::LayerBase(Context* _pContext)
        : ObjectBase( _pContext )
       
    {
    }

    void LayerBase::PreDraw(const HWViewport& _viewport, const HWCamera* _pCamera)
    {
        m_pActiveCam = _pCamera;   
        assert(m_activePasses.empty());
        for (auto& pass : m_layerPasses)
            if (pass->IsEnabled())
                m_activePasses.push_back(pass);   

        for (auto& pass : m_activePasses)
            pass->Begin(m_layer, _viewport, _pCamera ); //fetch items for this layer
    }

    void LayerBase::Draw()
    {
        for (auto& pass : m_activePasses)
        {
            pass->Process();
        }
    }


    void LayerBase::PostDraw()
    {
        for (auto& pass : m_activePasses)
            pass->End();

        m_activePasses.clear();
    }

    void LayerBase::Attach()
    {
        m_pContext->GetInputHandler().AddReceiver(this);
    }

    void LayerBase::Detach()
    {
        m_pContext->GetInputHandler().RemoveReceiver(this);
    }
   

    PerLayer LayerBase::GetPerLayerData(const HWCamera* _pCamera) const
    {
       
        Matrix4x4 identity = GetIdentity();
        PerLayer layerData = {
            _pCamera->GetProj(),
            _pCamera->GetWorldToView(),
            _pCamera->GetSkyView(),
            _pCamera->GetProjInv(),
            _pCamera->GetWorldToViewInv(),
            _pCamera->GetOrtho2D(),
            identity,
            Vector4f(_pCamera->GetPosition(), 1.0f),
            Vector4f(_pCamera->GetRight(), 1.0f),
            Vector4f(_pCamera->GetUp(), 1.0f ),
            Vector4f(_pCamera->GetForward(), 1.0f ),
            Vector4f(_pCamera->GetWidth(), _pCamera->GetHeight(), 1.0f / _pCamera->GetWidth(), 1.0 / _pCamera->GetHeight()),
            Vector4f(_pCamera->GetNear(), _pCamera->GetFar(), 0, 0),
            Vector4f(m_pContext->GetElapsedTime(), m_pContext->GetFrameTime(), (float)m_pContext->GetFrameNumber(), 0.0f),
            Vector4f(Normalize(Vector3f(1, -1, 1)), 0.0f),
            Vector4f(1.0f, 1.0f, 1.0f, 100.0f),
            Vector4f(1.0f, 0.5f, 0.0f, 1.0f)
        };
        return layerData;
    }
   
    void LayerBase::SetActive(bool _v)
    {
        m_active = _v;
    }

    bool LayerBase::IsActive() const
    {
        return m_active;
    }

    void LayerBase::AddPass(HWPass* _pPass)
    {
        m_layerPasses.push_back(_pPass);
    }

    void LayerBase::SetWindowId(uint32_t _windowId)
    {
        m_windowId = _windowId;
    }

    uint32_t LayerBase::GetWindowId() const
    {
        return m_windowId;
    }

    HWRenderer& LayerBase::GetRenderer()
    {
        return m_pContext->GetRenderer();
    }


    const std::string& LayerBase::GetLayerName() const
    {
        return m_layerName;
    }


    eLayer LayerBase::GetLayerType() const
    {
        return m_layer;
    }
}

