#pragma once
#include <stdint.h>
#include <cstddef>
#include "FrontEndDef.h"


namespace RayTrace
{
    

    
    class Entity
    {
    public:

        Entity(EntityId _mId);



       
        inline bool operator == (const Entity& _rhs) const {
            return m_id == _rhs.m_id;
        }

        struct hash_fn
        {
            std::size_t operator() (const Entity& _rhs) const
            {
                return _rhs.m_id;
            }
        };

        EntityId GetId() const { return m_id; }


        EntityId m_id;        
    };


}
