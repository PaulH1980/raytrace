#pragma once

#include "FrontEndDef.h"
#include "Transform.h"

namespace RayTrace
{

    template<typename T>
    T Clamp(const T& _value, const T& _a, const T& _b)
    {
        return _value;
    }

   
    template<typename T>
    class Properties
    {
    public:
       
        Properties( T* _val = nullptr, ObjectBase* _wrappedType = nullptr, 
            bool _isRanged = false, const T& _min = T(), const T& _max = T() )    
            : m_wrappedValue( _val )
            , m_isRanged(_isRanged)
            , m_wrappedType(_wrappedType)              
            , m_min(_min)
            , m_max(_max)          
        {
            //assert(_val != nullptr && "NullPtr");
			if (_val != nullptr) {
				m_initialValue = (*_val);
				m_editingValue = (*_val);
			}
        }

        void SetPropertyGroup(const std::string _propGroup) {
            m_propGroup = _propGroup;
        }

        void SetPropertyName(const std::string _propName)
        {
            m_propName = _propName;
        }


        void SetValue(const T& _v) 
        {
            const auto tmp = IsRanged() ? Clamp(_v, m_min, m_max) : _v;
            if (tmp != *m_wrappedValue)
            {
                *m_wrappedValue = tmp;
                m_editingValue = tmp;
                if (m_wrappedType) {
                    m_wrappedType->ObjectModified();
                }
            }            
        }


        T       GetValue() const
        {
            return *m_wrappedValue;
        }

        ObjectBase* GetWrappedObject() const {
            return m_wrappedType;
        }

        Context& GetContext() const { return m_wrappedType->GetContext(); }


        bool IsRanged() const {
            return m_isRanged;
        }

        std::string     m_propGroup;
        std::string     m_propName;
        bool            m_isRanged     = false;
        ObjectBase*     m_wrappedType  = nullptr;        
        T*              m_wrappedValue = nullptr;
        T               m_initialValue = T{}; //undo redo
        T               m_editingValue = T{}; //actively changing value
        T               m_min = {},
                        m_max = {};
    };
}