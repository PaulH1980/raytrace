#pragma once
#include "FrontEndDef.h"
#include <type_traits>


namespace RayTrace
{
    
    template< typename Base, typename Derived >
    const Derived* DownCast(const Base* _val)
    {
        static_assert(std::is_base_of<Base, Derived>::value);
#if _DEBUG
        return dynamic_cast<const Derived*>(_val);
#else
        return static_cast<const Derived*>(_val);
#endif
    }

    template< typename Base, typename Derived >
    Derived* DownCast(Base* _val)
    {
        static_assert(std::is_base_of<Base, Derived>::value);
#if _DEBUG
        return dynamic_cast<Derived*>(_val);
#else
        return static_cast<Derived*>(_val);
#endif
    }

    std::string ToLower(std::string _in);

    std::string ToUpper(std::string _in);

    bool        EndsWith(const std::string & _input, const std::string & _ending);
   
}