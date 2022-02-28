#include "ParameterSet.h"
#include "Filter.h"


namespace RayTrace
{

    float MitchellFilter::Evaluate(const Vector2f& _xy) const
    {
        return Mitchell1D(_xy[0] * m_invRadius[0]) * Mitchell1D(_xy[1] * m_invRadius[1]);
    }

    float BoxFilter::Evaluate(const Vector2f& _xy) const
    {
        return 1.f;
    }

    float LanczosSincFilter::Evaluate(const Vector2f& _xy) const
    {
        return Sinc1D(_xy[0] * m_invRadius[0]) * Sinc1D(_xy[1] * m_invRadius[1]);
    }

    float GaussianFilter::Evaluate(const Vector2f& _xy) const
    {
        return Gaussian(_xy[0], m_expX) * Gaussian(_xy[1], m_expY);
    }

    float TriangleFilter::Evaluate(const Vector2f& _xy) const
    {
        return std::max((float)0, m_radius[0] - std::abs(_xy[0])) *
               std::max((float)0, m_radius[1] - std::abs(_xy[1]));
    }





    Filter* CreateBoxFilter(const ParamSet& _params)
    {
        float xw = _params.FindOneFloat("xwidth", 0.5f);
        float yw = _params.FindOneFloat("ywidth", 0.5f);
        return new BoxFilter(Vector2f(xw, yw));
    }

    Filter* CreateGaussianFilter(const ParamSet& _params)
    {
        float xw = _params.FindOneFloat("xwidth", 2.f);
        float yw = _params.FindOneFloat("ywidth", 2.f);
        float alpha = _params.FindOneFloat("alpha", 2.f);
        return new GaussianFilter(Vector2f(xw, yw), alpha);
    }

    Filter* CreateMitchellFilter(const ParamSet& _params)
    {
        float xw = _params.FindOneFloat("xwidth", 2.f);
        float yw = _params.FindOneFloat("ywidth", 2.f);
        float B = _params.FindOneFloat("B", 1.f / 3.f);
        float C = _params.FindOneFloat("C", 1.f / 3.f);
        return new MitchellFilter(Vector2f(xw, yw), B, C);
    }

    Filter* CreateSincFilter(const ParamSet& _params)
    {
        float xw = _params.FindOneFloat("xwidth", 4.);
        float yw = _params.FindOneFloat("ywidth", 4.);
        float tau = _params.FindOneFloat("tau", 3.f);
        return new LanczosSincFilter(Vector2f(xw, yw), tau);
    }

    Filter* CreateTriangleFilter(const ParamSet& _params)
    {
        float xw = _params.FindOneFloat("xwidth", 2.f);
        float yw = _params.FindOneFloat("ywidth", 2.f);
        return new TriangleFilter(Vector2f(xw, yw));
    }


}

