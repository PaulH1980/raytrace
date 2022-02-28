//#include "Context.h"
//#include "HWLight.h"
//
//
//namespace RayTrace
//{
//
//   /* LightBase::LightBase(Context* _pContext)
//        : DrawableBase( _pContext )
//    {
//        m_name = fmt::format("Light_0{}", m_objectId);
//    }
//
//    ObjPropVector LightBase::GetProperties()
//    {
//        auto retVal = DrawableBase::GetProperties();
//
//        PropertyData objProps;
//        objProps._name = GetTypeNameStatic();
//        objProps.m_properties.push_back({ Properties<Vector3f>(m_position, this),"Position" });
//
//        Color3f* data =(Color3f*) &m_color;
//        objProps.m_properties.push_back({ Properties<Color3f>((Color3f&)(*data) , this),    "Color" });
//
//        PropertyData::MemberProperty objFlags = { Properties<uint32_t>((uint32_t&)m_type, this), "Light Type", PropertyData::ePropertyType::PROPERTY_TYPE_SELECTION};
//        objFlags.m_propFlags.push_back({ "Spot Light", (uint32_t)eLightType::SPOT_LIGHT });
//        objFlags.m_propFlags.push_back({ "Point Light",(uint32_t)eLightType::POINT_LIGHT });
//        objFlags.m_propFlags.push_back({ "Sun Light",  (uint32_t)eLightType::DIR_LIGHT });
//        objFlags.m_propFlags.push_back({ "Area Light", (uint32_t)eLightType::AREA_LIGHT });
//        objProps.m_properties.push_back(std::move(objFlags));
//
//        retVal.push_back(std::move(objProps));
//        return retVal;
//
//    }*/
//
//    LightBase* CreateRandomLight(Context* _pContext)
//    {
//        auto *pLight = new LightBase(_pContext);
//        pLight->m_position.x = g_rng.randomFloat(-20.0f, 20.0f);
//        pLight->m_position.y = g_rng.randomFloat(-20.0f, 20.0f);
//        pLight->m_position.z = g_rng.randomFloat(-20.0f, 20.0f);
//        pLight->m_type       = eLightType::POINT_LIGHT;
//        pLight->m_color.m_rgba.a = 1.0f;
//        pLight->m_color.m_rgba.r = g_rng.randomFloat(0.2f, 1.0f);
//        pLight->m_color.m_rgba.g = g_rng.randomFloat(0.2f, 1.0f);
//        pLight->m_color.m_rgba.b = g_rng.randomFloat(0.2f, 1.0f);
//        pLight->m_attenuation.x = g_rng.randomFloat(50, 100);
//        pLight->m_attenuation.y = g_rng.randomFloat(50, 100);
//
//        return pLight;
//
//    }
//
//}
//
