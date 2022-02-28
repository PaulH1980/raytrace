#include "Context.h"
#include "Logger.h"
#include "Properties.h"
#include "LayerManager.h"
#define UUID_SYSTEM_GENERATOR
#include "UUID.h"
#include "ObjectBase.h"

namespace RayTrace
{

    void AddLogMessage(Context* _pContext, const std::string& _msg, eLogLevel _level)
    {
        _pContext->GetLogger().AddMessage(_msg, _level);
    }


    ObjectBase::ObjectBase(Context* _pContext)
        : m_pContext(_pContext)
        , m_objectUUID(uuids::to_string(uuids::uuid_system_generator{}()))     
        , m_objectId( _pContext->GetNextObjectId() )
    {       
        m_flags.m_field.m_visible = 1;       
    }


	ObjectBase::ObjectBase(const ObjectBase& _rhs)
        : m_pContext( _rhs.m_pContext )
		, m_objectUUID(uuids::to_string(uuids::uuid_system_generator{}()))
		, m_objectId(_rhs.m_pContext->GetNextObjectId())
        , m_flags( _rhs.m_flags )
        , m_groupedName( _rhs.m_groupedName )
        , m_layerFlags( _rhs.m_layerFlags )
        , m_name(_rhs.m_name + "_cloned" )
	{
        //dont copy properties..
	}

	ObjectBase* ObjectBase::Clone() const
    {
        return nullptr;
    }

    Context& ObjectBase::GetContext() const
    {
        return *m_pContext;
    }

    const std::string& ObjectBase::GetObjectName() const
    {
        return m_name;
    }

    void ObjectBase::SetObjectName(const std::string& _name)
    {
        m_name = _name;
    }

    const std::string& ObjectBase::GetGroupName() const
    {
        return m_groupedName;
    }

    void ObjectBase::SetGroupName(const std::string& _name)
    {
        m_groupedName = _name;
    }

    const std::string& ObjectBase::GetUUID() const
    {
        return m_objectUUID;
    }

    ObjectFlags& ObjectBase::GetObjectFlags()
    {
        return m_flags;
    }

    const ObjectFlags& ObjectBase::GetObjectFlags() const
    {
        return m_flags;
    }

    void ObjectBase::SetObjectFlags(const ObjectFlags& _flags)
    {
        m_flags = _flags;
    }

    uint32_t ObjectBase::GetLayerFlags() const
    {
        return m_layerFlags;
    }

    void ObjectBase::SetLayerFlags(uint32_t flags)
    {
        m_layerFlags = flags;
    }

    bool ObjectBase::ContainsProperty(const std::string& _name) const
    {
        return m_properties.find(_name) != std::end(m_properties);
    }

    PropertyData* ObjectBase::GetPropertyData(const std::string& _name) const
    {
      if ( !ContainsProperty(_name)) 
        {
            assert(false);
            return{};
        }
        return m_properties.at(_name).get();
    }


    bool ObjectBase::AddPropertyData( std::unique_ptr<PropertyData> _propData)
    {
        const auto name = _propData->m_name;
        
        if (ContainsProperty(name)) {
            assert(false);
            return false;
        }
        m_properties[name] = std::move(_propData);
        return true;
    }

    void ObjectBase::Update(float _dt)
    {
        //NOP
    }

	void ObjectBase::Initialize()
	{
        ClearProperties();
        GetProperties();
	}

	void ObjectBase::ClearProperties()
	{
        m_properties.clear();
	}

	ObjPropVector ObjectBase::GetProperties()
    {
        if ( !ContainsProperty( GetTypeNameStatic() ) )
        {
            auto pPropertery = new PropertyData();
            pPropertery->m_name = GetTypeNameStatic();

            PropertyData::MemberProperty objFlags = { Properties<uint32_t>(&m_flags.m_flags, this), "Object Flags", PropertyData::ePropertyType::PROPERTY_TYPE_MASK };
            objFlags.m_propFlags.push_back({ "Visible",    0x01, });
            objFlags.m_propFlags.push_back({ "Selected",   0x02 });
            objFlags.m_propFlags.push_back({ "Persistent", 0x04 });
            objFlags.m_propFlags.push_back({ "Editor",     0x08 });

            pPropertery->AddProperty( std::move(objFlags));
            pPropertery->AddProperty({ Properties<std::string>(&m_name, this), "Name" });
            pPropertery->AddProperty({ Properties<std::string>(&m_groupedName, this), "Group" });

            m_properties[GetTypeNameStatic()] = std::unique_ptr<PropertyData>(pPropertery);
        }

        ObjPropVector retVal;
        retVal.push_back(m_properties[GetTypeNameStatic()].get());
        return retVal;        
    }

    void ObjectBase::ObjectModified()
    {
        m_flags.m_field.m_modified = true;
    }

}

