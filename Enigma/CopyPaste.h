#pragma once
#include "FrontEndDef.h"

namespace RayTrace
{
    EntityVector                    CloneEntities(const EntityVector& _pEntities);
    std::vector<WrappedEntityUPtr>  CopyEntitiesUPtr(const EntityVector& _pEntities);
    bool                            RemoveEntities( Context* _pContext, const EntityVector& _pEntities);
    
    
    class CopyPasteBuffer
    {
    public:
        CopyPasteBuffer(Context* _pContext);
        ~CopyPasteBuffer();

        void         Clear();
        /*
            @brief: Clone entities, so they keep their parameters when the snapshot was taken
            if buffer contains any previous those will be cleared/erased
        */
        void         SnapShot(const EntityVector& _pEntities);
        int          GetCount() const;


        const EntityVector& GetEntities() const;

        Context*     m_pContext = nullptr;
        EntityVector m_pEntities;

        Matrix4x4    m_proj,
                     m_view;
    };
}