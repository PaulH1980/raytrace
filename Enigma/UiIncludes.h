#pragma once

#include "UiHelperFuncs.h"
#include "InputHandler.h"
#include "HWFormats.h"
#include "HWCamera.h"
#include "HWViewport.h"
#include "HWTexture.h"
#include "SceneView.h"
#include "HWFrameBuffer.h"
#include "EventHandler.h"
#include "ResourceManager.h"
#include "HistoryStack.h"
#include "SceneManager.h"
#include "Tools.h"
#include "LayerManager.h"
#include "HWMesh.h"
#include "HWLight.h"
#include "HWMaterial.h"
#include "HWRenderer.h"
#include "Logger.h"
#include "HWTexture.h"
#include "Context.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"
#include "FilePaths.h"
#include "DragDrop.h"
#include "WrappedEntity.h"

namespace RayTrace
{
       
    static const eMouseButtons ButtonRemap[ImGuiMouseButton_COUNT] =
    {
        eMouseButtons::LEFT_MOUSE_BUTTON,
        eMouseButtons::RIGHT_MOUSE_BUTTON,
        eMouseButtons::MIDDLE_MOUSE_BUTTON,
        eMouseButtons::X1_MOUSE_BUTTON,
        eMouseButtons::X2_MOUSE_BUTTON
    };




    inline  uint32_t GetMouseButtons(ImGuiIO& _io)
    {

        uint32_t flags = 0;
        for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
        {
            if (_io.MouseDown[i])
                flags |= (uint32_t)MouseMask(ButtonRemap[i]);
        }
        return flags;
    }


    inline uint32_t GetKeyModifier(ImGuiIO& _io)
    {
        uint32_t mod = 0;
        if (_io.KeyAlt) {
            mod |= eKeyboardModifiers::KEY_MOD_ALT;
        }
        if (_io.KeyShift) {
            mod |= eKeyboardModifiers::KEY_MOD_SHIFT;
        }
        if (_io.KeyCtrl) {
            mod |= eKeyboardModifiers::KEY_MOD_CTRL;
        }
        return mod;
    }


    inline HWTexture* GetFirstColorTexture(const std::vector<std::unique_ptr<HWTexture>>& _textures)
    {
        for (const auto& curTex : _textures)
        {
            if (IsColor(curTex->m_texInfo.m_format))
                return curTex.get();
        }
        return nullptr;
    }
}