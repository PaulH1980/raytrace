#include "Context.h"
#include "WrappedEntity.h"
#include "SceneManager.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "Component.h"
#include "HWCamera.h"
#include "SceneView.h"
#include "CopyPaste.h"

namespace RayTrace
{

    EntityVector CloneEntities(const EntityVector& _pEntities)
    {
        EntityVector retVal;
        retVal.reserve(_pEntities.size());
        for (auto pEnt : _pEntities) {
            retVal.push_back(pEnt->Clone());
        }
        return retVal;
    }

    std::vector<WrappedEntityUPtr> CopyEntitiesUPtr(const EntityVector& _pEntities)
    {
        std::vector<WrappedEntityUPtr> retVal;
        retVal.reserve(_pEntities.size());
        for (auto pEnt : _pEntities) {
            retVal.push_back( WrappedEntityUPtr( pEnt->Clone() ) );
        }
        return retVal;
    }




    bool RemoveEntities(Context* _pContext, const EntityVector& _pEntities)
    {
        auto& compSystem = _pContext->GetComponentSystem();

        bool valid = true;
        for (auto pEnt : _pEntities)
        {
            valid &= compSystem.Remove(pEnt->GetEntity());
            delete pEnt;
        }
        return valid;
    }




    CopyPasteBuffer::CopyPasteBuffer(Context* _pContext )
        : m_pContext( _pContext  )       
    {       
    }

    CopyPasteBuffer::~CopyPasteBuffer()
    {

    }

    void CopyPasteBuffer::Clear()
    {
        
        auto valid = RemoveEntities(m_pContext, m_pEntities);
        assert(valid);
        
        m_pEntities.clear();
        m_proj = Identity4x4;
        m_view = Identity4x4;
    }

    void CopyPasteBuffer::SnapShot(const EntityVector& _pEntities)
    {
        if (GetCount()!=0)
            Clear();
        
        
        m_pEntities = CloneEntities( _pEntities );
        auto activeView = m_pContext->GetSceneManager().GetActiveSceneView();
        assert(activeView);
        m_proj = activeView->m_pCamera->GetProj();
        m_view = activeView->m_pCamera->GetWorldToView();
    }

    int CopyPasteBuffer::GetCount() const
    {
        return static_cast<int>(m_pEntities.size());
    }


    const EntityVector& CopyPasteBuffer::GetEntities() const
    {
        return m_pEntities;
    }

}

