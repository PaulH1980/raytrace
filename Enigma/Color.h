#pragma once
#include "Defines.h"
#include "MathCommon.h"

namespace RayTrace
{
    class Color3f
    {
    public:
        Color3f()
        {

        }

        Color3f(float _rgb)
        {
            m_rgb[0] = _rgb;
            m_rgb[1] = _rgb;
            m_rgb[2] = _rgb;
        }

        Color3f(float _r, float _g, float _b)           
        {
            m_rgb[0] = _r;
            m_rgb[1] = _g;
            m_rgb[2] = _b;
        }

        Color3f(const Vector3f& _rgb) : m_rgb(_rgb) {}


        bool operator == (const Color3f& _rhs) const {
            return m_rgb == _rhs.m_rgb;
        }

        bool operator != (const Color3f& _rhs) const {
            return m_rgb != _rhs.m_rgb;
        }

        Color3f& operator = (const Color3f& _rhs) {
            m_rgb = _rhs.m_rgb;
            return *this;
        }

        float& operator [] (int _index) {
            return m_rgb[_index];
        }

        const float& operator [] (int _index) const {
            return m_rgb[_index];
        }



        Vector3f m_rgb;
    };
    
    
    
    class Color4f
    {
    public:
        Color4f(float _rgb, float _a = 1.0f) {
            m_rgba[0] = _rgb;
            m_rgba[1] = _rgb;
            m_rgba[2] = _rgb;
            m_rgba[3] = _a;
        }

        Color4f(float _r, float _g, float _b, float _a)  
        {
            m_rgba[0] = _r;
            m_rgba[1] = _g;
            m_rgba[2] = _b;
            m_rgba[3] = _a;
        }
        Color4f() {
            m_rgba[3] = 1.0f;
        }

        Color4f(const Color3f& _rgb) {
            m_rgba[0] = _rgb.m_rgb[0];
            m_rgba[1] = _rgb.m_rgb[1];
            m_rgba[2] = _rgb.m_rgb[2];
            m_rgba[3] = 1.0f;
        }
        Color4f(const Vector4f& _rgba) : m_rgba(_rgba) {}


        Color4f& operator = (const Color4f& _rhs) {
            m_rgba = _rhs.m_rgba;
            return *this;
        }

        float& operator [] (int _index) {
            return m_rgba[_index];
        }

        const float& operator [] (int _index) const {
            return m_rgba[_index];
        }


        bool operator == (const Color4f& _rhs) const {
            return m_rgba == _rhs.m_rgba;
        }

        bool operator != (const Color4f& _rhs) const {
            return m_rgba != _rhs.m_rgba;
        }


        Vector4f m_rgba;
    };


   
    inline Color3f Clamp(const Color3f& _v, const Color3f& _lo, const Color3f& _hi) {
        const auto tmp = Max(_v.m_rgb, _lo.m_rgb);
        return Min(tmp, _hi.m_rgb);
    }

   
    inline Color4f Clamp(const Color4f& _v, const Color4f& _lo, const Color4f& _hi) {
        const auto tmp = Max(_v.m_rgba, _lo.m_rgba);
        return Min(tmp, _hi.m_rgba);
    }


}