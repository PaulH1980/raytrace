#pragma once
#include "FrontEndDef.h"
#include "ObjectBase.h"
#include "Properties.h"

namespace RayTrace
{


    static const Vector3f DIM_MIN = { 0.001f, 0.001f, 0.001f };
    static const Vector3f DIM_MAX = { 1000.f, 1000.f, 1000.f };

    static const int SD_MIN = 1;
    static const int SD_MAX = 25;
    static const int SIDES_MIN = 3;
    static const int STACKS_MIN = 1;

  
    static const Vector2i SD_MIN2 = { 1,1 };
    static const Vector2i SD_MAX2 = { 25, 25};

    static const Vector3i SD_MIN3 = { 1,1,1 };
    static const Vector3i SD_MAX3 = { 25, 25, 25 };


    class ModifierBase : public ObjectBase
    {
        RT_OBJECT(ModifierBase, ObjectBase)
    public:
        ModifierBase(Context* _pContext);       

        virtual void Apply() = 0;

        void                      ObjectModified() override;

        void                      SetModifierStack(ModifierStack* _pStack);

        ObjPropVector             GetProperties() override;

        GeometryBase*             GetGeometry() const;

        ModifierBase*             Clone() const override = 0;      


    protected:
        ModifierBase(const ModifierBase& _rhs);

        Vector3f m_dimensions = Vector3f(1.0f, 1.0f, 1.0f);
        ModifierStack*   m_pStack = nullptr;
    };

    class MaterialModifier : public ModifierBase
    {
    public:
        RT_OBJECT(MaterialModifier, ModifierBase)

        MaterialModifier(Context* _pContext);

        void                      Apply()override;

        ObjPropVector             GetProperties() override;

        MaterialModifier*         Clone() const override;

        void                      SetMaterial(HWMaterial* _pMaterial);

    protected:
        MaterialModifier(const MaterialModifier& _rhs);
        HWMaterial* m_pMaterial = nullptr;
    };


    class PlaneModifier : public ModifierBase
    {
        RT_OBJECT(PlaneModifier, ModifierBase)
    public:

        PlaneModifier(Context* _pContext);
        void                      Apply()override;
        ObjPropVector             GetProperties() override;
        PlaneModifier*            Clone() const override;

        Vector2i m_subDivs = SD_MIN2;       

    protected:
        PlaneModifier(const PlaneModifier& _rhs);
    };
       


    class DiskModifier : public ModifierBase
    {
        RT_OBJECT(DiskModifier, ModifierBase)
    public:
        DiskModifier(Context* _pContext);
        void                      Apply() override;
        ObjPropVector             GetProperties() override;
        DiskModifier*             Clone() const override;

        int  m_numSides = 8;
        int  m_numStacks = 1;       
    protected:
        DiskModifier(const DiskModifier& _rhs);
    };

    class CubeModifier : public ModifierBase
    {
        RT_OBJECT(CubeModifier, ModifierBase)
    public:
        CubeModifier(Context* _pContext);
        void                      Apply() override;;
        ObjPropVector             GetProperties() override;
        CubeModifier*             Clone() const override;

        Vector3i m_subDivs   = SD_MIN3;

    protected:
        CubeModifier(const CubeModifier& _rhs);
       
    };

    class CylinderModifier : public ModifierBase
    {
        RT_OBJECT(CylinderModifier, ModifierBase)
    public:
        CylinderModifier(Context* _pContext);
        void                      Apply()override;;
        ObjPropVector             GetProperties() override;
        CylinderModifier*         Clone() const override;

        int  m_numSides = 8;
        int  m_numStacks = 1;
        int  m_capStacks = 1;
        bool m_endCaps = true;

    protected:
        CylinderModifier(const CylinderModifier& _rhs);
    };

    class SphereModifier : public ModifierBase
    {
        RT_OBJECT(SphereModifier, ModifierBase)
    public:
        SphereModifier(Context* _pContext);
        void                      Apply()override;
        ObjPropVector             GetProperties() override;
        SphereModifier*           Clone() const override;

        int  m_numSides = 8;
        int  m_numStacks = 2;

    protected:
        SphereModifier(const SphereModifier& _rhs);
    };

    class ConeModifier : public ModifierBase
    {
        RT_OBJECT(ConeModifier, ModifierBase)
    public:
        ConeModifier(Context* _pContext);

        void                      Apply()override;
        ObjPropVector             GetProperties() override;
        ConeModifier*             Clone() const override;

        int  m_numSides  = 8;
        int  m_numStacks = 1;
        int  m_capStacks = 1;
        bool m_endCaps = true;

    protected:
        ConeModifier(const ConeModifier& _rhs);
    };


   
}