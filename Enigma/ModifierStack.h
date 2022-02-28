#pragma once
#include "ObjectBase.h"

namespace RayTrace
{

    class GeometryBase;

    class ModifierStack : public ObjectBase
    {
        RT_OBJECT(ModifierStack, ObjectBase)

    public:
        ModifierStack(Context* _pContext);

        ~ModifierStack();

        GeometryBase*  GetGeometry() const;
        ModifierStack* Clone() const override;

        void           Apply();
        void           Updated(ModifierBase* _pModifier);
        ModifierStack* AddModifier(std::unique_ptr<ModifierBase> _modifier);


        std::vector<std::unique_ptr<ModifierBase>> m_modifiers;
        std::unique_ptr<GeometryBase> m_object;

    protected:
        ModifierStack(const ModifierStack& _rhs);
    };
}