#include "Geometry.h"
#include "Modifiers.h"
#include "ModifierStack.h"


namespace RayTrace
{


    /// <summary>
    /// ModifierStack Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    ModifierStack::ModifierStack(Context* _pContext)
        : ObjectBase(_pContext)
        , m_object(std::make_unique<GeometryBase>())
    {

    }

    ModifierStack::ModifierStack(const ModifierStack& _rhs)
        : ObjectBase(_rhs)
        , m_object(std::make_unique<GeometryBase>())
    {
        for (auto& _mod : _rhs.m_modifiers)
        {
            m_modifiers.push_back(std::unique_ptr<ModifierBase>(_mod->Clone()));
            m_modifiers.back()->SetModifierStack(this);
        }
    }

    ModifierStack::~ModifierStack()
    {

    }

    GeometryBase* ModifierStack::GetGeometry() const
    {
        return m_object.get();
    }

    ModifierStack* ModifierStack::Clone() const
    {
        ModifierStack* newStack = new ModifierStack(*this);
        return newStack;
    }


    void ModifierStack::Apply()
    {
        for (auto& curMod : m_modifiers) {
            curMod->Apply();
        }
    }

    void ModifierStack::Updated(ModifierBase* _pModifier)
    {
        bool apply = false;
        for (auto& curMod : m_modifiers)
        {
            if (curMod.get() == _pModifier) {
                apply = true;
            }
            if (apply)
                curMod->Apply();
        }
    }

    ModifierStack* ModifierStack::AddModifier(std::unique_ptr<ModifierBase> _modifier)
    {
        _modifier->SetModifierStack(this);
        m_modifiers.push_back(std::move(_modifier));
        return this;
    }

}