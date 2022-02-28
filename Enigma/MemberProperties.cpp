#include "MemberProperties.h"

namespace RayTrace
{

	bool PropertyData::AddProperty(MemberProperty&& _prop)
	{
		if (m_properties.find(_prop.m_name) != std::end(m_properties))
			throw std::exception("Duplicate Property Name");		
				
		const auto propName = _prop.m_name;
		m_properties[propName] = m_propVector.size();
		m_propVector.push_back(std::move(_prop));
		return true;
	}

	PropertyData::MemberProperty* PropertyData::GetProperty(const std::string& _name)
	{
		if (m_properties.find(_name) == std::end(m_properties))
			throw std::exception("Property Not Found");
		const auto idx = m_properties.at(_name);
		return &m_propVector[idx];
	}

	std::vector<PropertyData::MemberProperty>& PropertyData::GetProperties() 
	{
		return m_propVector;
	}

}

