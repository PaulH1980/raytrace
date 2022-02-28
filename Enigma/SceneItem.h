#pragma once
#include <type_traits>
#include "FrontEndDef.h"

namespace RayTrace
{
   // using DrawFunction = std::function<void(MeshBase* _obj, HWMaterial* _mat, ShaderProgram* _shader)>;
   



    struct SceneItem
    {
        WrappedEntity*      m_pEntity               = nullptr;
        HWMaterial*         m_material              = nullptr;
        ShaderProgram*      m_activeShader          = nullptr;
        ProcessObjectFun    m_pProcessEntityFun     = nullptr;
    };

    using SceneItemVector = std::vector<SceneItem>;
}