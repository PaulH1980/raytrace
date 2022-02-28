#pragma once

#include "FrontEndDef.h"

namespace RayTrace
{
    std::vector<WrappedEntity*>  LoadModel(Context* _pContext, const std::string& _path, const std::string& _texPath = "");
}