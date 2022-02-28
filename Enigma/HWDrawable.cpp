#include "Context.h"
#include "HWDrawable.h"

namespace RayTrace
{
    DrawableBase::DrawableBase(Context* _pContext)
        : ObjectBase(_pContext)     
    {

    }

    //const BBox3f& DrawableBase::GetBounds() const
    //{
    //    return m_bounds;
    //}

    //void DrawableBase::SetBounds(const BBox3f& _bounds)
    //{
    //    m_bounds = _bounds;
    //}

}

