#pragma once

#include "ImGuizmo.h"
#include "UiIncludes.h"


#include "PropertyWindow.h"

namespace RayTrace
{
    class ChangeMaterial :public HistoryItem
    {
    public:
        ChangeMaterial(Context* _pContext, HWMaterial* _pMat ,std::string& _exec, const std::string& _unExec, eTextureTypes _type )
            : HistoryItem(_pContext)
            , m_type(_type)
            , m_pMat(_pMat)
        {
        }


		bool Undo() override
		{
			return Apply(m_unexec);
		}

		bool Redo() override
		{
			return Execute();
		}

		bool Execute() override
		{
			return Apply(m_exec);
		}

		bool Apply(const std::string tex)
        {

         }



    private:
        std::string m_exec,
            m_unexec;
        eTextureTypes m_type;
        HWMaterial* m_pMat = nullptr;
    };



    /*
        @brief: convenience template
    */
    template<typename T>
    class TPropertyHistory : public HistoryItem
    {
    public:
        TPropertyHistory(Context* _pContext, const UUID& _object, 
            const std::string& _propGroup, 
            const std::string& _propName, 
            const T& _exec, const T& _unExec)
            : HistoryItem(_pContext, _propName)
            , m_objectId( _object )
            , m_propGroup( _propGroup )
            , m_propName( _propName )
            , m_exec(_exec)
            , m_unExec(_unExec)
        {
            m_eventToSend = eEvents::EVENT_OBJECT_MEMBER_CHANGED;           
        }

        virtual ~TPropertyHistory()
        {

        }

        void SetEvent(eEvents _evt) {
            m_eventToSend = _evt;
        }

        bool Undo() override
        {
            return Apply(m_unExec);
        }

        bool Redo() override
        {
            return Execute();
        }

        bool Execute() override
        {
            return Apply(m_exec);
        }

        bool Apply(const T& _val) {

            auto pEnt = GetContext().GetSceneManager().GetObject(m_objectId);
            assert(pEnt);
            pEnt->GetProperties();

            auto pPropGroup = pEnt->GetPropertyData(m_propGroup);
            assert(pEnt);
            auto propData    = pPropGroup->GetProperty(m_propName);
            auto wrappedType = std::get<Properties<T>>(propData->m_prop);
            wrappedType.SetValue(_val);      
            
            if (m_eventToSend != eEvents::EVENT_UNDEFINED)
            {
                EventBase evt(m_eventToSend, GetContext().GetElapsedTime());
                evt.m_data["entity"] = m_objectId;
                GetContext().AddEvent(evt);
            }
            return true;
        }


    private:
        UUID            m_objectId;
        std::string     m_propGroup,
                        m_propName;
        T               m_exec;
        T               m_unExec;      
        eEvents         m_eventToSend = eEvents::EVENT_UNDEFINED;
    };


    template<typename MemberType>
    void HandleItemChange(Context* _pContext,
        const std::string _objId,
        const std::string& _propGroup,
        const std::string& _propName,
        Properties<MemberType>& _prop,
        const MemberType& _oldValue)
    {
		if (ImGui::IsItemActivated()) {
			_prop.m_initialValue = _oldValue;
		}
		if (ImGui::IsItemDeactivatedAfterEdit() && (_prop.m_editingValue != _prop.m_initialValue))
		{
			auto pHistory = new TPropertyHistory<MemberType>(_pContext, _objId, _propGroup, _propName, _prop.m_editingValue, _prop.m_initialValue);
			auto pContext = &_prop.GetWrappedObject()->GetContext();
			pContext->GetHistory().Execute(std::unique_ptr<HistoryItem>(pHistory));
		}
    }  
    
    
    void EditTransform(const HWCamera& camera, Matrix4x4& matrix)
    {
        static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
        static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
        static Vector3f snapTransRotScale[3];

        if (ImGui::IsKeyPressed(90))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(69))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(82)) // r Key
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), matrixTranslation, matrixRotation, matrixScale);
        ImGui::InputFloat3("Tr", matrixTranslation);
        ImGui::InputFloat3("Rt", matrixRotation);
        ImGui::InputFloat3("Sc", matrixScale);
        
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(matrix));

        if (mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;
        }
        static bool useSnap(false);
        if (ImGui::IsKeyPressed(83))
            useSnap = !useSnap;
        ImGui::Checkbox("", &useSnap);
        ImGui::SameLine();
       Vector3f snap;
        switch (mCurrentGizmoOperation)
        {
        case ImGuizmo::TRANSLATE:
            snap = snapTransRotScale[0];
            ImGui::InputFloat3("Trans. Snap", &snap.x);
            break;
        case ImGuizmo::ROTATE:
            snap = snapTransRotScale[1];
            ImGui::InputFloat("Angle Snap", &snap.x);
            break;
        case ImGuizmo::SCALE:
            snap = snapTransRotScale[2];
            ImGui::InputFloat("Scale Snap", &snap.x);
            break;
        }        
    }
    
    
    
    
    
    
    
    //////////////////////////////////////////////////////////////////////////
   //Visitor for property grid
   //////////////////////////////////////////////////////////////////////////
    struct UiPropVisitor : public PropertyVisitor
    {
        const char* GetName() const {
            return m_curMember.m_name.c_str();
        }

        Context& GetContext() {
            return *m_pContext;
        }


        UiPropVisitor(PropertyGrid* _grid, const std::string& _objId, const std::string& _propGroupName,  PropertyData::MemberProperty& _prop )
            : m_propGrid(_grid)
            , m_pContext( &_grid->GetContext() )
            , m_curMember(_prop)
            , m_objId(_objId)
            , m_propGroup( _propGroupName )
            , m_propName( _prop.m_name )

        {
        }

        virtual void operator() (Properties<bool>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
			if (ImGui::Checkbox(GetName(), &value)) 
			{
				_prop.m_editingValue = value;
			}
			HandleItemChange<bool>( m_pContext, m_objId, m_propGroup, m_propName,  _prop, prevValue);
        }
        virtual void operator() (Properties<int>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (_prop.IsRanged())
            {
                if (ImGui::SliderInt(GetName(), &value, _prop.m_min, _prop.m_max, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
                   _prop.m_editingValue = value;               
            }
            else {

                if (ImGui::InputInt(GetName(), &value))               
                    _prop.m_editingValue = value;             
            }
            HandleItemChange<int>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }
        virtual void operator() (Properties<uint32_t>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;

            std::string name = "";
               
            if (m_curMember.m_type == PropertyData::ePropertyType::PROPERTY_TYPE_MASK) {
                // ImGui::CheckboxFlags()
                if (ImGui::BeginTable("split", 2))
                {
                    for (const auto& flag : m_curMember.m_propFlags) {
                        ImGui::TableNextColumn();
                        if (ImGui::CheckboxFlags(flag.m_name.c_str(), &value, flag.m_mask)) {
                            name = "Mask";
                        }
                    }                    
                    ImGui::EndTable();                 
                }
            }
            else if(m_curMember.m_type == PropertyData::ePropertyType::PROPERTY_TYPE_SELECTION) {
         
                int curIdx = m_curMember.GetIndex(value);
                assert(curIdx != INVALID_ID);
                const auto& curItem = m_curMember.m_propFlags[curIdx];

                if (ImGui::BeginCombo(m_curMember.m_name.c_str(), curItem.m_name.c_str(), 0))
                {
                    for (int n = 0; n < m_curMember.m_propFlags.size(); n++)
                    {
                        const auto& prop = m_curMember.m_propFlags[n];
                        const bool isSelected = (curIdx == n);
                        if (ImGui::Selectable(prop.m_name.c_str(), isSelected))
                            curIdx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();                  
                    name = "DropDown";
                    value = m_curMember.m_propFlags[curIdx].m_mask;   
                }
            } //regular uint32
            else {

                int tmp = static_cast<int>( value );
				if (_prop.IsRanged())
				{
					if (ImGui::SliderInt(GetName(), &tmp, _prop.m_min, _prop.m_max, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
						_prop.m_editingValue = tmp;
				}
				else {

					if (ImGui::InputInt(GetName(), &tmp))
						_prop.m_editingValue = tmp;
				}
                value = tmp;
            }
            if (value != prevValue)
            {
                auto pHistory = new TPropertyHistory<uint32_t>( m_pContext, m_objId, m_propGroup, m_propName, value, prevValue );
                m_pContext->GetHistory().Execute(std::unique_ptr<HistoryItem>(pHistory));
            }
        }

        virtual void operator() (Properties<float>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (_prop.IsRanged())
            {
                if (ImGui::SliderFloat(GetName(), &value, _prop.m_min, _prop.m_max, "%1f", 1.0f))
                {
                    _prop.m_editingValue = value;
                }
            }
            else{
                if (ImGui::InputFloat(GetName(), &value))
                {
                    _prop.m_editingValue = value;
                }
            }
            HandleItemChange<float>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<double>& _prop) {
            auto value     = _prop.GetValue();
            auto prevValue = value;
			if (_prop.IsRanged())
			{
                float tmp = value;
                if (ImGui::SliderFloat(GetName(), &tmp, _prop.m_min, _prop.m_max, "%1f", 1.0f))
				{
					_prop.m_editingValue = tmp;
				}
			}
			else {
				if (ImGui::InputDouble(GetName(), &value))
				{
					_prop.m_editingValue = value;
				}
			}
            HandleItemChange<double>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Color3f>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;

            if (ImGui::ColorEdit3(GetName(), (float*)&value.m_rgb)) 
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Color3f>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Color4f>& _prop)
        {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (ImGui::ColorEdit4(GetName(), (float*)&value.m_rgba))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Color4f>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector2i>& _prop) 
        {
            auto prevValue = _prop.GetValue();
            auto value = _prop.GetValue();
			if (ImGui::InputInt2(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector2i>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector2f>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (ImGui::InputFloat2(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector2f>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector3i>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (ImGui::InputInt3(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector3i>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector3f>& _prop) {
            auto prevValue = _prop.GetValue();
            auto value = _prop.GetValue();
            if (ImGui::InputFloat3(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector3f>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector4i>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (ImGui::InputInt4(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector4i>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<Vector4f>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
            if (ImGui::InputFloat4(GetName(), glm::value_ptr(value)))
			{
				_prop.m_editingValue = value;
			}
            HandleItemChange<Vector4f>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }

        virtual void operator() (Properties<TextureSlot>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;        
        }

        virtual void operator() (Properties<FileNameSlot>& _prop) {
            auto prevValue = _prop.GetValue();
            auto value = _prop.GetValue();        
        }

        virtual void operator() (Properties<MaterialSlot>& _prop) {
            static const int IMG_SIZE = 64;
            auto value = _prop.GetValue();
            auto prevValue = value;   
            auto pMat = value.m_pMaterial;

            assert(pMat);
            if (ImGui::BeginTable("matTable", 1)) 
            {
                for (int i = 0; i < NUM_EDITOR_SLOTS; ++i)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    auto curTex = pMat->m_textures[i];
                    bool useConstant  = pMat->m_useConstant[i];
                    const auto name = "constant " + std::to_string(i);
                    if (curTex)
                    {
                        if (useConstant)
                        {
                            
                        }
                        
                        const auto& texInfo = curTex->m_texInfo;
                        ImGui::Image((ImTextureID*)texInfo.m_id, ImVec2(IMG_SIZE, IMG_SIZE));
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (auto payLoad = ImGui::AcceptDragDropPayload(TEXTURE_DND))
                            {
                                auto& resMan        = GetContext().GetResourceManager();
                                const std::string fullPath = (const char*)payLoad->Data;
                                const auto relPath  = RelativePath(GetWorkDirectory(), fullPath);
                                auto texPtr = resMan.GetTexture(relPath);
                                if (!texPtr)
                                {
									HWTexInfo params;
									params.m_magFilter = eMagFilter::MAG_FILTER_LINEAR;
									params.m_minFilter = eMinFilter::MIN_FILTER_LINEAR_MIPMAP_LINEAR;;
									params.m_wrapR = params.m_wrapS = params.m_wrapT = eWrapMode::WRAP_MODE_REPEAT;
									params.m_target = eTarget::TARGET_TEXTURE_2D;
									params.m_mips = true;
                                    texPtr = CreateTexture(&GetContext(), fullPath, params);
                                    if (texPtr)
                                        resMan.AddTexture(relPath, HWTextureUPtr(texPtr));
                                    
                                }
                                assert(texPtr);
                                pMat->m_textures[i] = texPtr;

                                AddLogMessage(&GetContext(), (const char*)payLoad->Data, eLogLevel::LOG_LEVEL_INFO);

                            }
                        }
                        ImGui::SameLine();
                        ImGui::Text(FileName(texInfo.m_texName).c_str());
                      
                        ImGui::Checkbox(name.c_str(), &useConstant);
                        pMat->m_useConstant[i] = useConstant;
                        ImGui::Separator();
                    }
                    else
                    {

                    }
                    ImGui::Separator();
                }
                ImGui::EndTable();
            }
        }

        virtual void operator() (Properties<ShaderSlot>& _prop) {
            auto value = _prop.GetValue();
            auto prevValue = value;
        }

        virtual void operator() (Properties<Matrix4x4>& _prop) {
            
            using namespace IMGUIZMO_NAMESPACE;
            
            auto value = _prop.GetValue();
            auto prevValue = value;
            ImGui::Text(GetName());
            float trans[3],
                  scale[3],
                  rot[3];

            DecomposeMatrixToComponents( glm::value_ptr(value), trans, rot, scale);

            bool modified = false;
            //Generate unique names so imgui handles it correctly
            std::string names[3] = 
            {
                std::string( "Translate##" ) + GetName(),
                std::string( "Rotate##") + GetName(),
                std::string( "Scale##") + GetName(),
            };

          
            modified |= ImGui::InputFloat3(names[0].c_str(), trans);
            modified |= ImGui::InputFloat3(names[1].c_str(), rot);
            modified |= ImGui::InputFloat3(names[2].c_str(), scale);
            if (modified) {
                RecomposeMatrixFromComponents(trans, rot, scale, glm::value_ptr(value) );
				auto pHistory = new TPropertyHistory<Matrix4x4>(m_pContext, m_objId, m_propGroup, m_propName, value, prevValue);
				m_pContext->GetHistory().Execute(std::unique_ptr<HistoryItem>(pHistory));
            }            
        }


        virtual void operator() (Properties<std::string>& _prop) {

            static const std::size_t BUF_SIZE = 256;
           
            auto value     = _prop.GetValue();
            auto prevValue = value;

            char buf[BUF_SIZE];
            memcpy(buf, value.data(), std::min(value.length() + 1, BUF_SIZE));
            if (ImGui::InputText(GetName(), buf, BUF_SIZE))
			{
				_prop.m_editingValue = std::string(buf);
			}
            HandleItemChange<std::string>(m_pContext, m_objId, m_propGroup, m_propName, _prop, prevValue);
        }


        PropertyGrid* m_propGrid;
        PropertyData::MemberProperty& m_curMember;

        Context*    m_pContext = nullptr;
        UUID        m_objId;
        std::string m_propGroup,
                    m_propName;
    };
}