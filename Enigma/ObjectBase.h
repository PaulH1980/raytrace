#pragma once

#include "FrontEndDef.h"
#include "NonAssignable.h"
#include "MemberProperties.h"

namespace RayTrace
{
    void AddLogMessage(Context* _pContext, const std::string& _msg, eLogLevel _level = eLogLevel::LOG_LEVEL_INFO);


#define RT_OBJECT(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        virtual std::string        GetTypeName() const  { return #typeName; } \
        static const std::string&  GetTypeNameStatic() { static const std::string name = #typeName; return name;} \
        virtual std::string        GetBaseName() const  { return #baseTypeName; } \
        static const std::string&  GetBaseNameStatic() { static const std::string name = #baseTypeName; return name; } \

#define RT_COMPONENT(typeName) \
    public: \
    static const std::string&  GetTypeNameStatic() { static const std::string name = #typeName; return name;} \



    struct ObjectFlags
    {
        union {
            uint32_t m_flags = 0;
            struct BitField
            {
                uint32_t m_visible          : 1; //0001  //draw in viewport
                uint32_t m_selected         : 1; //0002  //selected              
                uint32_t m_persistent       : 1; //0004 //don't clear this object after scene clear
                uint32_t m_visibleEditor    : 1; //0008
                uint32_t m_markedForDelete  : 1; //0010 //internal
                uint32_t m_modified         : 1; //0020  //internal state modified
                uint32_t m_activated        : 1; //0040
               
            } m_field;
        };
    };
 

    /*
        @brief: Base class for all engine objects
                No Copying/assignment allowed, use Clone instead
    */
    class ObjectBase : public NonAssignable
    {
        RT_OBJECT( ObjectBase, NonAssignable )
    public:
        ObjectBase(Context* _pContext);
        virtual ~ObjectBase() {};

        virtual ObjectBase* Clone() const;
        virtual void        Update(float _dt);
        virtual void        Initialize();

        void                  ClearProperties();
        virtual ObjPropVector GetProperties();
        virtual void          ObjectModified();
        PropertyData*         GetPropertyData(const std::string& _name) const;
        
        template<typename T>
        Properties<T>*        GetProperty(const std::string& _propGroup, const std::string& _propName) const;

      
        Context&            GetContext() const;

        const std::string&  GetObjectName() const;
        void                SetObjectName(const std::string& _name);

        const std::string&  GetGroupName() const;
        void                SetGroupName(const std::string& _name);

        const std::string&  GetUUID() const;
  

        const ObjectFlags&  GetObjectFlags() const;
        ObjectFlags&        GetObjectFlags();
        void                SetObjectFlags(const ObjectFlags& _flags);


        uint32_t            GetLayerFlags() const;
        void                SetLayerFlags(uint32_t flags);

        uint32_t            GetObjectId() const{ return m_objectId;}

        bool                ContainsProperty(const std::string& _name) const;
        bool                AddPropertyData( std::unique_ptr<PropertyData> _propData);
        

    protected:

        ObjectBase(const ObjectBase& _rhs);

        std::string m_name;
        std::string m_groupedName;
        std::string m_objectUUID; //UID
        uint32_t    m_layerFlags = LAYER_ALL; //TODO
        ObjectFlags m_flags;
        Context*    m_pContext = nullptr;
        uint32_t    m_objectId = 0;

        
        std::map<std::string, std::unique_ptr<PropertyData>> m_properties;

    private:
      
   
    };

	template<typename T>
    Properties<T>* ObjectBase::GetProperty(const std::string& _propGroup, const std::string& _propName) const
	{
        if (!ContainsProperty(_propGroup))
        {
            assert(false && "Property Group Not Found");
            return nullptr;
        }

        auto propGroup = GetPropertyData(_propGroup);
        auto propItem  = propGroup->GetProperty(_propName );
       
        return std::get_if <Properties<T>>(&propItem->m_prop);
       
	}

}