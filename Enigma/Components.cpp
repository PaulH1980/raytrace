#include "WrappedEntity.h"
#include "Context.h"
#include "Geometry.h"
#include "Modifiers.h"
#include "ModifierStack.h"
#include "Components.h"


namespace RayTrace
{

	PropertyData* GetProperties(WrappedEntity* _pObj, CameraComponent* _pComp)
	{
		using Type = CameraComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperties = new PropertyData();
			pProperties->m_name = Type::GetTypeNameStatic();
			pProperties->AddProperty({ Properties<float>(&_pComp->m_fov, _pObj,  true,1.0f, 90.0f), "FOV" });
			pProperties->AddProperty({ Properties<float>(&_pComp->m_near, _pObj, true, 0.001f, 1000000.0f), "Near" });
			pProperties->AddProperty({ Properties<float>(&_pComp->m_far, _pObj,  true, 0.001f, 1000000.0f), "Far" });
			pProperties->AddProperty({ Properties<bool>(&_pComp->m_ortho, _pObj), "Orthographic" });
			pProperties->AddProperty({ Properties<int>(&_pComp->m_width, _pObj), "Width" });
			pProperties->AddProperty({ Properties<int>(&_pComp->m_height, _pObj), "Height" });
			_pObj->AddPropertyData(std::unique_ptr<PropertyData>(pProperties));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, HierarchicalComponent* _pComp)
	{

		using Type = HierarchicalComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperties = new PropertyData();
			pProperties->m_name = Type::GetTypeNameStatic();
			pProperties->AddProperty({ Properties<EntityId>(&_pComp->m_parent, _pObj), "Parent" });
			_pObj->AddPropertyData(std::unique_ptr<PropertyData>(pProperties));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Color3fComponent* _pComp)
	{

		using Type = Color3fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Color3f>(&_pComp->m_color, _pObj), "Color" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Color4fComponent* _pComp)
	{
		using Type = Color4fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Color4f>(&_pComp->m_color, _pObj), "Color" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, BBox2fComponent* _pComp)
	{
		using Type = BBox2fComponent;

		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector2f>(&_pComp->m_bounds.m_min, _pObj), "Min" });
			pProperty->AddProperty({ Properties<Vector2f>(&_pComp->m_bounds.m_max, _pObj), "Max" });
			_pObj->AddPropertyData(std::move(pProperty));

		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, BBox2iComponent* _pComp)
	{
		using Type = BBox2iComponent;

		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector2i>(&_pComp->m_bounds.m_min, _pObj), "Min" });
			pProperty->AddProperty({ Properties<Vector2i>(&_pComp->m_bounds.m_max, _pObj), "Max" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, BBox3fComponent* _pComp)
	{
		using Type = BBox3fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector3f>(&_pComp->m_bounds.m_min, _pObj), "Min" });
			pProperty->AddProperty({ Properties<Vector3f>(&_pComp->m_bounds.m_max, _pObj), "Max" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, BBox3iComponent* _pComp)
	{

		using Type = BBox3iComponent;

		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector3i>(&_pComp->m_bounds.m_min, _pObj), "Min" });
			pProperty->AddProperty({ Properties<Vector3i>(&_pComp->m_bounds.m_max, _pObj), "Max" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, TransformComponent* _pComp)
	{
		using Type = TransformComponent;

		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Matrix4x4>(&_pComp->m_transform, _pObj), "Transform" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, LocalTransformComponent* _pComp)
	{

		using Type = LocalTransformComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Matrix4x4>(&_pComp->m_local, _pObj), "Local" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position2fComponent* _pComp)
	{
		using Type = Position2fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector2f>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position2iComponent* _pComp)
	{
		using Type = Position2iComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector2i>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position3fComponent* _pComp)
	{
		using Type = Position3fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector3f>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position3iComponent* _pComp)
	{
		using Type = Position3iComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector3i>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position4fComponent* _pComp)
	{
		using Type = Position4fComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector4f>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, Position4iComponent* _pComp)
	{
		using Type = Position4iComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector4i>(&_pComp->m_pos, _pObj), "Value" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, LightComponent* _pComp)
	{

		using Type = LightComponent;


		auto GetComponentName = [&]() {
			/*switch (_pComp->m_type)
			{
			case eLightType::AREA_LIGHT:
				return Type::GetTypeNameStatic() + " - Area Light";
			case eLightType::SPOT_LIGHT:
				return Type::GetTypeNameStatic() + " - Spot Light";
			case eLightType::DIR_LIGHT:
				return Type::GetTypeNameStatic() + " - Sun Light";
			case eLightType::POINT_LIGHT:
				return Type::GetTypeNameStatic() + " - Point Light";
			default:
				throw std::exception("Invalid Light Type");
			}*/
			return Type::GetTypeNameStatic();
		};
		/*auto GetLightBasic = [&]()
		{
			
			return pProperty;
		};*/



		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetComponentName();

			PropertyData::MemberProperty lightType = { Properties<uint32_t>((uint32_t*)&_pComp->m_type, _pObj), "Light Type", PropertyData::ePropertyType::PROPERTY_TYPE_SELECTION };
			lightType.m_propFlags.push_back({ "Spot Light", (uint32_t)eLightType::SPOT_LIGHT });
			lightType.m_propFlags.push_back({ "Point Light",(uint32_t)eLightType::POINT_LIGHT });
			lightType.m_propFlags.push_back({ "Sun Light",  (uint32_t)eLightType::DIR_LIGHT });
			lightType.m_propFlags.push_back({ "Area Light", (uint32_t)eLightType::AREA_LIGHT });

			PropertyData::MemberProperty lightFlags = { Properties<uint32_t>(&_pComp->m_flags, _pObj), "Flags", PropertyData::ePropertyType::PROPERTY_TYPE_MASK };
			lightFlags.m_propFlags.push_back({ "Cast Shadows", (uint32_t)0x01 });

			pProperty->AddProperty(std::move(lightType));
			pProperty->AddProperty(std::move(lightFlags));

			pProperty->AddProperty({ Properties<Color3f>(((Color3f*)&_pComp->m_color), _pObj), "Color" });
			pProperty->AddProperty({ Properties<float>(((float*)&_pComp->m_color.w), _pObj), "Intensity" });
			
			
			//auto pProperty = std::move(GetLightBasic());

			//if (_pComp->m_type == eLightType::SPOT_LIGHT)
			{
				pProperty->AddProperty({ Properties<float>(((float*)&_pComp->m_angles.x), _pObj, true, 1.0f, 45.0f), "Inner Angle" });
				pProperty->AddProperty({ Properties<float>(((float*)&_pComp->m_angles.y), _pObj, true, 1.0f, 45.0f), "Outer Angle" });
			}
			//else if (_pComp->m_type == eLightType::AREA_LIGHT)
			{
				pProperty->AddProperty({ Properties<float>(((float*)&_pComp->m_area.x), _pObj, true, 0.001f, 100.0f), "Width" });
				pProperty->AddProperty({ Properties<float>(((float*)&_pComp->m_area.y), _pObj, true, 0.001f, 100.0f), "Height" });
			}
			_pObj->AddPropertyData(std::move(pProperty));
		}

		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, SpriteComponent* _pComp)
	{
		using Type = SpriteComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<Vector2f>(&_pComp->m_scale, _pObj), "Scale" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, TextureComponent* _pComp)
	{


		using Type = TextureComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<TextureSlot>(&_pComp->m_texture, _pObj), "Texture" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, MaterialComponent* _pComp)
	{
		using Type = MaterialComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<MaterialSlot>(&_pComp->m_material, _pObj), "Material" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, ShaderComponent* _pComp)
	{
		using Type = ShaderComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<ShaderSlot>(&_pComp->m_shader, _pObj), "Material" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}

	PropertyData* GetProperties(WrappedEntity* _pObj, AudioComponent* _pComp)
	{
		using Type = AudioComponent;
		if (!_pObj->ContainsProperty(Type::GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = Type::GetTypeNameStatic();
			pProperty->AddProperty({ Properties<FileNameSlot>(&_pComp->m_fileName, _pObj), "File" });
			pProperty->AddProperty({ Properties<int>(&_pComp->m_repeatCount, _pObj), "Repeat" });
			pProperty->AddProperty({ Properties<float>(&_pComp->m_attenuation, _pObj), "Attenuation" });
			_pObj->AddPropertyData(std::move(pProperty));
		}
		return _pObj->GetPropertyData(Type::GetTypeNameStatic());
	}


	EditorGeometricComponent::EditorGeometricComponent(const EditorGeometricComponent& _rhs)
		: m_pContext( _rhs.m_pContext )
		, m_pModStack( ModifierStackUPtr( _rhs.m_pModStack->Clone()))
	{

	}

	EditorGeometricComponent::EditorGeometricComponent(EditorGeometricComponent&& _rhs)
		: m_pContext(_rhs.m_pContext)
		, m_pModStack(std::move( _rhs.m_pModStack ) )
	{
	}

	EditorGeometricComponent::~EditorGeometricComponent()
	{
	}

	EditorGeometricComponent& EditorGeometricComponent::operator=(const EditorGeometricComponent& _rhs)
	{
		m_pContext = _rhs.m_pContext;
		m_pModStack = ModifierStackUPtr(_rhs.m_pModStack->Clone());
		return *this;
	}

	EditorGeometricComponent& EditorGeometricComponent::operator=(EditorGeometricComponent&& _rhs)
	{
		m_pContext = _rhs.m_pContext;
		m_pModStack = std::move(_rhs.m_pModStack);
		return *this;
	}

	void EditorGeometricComponent::PostConstructor(Context* _pContext)
	{
		m_pModStack = std::make_unique<ModifierStack>(_pContext);
		m_pContext = _pContext;
	}

}