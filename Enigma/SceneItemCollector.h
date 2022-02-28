#pragma once
#include "FrontEndDef.h"
#include "SceneItem.h"

namespace RayTrace
{
    
    
    class SceneItemCollector
    {
    public:
        virtual ~SceneItemCollector();
        virtual std::vector<SceneItem> GetItems( SceneManager* _pScene ) const = 0;
    };

   


    class FilteredSceneItemCollector : public SceneItemCollector 
    {
    public:
        FilteredSceneItemCollector(EntityFilter _pFilter = nullptr, bool _fetchDrawFunc = true);
        std::vector<SceneItem> GetItems(SceneManager* _pScene) const override;



    private:

        EntityFilter m_pEntFilter = nullptr;
        bool         m_fetchDrawFunc = true;      
    };
}