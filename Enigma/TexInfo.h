#pragma once
#include "Defines.h"

namespace RayTrace
{
    // TexInfo Declarations
    struct TexInfo {
        TexInfo(const std::string& _filename, bool _doTri, float _maxAniso,
            eImageWrapMode _wrap, float _scale, float _gamma)
            : m_filename(_filename)
            , m_doTrilinear(_doTri)
            , m_maxAniso(_maxAniso)
            , m_wrapMode(_wrap)
            , m_scale(_scale)
            , m_gamma(_gamma)
        { }
        std::string     m_filename;
        bool            m_doTrilinear;
        float           m_maxAniso;
        eImageWrapMode  m_wrapMode;
        float           m_scale,
                        m_gamma;

        bool operator<(const TexInfo& t2) const {
            if (m_filename != t2.m_filename) return m_filename < t2.m_filename;
            if (m_doTrilinear != t2.m_doTrilinear) return m_doTrilinear < t2.m_doTrilinear;
            if (m_maxAniso != t2.m_maxAniso) return m_maxAniso < t2.m_maxAniso;
            if (m_scale != t2.m_scale) return m_scale < t2.m_scale;
            if (m_gamma != t2.m_gamma) return m_gamma < t2.m_gamma;
            return m_wrapMode < t2.m_wrapMode;
        }
    };
}