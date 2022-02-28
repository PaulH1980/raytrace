#pragma once
#include <imgui.h>
#include <imgui_internal.h>

#include "FrontEndDef.h"

namespace RayTrace
{
   
  
   
    
    
    
    inline ImVec2 operator + (const ImVec2& a, const ImVec2& b)
    {
        return ImVec2(a.x + b.x, a.y + b.y);
    }

    inline ImVec2 operator - (const ImVec2& a, const ImVec2& b)
    {
        return ImVec2(a.x - b.x, a.y - b.y);
    }

    inline bool IsInside(const ImVec2& _a, const ImVec2& _b)
    {
        return (_a.x >= 0.f && _a.x < _b.x) &&
            (_a.y >= 0.f && _a.y < _b.y);
    }

    inline Vector2i ToVector2i(const ImVec2& _in) {
        return Vector2i(_in.x, _in.y);
    }


    inline Vector2f ToVector2f(const ImVec2& _in) {
        return Vector2f(_in.x, _in.y);
    }

    inline ImVec2 ToImVec2(const Vector2f& _in) {
        return ImVec2(_in.x, _in.y);
    }

    inline ImVec2 ToImVec2(const Vector2i& _in) {
        return ImVec2(_in.x, _in.y);
    }

    
}