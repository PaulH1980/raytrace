#pragma once
#include <unordered_map>
#include "SystemBase.h"
#include "Components.h"
#include "Entity.h"

namespace RayTrace
{
    class ComponentManagerBase
    {
    public:
        ComponentManagerBase( Context* _pContext );
        virtual ~ComponentManagerBase() {};

        virtual bool Clone(Entity _srcEnt, Entity _dstEnt) = 0;
        virtual bool Remove(Entity _ent) = 0;
        virtual bool ContainsEntity(Entity _ent) const = 0;


        protected:
            Context* m_pContext = nullptr;
    };


    template<typename T>
    class TypedComponentManager : public ComponentManagerBase
    {
    public:
        TypedComponentManager(Context* _pContext, uint32_t _initialSize = 16384 ) 
            : ComponentManagerBase(_pContext){
			m_entities.reserve(_initialSize);
			m_components.reserve(_initialSize);
        }

        ~TypedComponentManager() {}

		
        bool ContainsEntity(Entity _ent) const override {
            return m_lookup.find(_ent) != std::end(m_lookup);
        }

        bool Clone(Entity _srcEnt, Entity _dstEnt) override {
            assert(ContainsEntity(_srcEnt));            
            if (!ContainsEntity(_srcEnt))
                return false;
            T cloned = *GetComponent(_srcEnt);

            m_lookup[_dstEnt] = m_components.size();
            m_components.push_back(cloned);
            m_entities.push_back(_dstEnt);

            return true;
        }

        template<typename... Targs>
        T* Create(Entity _ent, Targs... _args) {
            assert(ContainsEntity(_ent) == false);

            m_lookup[_ent] = m_components.size();
            m_components.push_back(T(_args...));
            m_entities.push_back(_ent);
            return &m_components.back();
        }

        uint32_t  GetCount() const {
            return m_components.size();
        }

        T* GetComponent(Entity _ent)         
        {
            if (!ContainsEntity(_ent))
                return nullptr;
            const auto idx = m_lookup.at(_ent);
            return &m_components.at(idx);
        }

        const T* GetComponent(Entity _ent) const
        {
            if (!ContainsEntity(_ent))
                return nullptr;
            const auto idx = m_lookup.at(_ent);
            return &m_components.at(idx);
        }

        T& operator[] (uint32_t _idx)
        {
            return m_components[_idx];
        }

        const T& operator[] (uint32_t _idx) const
        {
            return m_components[_idx];
        }

        Entity GetEntity(uint32_t _idx) const
        {
            return m_entities[_idx];
        }

        bool Remove(Entity _ent) override
        {
            if (!ContainsEntity(_ent))
                return false;
            const auto idx = m_lookup[_ent];
            if (idx < m_components.size() - 1)
            {
                m_components[idx] = std::move(m_components.back());
                m_entities[idx] = std::move(m_entities.back());

                const auto& entity = m_entities[idx];
                m_lookup[entity] = idx;
            }
            m_components.pop_back();
            m_entities.pop_back();
            m_lookup.erase(_ent);
            return true;
        }

        static const char* GetTypeName() {
            return typeid(T).name();
        }

        std::vector<T>                       m_components;
        std::vector<Entity>                  m_entities;
        std::unordered_map<Entity, uint32_t, Entity::hash_fn> m_lookup;
    };



    class ComponentSystem : public SystemBase
    {
        RT_OBJECT(ComponentSystem, SystemBase)

    public:       

        ComponentSystem(Context* _pContext);

        bool PostConstructor() override;

        bool Clear() override;

        bool Clone(Entity _srcEnt, Entity _dstEnt);

        bool Remove(Entity _ent);

        bool Register(const std::string& _name, std::unique_ptr<ComponentManagerBase> _comp) {
            if (Contains(_name)) {
                AddLogMessage(&GetContext(), fmt::format("Duplicate Component<{}>", _name), eLogLevel::LOG_LEVEL_ERROR);
                return false;
            }
            m_components[_name] = std::move(_comp);
            AddLogMessage(&GetContext(), fmt::format("Registered Component<{}>", _name));
            return true;
        }

       
        template<typename T>
        bool Register(uint32_t _initialSize = 4096 )
        {
            using NewComp = TypedComponentManager<T>;              
           
            bool IsNew = GetTypedManager<T>() == nullptr;
            if (IsNew)
            {
                std::string newCompName = NewComp::GetTypeName();
                auto newManager = std::make_unique<NewComp>(&GetContext(), _initialSize);
                return Register(newCompName, std::move(newManager));              
            }                 
            return false;
        }     

        template<typename T>
        T* GetComponent(Entity _ent) const{
            using NewComp = TypedComponentManager<T>;
            const std::string newCompName = NewComp::GetTypeName();          
            auto comp = GetTypedManager<T>();
            if (!comp)
                return nullptr;
            return comp->GetComponent(_ent);
        }

        template<typename T>
        T* CreateComponent(Entity _ent, bool _createNew = true) {
            using NewComp = TypedComponentManager<T>;
            const std::string newCompName = NewComp::GetTypeName();
            auto comp = GetTypedManager<T>();
            if ( !comp && _createNew )
            {
                 AddLogMessage(&GetContext(), fmt::format("Component: {} not found, registering...", newCompName));
                 Register<T>(); //register new component type
                 comp = GetTypedManager<T>();    
            }
            if (!comp)
                return nullptr;
            return comp->Create(_ent);
        }
    
        template<typename T>
        TypedComponentManager<T>* GetTypedManager() const{
            using NewComp = TypedComponentManager<T>;
            return static_cast<TypedComponentManager<T>*>(GetManager(NewComp::GetTypeName()));
        }

        template<typename T>
        bool            RemoveComponent(Entity _ent) {
            using NewComp = TypedComponentManager<T>;
            const std::string newCompName = NewComp::GetTypeName();
            auto comp = GetTypedManager<T>();
            if (!comp)
            {
                assert(false && "Component Manager Not Found");
                return false;
            }
            return comp->Remove(_ent);
        }

        ComponentManagerBase* GetManager(const std::string& _name) const;

        bool              Contains(const std::string& _name) const;

        template<typename... Targs>
        std::tuple<Targs* ...>   CreateComponents(Entity _ent, bool _createNew = true)
        {
           return std::make_tuple(this->CreateComponent<Targs>(_ent, _createNew ) ...);
        }

        template<typename... Targs>
        std::tuple<Targs* ...>             GetComponents(Entity _ent) const
        {
            return std::make_tuple(this->GetComponent<Targs>(_ent) ...);
        }       


    private:
        std::unordered_map<std::string, std::unique_ptr<ComponentManagerBase>> m_components;       
    };
}