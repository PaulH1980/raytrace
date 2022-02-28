#include "UiIncludes.h"
#include "ImGuizmo.h"
#include "EditorLayer.h"
#include "SceneWindow.h"

namespace RayTrace
{

    void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
    {
        float temp, temp2, temp3, temp4;
        temp = 2.0f * znear;
        temp2 = right - left;
        temp3 = top - bottom;
        temp4 = zfar - znear;
        m16[0] = temp / temp2;
        m16[1] = 0.0;
        m16[2] = 0.0;
        m16[3] = 0.0;
        m16[4] = 0.0;
        m16[5] = temp / temp3;
        m16[6] = 0.0;
        m16[7] = 0.0;
        m16[8] = (right + left) / temp2;
        m16[9] = (top + bottom) / temp3;
        m16[10] = (-zfar - znear) / temp4;
        m16[11] = -1.0f;
        m16[12] = 0.0;
        m16[13] = 0.0;
        m16[14] = (-temp * zfar) / temp4;
        m16[15] = 0.0;
    }

    void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
    {
        float ymax, xmax;
        ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
        xmax = ymax * aspectRatio;
        Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
    }

    //////////////////////////////////////////////////////////////////////////
   //SceneWindow
   //////////////////////////////////////////////////////////////////////////
    void SceneWindow::Layout()
    {

    }

    void SceneWindow::Resize(int _width, int _height)
    {
        glm::ivec2 dims(_width, _height);
        m_size = Vector2f(std::max( MIN_FBO_DIMS,  _width), std::max(MIN_FBO_DIMS, _height));
        if (dims != m_info.m_frameBufferSize)
        {
            m_info.m_frameBufferSize = dims;
            auto& context = m_info.m_manager->GetContext();

            EventBase evt(eEvents::EVENT_VIEWPORT_RESIZE, context.GetElapsedTime());
            evt.m_data["width"] = dims.x;
            evt.m_data["height"] = dims.y;
            evt.m_data["windowId"] = (int)m_info.m_id;
            context.AddEvent(evt);
        }
    }

    void SceneWindow::PreDraw()
    {

    }

    void SceneWindow::PostDraw()
    {

    }

    void SceneWindow::HandleEvents()
    {


        auto& context = m_info.m_manager->GetContext();
        float time = context.GetElapsedTime();
        auto& io = ImGui::GetIO();

        auto id = ImGui::GetCurrentWindow()->ID;
        if (id != m_info.m_id)
            return;
        bool isHovered = ImGui::IsItemHovered();
        bool isFocused = ImGui::IsItemFocused();

        Vector2i mousePosAbs = ToVector2i( ImGui::GetMousePos() );     
        Vector2i mousePosLocal = mousePosAbs - m_imagePos;

        if (isHovered)
        {
            if (!m_hovered) //mouse enter
            {
                EventBase evt(eEvents::EVENT_SCENEWINDOW_MOUSE_ENTERED, time);
                evt.m_data["windowId"] = (int)id;
                evt.m_data["sceneView"] = m_info.m_sceneViewName;
                context.AddEvent(evt);
                m_hovered = true;
            }
        }
        else
        {
            if (m_hovered)
            {
                EventBase evt(eEvents::EVENT_SCENEWINDOW_MOUSE_LEAVE, time);
                evt.m_data["windowId"] = (int)id;
                evt.m_data["sceneView"] = m_info.m_sceneViewName;
                context.AddEvent(evt);      
                m_hovered = false;
            }
        }

        if (isFocused) //handle mouse
        {
            if (!m_focused) //focus gained
            {
                EventBase evt(eEvents::EVENT_SCENEWINDOW_FOCUS_GAINED, time);
                evt.m_data["windowId"] = (int)id;
                evt.m_data["sceneView"] = m_info.m_sceneViewName;
                context.AddEvent(evt);
            }

            HandleMouse(Vector2i(mousePosLocal.x, mousePosLocal.y));
            HandleKeyboard();

            m_focused = true;
        }
        else
        {
            if (m_focused) //focus lost
            {
                EventBase evt(eEvents::EVENT_SCENEWINDOW_FOCUS_LOST, time);
                evt.m_data["windowId"] = (int)id;
                evt.m_data["sceneView"] = m_info.m_sceneViewName;
                context.AddEvent(evt);
            }
            m_focused = false;
        }
    }

    void SceneWindow::HandleCursors(const Vector2i& _localPos)
    {
        if (ImGui::IsItemHovered())
        {
            auto winMan   = m_info.m_manager;
            auto& context = winMan->GetContext();
            auto keyMode  = GetKeyModifier(ImGui::GetIO());
            if (keyMode & eKeyboardModifiers::KEY_MOD_ALT)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeAll);
            }
            
            //ImGui::SetMouseCursor(ImGuiMouseCursor_::ImGuiMouseCursor_Hand);
           /* bool valid = true;
            auto objectInfo = context.ObjectInfoFromScreen(_localPos, m_info.m_id, valid);
            if (objectInfo.first != INVALID_ID)
            {
                winMan->SetCursor("CrossHair");
            }*/

            

          //  ImGui::SetMouseCursor(ImGui::imgui)
        }
    }

    void SceneWindow::HandleMouse(const Vector2i& mousePosLocal)
    {
        auto id = ImGui::GetCurrentWindow()->ID;
        auto& context = m_info.m_manager->GetContext();
        float time = context.GetElapsedTime();
        auto& io = ImGui::GetIO();


        auto MouseDown = [&](int _button)
        {
            auto evtId = io.MouseDownDuration[_button] > 0.0f
                ? eEvents::EVENT_MOUSE_PRESSED : eEvents::EVENT_MOUSE_DOWN;


            auto nativeBut = ButtonRemap[_button];
            EventBase evt(evtId, time);
            evt.m_data["x"] = (int)mousePosLocal.x;
            evt.m_data["y"] = (int)mousePosLocal.y;
            evt.m_data["deltaX"] = (int)0;
            evt.m_data["deltaY"] = (int)0;
            evt.m_data["wheelDelta"] = 0.0f;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["buttonsMask"] = (int)GetMouseButtons(io);
            evt.m_data["button"] = (int)nativeBut;
            evt.m_data["downTime"] = io.MouseDownDuration[_button];
            evt.m_data["windowId"] = (int)id;
            context.AddEvent(evt);
        };

        auto MousDoubleClicked = [&](int _button)
        {
            auto evtId = eEvents::EVENT_MOUSE_DOUBLE_CLICKED;

            auto nativeBut = ButtonRemap[_button];
            EventBase evt(evtId, time);
            evt.m_data["x"] = (int)mousePosLocal.x;
            evt.m_data["y"] = (int)mousePosLocal.y;
            evt.m_data["deltaX"] = (int)0;
            evt.m_data["deltaY"] = (int)0;
            evt.m_data["wheelDelta"] = 0.0f;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["buttonsMask"] = (int)GetMouseButtons(io);
            evt.m_data["button"] = (int)nativeBut;
            evt.m_data["downTime"] = io.MouseDownDuration[_button];
            evt.m_data["windowId"] = (int)id;
            context.AddEvent(evt);
        };

        auto MouseReleased = [&](int _button)
        {
            auto nativeBut = ButtonRemap[_button];
            EventBase evt(eEvents::EVENT_MOUSE_UP, time);
            evt.m_data["x"] = (int)mousePosLocal.x;
            evt.m_data["y"] = (int)mousePosLocal.y;
            evt.m_data["deltaX"] = (int)0;
            evt.m_data["deltaY"] = (int)0;
            evt.m_data["wheelDelta"] = 0.0f;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["buttonsMask"] = (int)GetMouseButtons(io);
            evt.m_data["button"] = (int)nativeBut;
            evt.m_data["downTime"] = 0.0f;
            evt.m_data["windowId"] = (int)id;
            context.AddEvent(evt);

        };
        auto MouseMotion = [&](const glm::ivec2& _curPos) {
            EventBase evt(eEvents::EVENT_MOUSE_MOTION, time);
            evt.m_data["x"] = _curPos.x;
            evt.m_data["y"] = _curPos.y;
            evt.m_data["deltaX"] = (int)io.MouseDelta.x;
            evt.m_data["deltaY"] = (int)io.MouseDelta.y;
            evt.m_data["wheelDelta"] = 0.0f;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["buttonsMask"] = (int)GetMouseButtons(io);
            evt.m_data["button"] = (int)-1;
            evt.m_data["downTime"] = 0.0f;
            evt.m_data["windowId"] = (int)id;
            context.AddEvent(evt);
        };



        auto MouseWheel = [&](float _delta)
        {
            EventBase evt(eEvents::EVENT_MOUSE_WHEEL, time);
            evt.m_data["x"] = (int)mousePosLocal.x;
            evt.m_data["y"] = (int)mousePosLocal.y;
            evt.m_data["deltaX"] = (int)0;
            evt.m_data["deltaY"] = (int)0;
            evt.m_data["wheelDelta"] = _delta;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["buttonsMask"] = (int)GetMouseButtons(io);
            evt.m_data["button"] = (int)-1;
            evt.m_data["downTime"] = 0.0f;
            evt.m_data["windowId"] = (int)id;
            context.AddEvent(evt);
        };

        for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
            if (ImGui::IsMouseReleased(i)) MouseReleased(i);
        for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
            if (ImGui::IsMouseDoubleClicked(i)) MousDoubleClicked(i);
        for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
            if (ImGui::IsMouseDown(i)) MouseDown(i);
      


        {//MouseMotion

            if (io.MouseDelta.x != 0.f || io.MouseDelta.y != 0) {
                const glm::ivec2 curPos = { (int)mousePosLocal.x, (int)mousePosLocal.y };
                MouseMotion(curPos);
            }
        }
        {//MouseWheel
            float  wheelDelta = io.MouseWheel;
            if (wheelDelta != 0.0f)
            {
                MouseWheel(wheelDelta);
            }
        }//end mouse input
     

    }

    void SceneWindow::HandleKeyboard()
    {
        auto id = ImGui::GetCurrentWindow()->ID;
        auto& context = m_info.m_manager->GetContext();
        float time = context.GetElapsedTime();
        auto& io = ImGui::GetIO();

        auto KeyDown = [&](int _keyId) {
            EventBase evt(eEvents::EVENT_KEY_DOWN, time);
            evt.m_data["windowId"] = (int)id;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["scanCode"] = _keyId;
            evt.m_data["downTime"] = io.KeysDownDuration[_keyId];
            context.AddEvent(evt);
        };

        auto KeyPressed = [&](int _keyId) {
            EventBase evt(eEvents::EVENT_KEY_PRESSED, time);
            evt.m_data["windowId"] = (int)id;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["scanCode"] = _keyId;
            evt.m_data["downTime"] = io.KeysDownDuration[_keyId];
            context.AddEvent(evt);
        };

        auto KeyReleased = [&](int _keyId) {
            EventBase evt(eEvents::EVENT_KEY_UP, time);
            evt.m_data["windowId"] = (int)id;
            evt.m_data["keyMod"] = (int)GetKeyModifier(io);
            evt.m_data["scanCode"] = _keyId;
            evt.m_data["downTime"] = io.KeysDownDuration[_keyId];
            context.AddEvent(evt);
        };

        for (int i = 0; i < 512; ++i)
            if (ImGui::IsKeyDown(i))
                KeyDown(i);

        for (int i = 0; i < 512; ++i)
            if (ImGui::IsKeyPressed(i))
                KeyPressed(i);

        for (int i = 0; i < 512; ++i)
            if (ImGui::IsKeyReleased(i))
                KeyReleased(i);
    }

    void SceneWindow::Draw()
    {
        ImGuiIO& io = ImGui::GetIO();
        bool visible = IsVisible();
        auto& resMan = m_info.m_manager->GetContext().GetResourceManager();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        ImGui::Begin(m_info.m_name.c_str(), &visible, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();


            const auto size = ImGui::GetContentRegionAvail();
            //const auto pos = ImGui::GetCurrentWindow()->Pos;
            Resize(size.x, size.y);

            const auto& sv = resMan.GetSceneView(m_info.m_sceneViewName);
            assert(sv&& sv->m_viewport.m_frameBuffer);
            const auto& rgbTex = GetFirstColorTexture(sv->m_viewport.m_frameBuffer->GetTextures());
            assert(rgbTex);
            m_imagePos = ToVector2i( ImGui::GetCursorScreenPos() );
            sv->SetWindowPos(m_imagePos);

#pragma warning( push )
#pragma warning(disable:4312) //conversion from 'uint32_t' to 'ImTextureID' of greater size
            auto imgTexId = reinterpret_cast<ImTextureID>(rgbTex->m_texInfo.m_id);
            ImGui::ImageButton(imgTexId, size, ImVec2(0, 1), ImVec2(1, 0), 0);
            {
#pragma warning(pop )
                m_info.m_id = ImGui::GetCurrentWindow()->ID;
                {   //HACK draw tool after drawing viewport, 
                    //scene window shouldn't have knowledge of active tools etc
                    auto pActiveTool = GetContext().GetToolManager().GetActiveTool();
                    if (sv && pActiveTool) {
                        if (sv->m_viewport.GetWindowId() == m_info.m_id)
                            pActiveTool->DrawTool(sv);
                    }
                }

                if (auto editorLayer = static_cast<EditorLayer*>( GetContext().GetLayerManager().GetLayer(LAYER_EDITOR)))
                {
                    editorLayer->DrawEditorOverlay(sv);
                }
                
                HandleEvents();
                HandleCursors(m_imagePos);
            }

        }
        ImGui::End();
        SetVisible(visible);
    }
}

