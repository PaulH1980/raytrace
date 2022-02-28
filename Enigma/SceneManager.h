#pragma once
#include <unordered_map>
#include "SystemBase.h"
#include "Entity.h"

namespace RayTrace
{
    void        SelectObjects(const EntityVector& _objects, bool _selected);
    UUIDVector  GetUuids(  const EntityVector& _objects);
    std::vector<WrappedEntity*> FilterObjects(const EntityVector& _objects, EntityFilter _filter = nullptr);
    
    
    template<typename T>
    std::vector<T*> FilterObjectsTyped(const std::vector<WrappedEntity*>& _objects, EntityFilter _filter = nullptr) {
        static_assert(std::is_base_of<WrappedEntity, T>::value);
        const auto it = FilterObjects(_objects, _filter);
        std::vector<T*> ret;
        for (auto& curObject : it)
        {
            auto objPtr = dynamic_cast<T*>(curObject);
            if (objPtr)
                ret.push_back(objPtr);
        }
        return ret;
    }
    
    class SceneManager : public SystemBase
    {
        RT_OBJECT(SceneManager, SystemBase)
    public:


        //using ObjectUptr =  std::unique_ptr<ObjectBase>;
        

        SceneManager(Context* _pContext);

        bool                        PostConstructor();

        bool                        Clear() override;

        EntityVector                GetEntities(const UUIDVector& _names) const;
        WrappedEntity*              GetObject(const std::string& _name) const;

        bool                        AddObject(const std::string&_name, WrappedEntityUPtr _object);
        
        bool                        ContainsObject(const std::string& _name) const;
        
        WrappedEntityUPtr           RemoveObject(const std::string& _name);
    
        uint32_t                    GetObjectCount() const;        

        template<typename T>
        std::vector<T*>             GetObjectsOfType(EntityFilter _filter = nullptr) const;

        std::vector<WrappedEntity*> GetObjects(EntityFilter _filter = nullptr) const;

        void                        BeginModification();

        void                        EndModification();

        BBox3f                      GetSceneBounds() const;

        SceneView*                  GetActiveSceneView() const;
        void                        SetActiveSceneView(SceneView* _pView);

    private:
      
        using SceneObjectsMap = std::unordered_map<std::string, uint32_t>; //<objectName, vector idx>
       
        SceneObjectsMap                     m_objectMap;
        std::vector<WrappedEntityUPtr>      m_objectVector;       
        uint32_t                            m_modBeginSize = 0;
        uint32_t                            m_modEndSize   = 0;      
        SceneView*                          m_pActiveView = nullptr;
      
    };

    template<typename T>
    std::vector<T*> SceneManager::GetObjectsOfType(EntityFilter _filter ) const
    {
        static_assert(std::is_base_of<WrappedEntity, T>::value);  
        const auto it = GetObjects(_filter);
        return FilterObjectsTyped<T>(it, _filter);
    }

}