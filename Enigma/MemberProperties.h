#pragma once
#include "Properties.h"
#include "Color.h"

namespace RayTrace
{


    struct FileNameSlot
    {
        std::string m_fileName;
    };

    struct TextureSlot
    {
        HWTexture* m_pTexture = { nullptr };
    };

    struct ShaderSlot {
        ShaderProgram* m_pShader = { nullptr };
    };

    struct MaterialSlot
    {
        HWMaterial* m_pMaterial = { nullptr };
    };

    using PropertyType = std::variant <
        Properties<bool>,
        Properties<std::string>,
        Properties<int>,
        Properties<uint32_t>,
        Properties<float>,
        Properties<double>,
        Properties<Color3f>,
        Properties<Color4f>,
        Properties<Vector2i>,
        Properties<Vector2f>,
        Properties<Vector3i>,
        Properties<Vector3f>,
        Properties<Vector4i>,
        Properties<Vector4f>,
        Properties<TextureSlot>,
        Properties<FileNameSlot>,
        Properties<Matrix4x4>,           //will be decomposed/recomposed
        Properties<MaterialSlot>,
        Properties<ShaderSlot>
    > ;

    /*
        @brief: Used for communicating with other parts of the application,
        in this case mainly the FrontEnd
    */
    struct PropertyData
    {
        enum class ePropertyType
        {
            PROPERTY_TYPE_REGULAR = 0,
            PROPERTY_TYPE_SELECTION = 1,
            PROPERTY_TYPE_MASK = 2,
        };


        struct PropFlag
        {
            std::string m_name;
            uint32_t    m_mask = 0x0;
            std::string m_toolTip;
        };

        struct MemberProperty {

            PropertyType          m_prop;
            std::string           m_name;
            ePropertyType         m_type = ePropertyType::PROPERTY_TYPE_REGULAR;
            bool                  m_isReadOnly = false;
            std::vector<PropFlag> m_propFlags;          //contains names for property flags & selection
            std::string           m_toolTip;

            int GetIndex(uint32_t val) {
                for (int i = 0; i < m_propFlags.size(); ++i)
                    if (val == m_propFlags[i].m_mask)
                        return i;
                return INVALID_ID;
            }
        };
		using PropertyMap = std::map<std::string, int>;



        bool                    AddProperty( MemberProperty&& _prop);
        MemberProperty*         GetProperty(const std::string& _name);
        std::vector<MemberProperty>&           GetProperties();

        std::string m_name; //Objectname
    private:
        PropertyMap                 m_properties;
        std::vector<MemberProperty> m_propVector;
    };


    struct PropertyVisitor
    {
        virtual void operator() (Properties<std::string>&) {}
        virtual void operator() (Properties<bool>&) {}
        virtual void operator() (Properties<int>&) {}
        virtual void operator() (Properties<uint32_t>&) {}
        virtual void operator() (Properties<float>&) {}
        virtual void operator() (Properties<double>&) {}
        virtual void operator() (Properties<Color3f>&) {}
        virtual void operator() (Properties<Color4f>&) {}
        virtual void operator() (Properties<Vector2i>&) {}
        virtual void operator() (Properties<Vector2f>&) {}
        virtual void operator() (Properties<Vector3i>&) {}
        virtual void operator() (Properties<Vector3f>&) {}
        virtual void operator() (Properties<Vector4i>&) {}
        virtual void operator() (Properties<Vector4f>&) {}
        virtual void operator() (Properties<TextureSlot>&) {}
        virtual void operator() (Properties<FileNameSlot>&) {}
        virtual void operator() (Properties<HWTexture*>&) {}
        virtual void operator() (Properties<HWMaterial*>&) {}
        virtual void operator() (Properties<ShaderProgram*>&) {}
        virtual void operator() (Properties<Matrix4x4>&) {}
    };



    using ObjPropVector = std::vector<PropertyData*>;


}