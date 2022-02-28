#pragma once

#include "ObjectBase.h"



namespace RayTrace
{
   /*
        @brief: Managing base class
    */
    class SystemBase : public ObjectBase
    {
        RT_OBJECT( SystemBase, ObjectBase )
    public:
        SystemBase(Context* _pContext);
        virtual bool PostConstructor() {
            return true;
        }

        virtual bool Clear() = 0;

    protected:
        
    };
}