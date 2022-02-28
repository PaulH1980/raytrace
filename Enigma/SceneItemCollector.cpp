#include "SceneManager.h"
#include "WrappedEntity.h"
#include "HWMesh.h"
#include "HWLight.h"
#include "SceneItemCollector.h"

namespace RayTrace
{
    SceneItemCollector::~SceneItemCollector()
    {

    } 
   

    FilteredSceneItemCollector::FilteredSceneItemCollector(EntityFilter _pFilter /*= nullptr*/, bool _fetchDrawFunc /*= true*/) : m_pEntFilter(_pFilter)
        , m_fetchDrawFunc(_fetchDrawFunc)
    {

    }

    std::vector<SceneItem> FilteredSceneItemCollector::GetItems(SceneManager* _pScene) const
    {
        auto* pRenderer = &_pScene->GetContext().GetRenderer();
        const auto entities = _pScene->GetObjects(m_pEntFilter);

        std::vector<SceneItem> retVal;
        retVal.reserve(entities.size());
        for (auto ent : entities) {
            SceneItem item = { ent };
            if (m_fetchDrawFunc) { 
                item.m_pProcessEntityFun = ent->GetProcessFunction();
                if (item.m_pProcessEntityFun) { //also fetch shader & material
                    item.m_activeShader = ent->GetShader();
                    item.m_material = ent->GetMaterial();
                }
                
            }
            retVal.push_back(item);            
        }
        return retVal;
    }

   

}

