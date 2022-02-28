
#include "HWCamera.h"

namespace RayTrace
{



    //////////////////////////////////////////////////////////////////////////
    //HWCamera Implementation
    //////////////////////////////////////////////////////////////////////////
    HWCamera::HWCamera(int _width, int _height) : m_width(_width)
        , m_height(_height)
    {
        //m_near = 2;
        //m_far = 8;a

      
        Reset();
    }

    HWCamera::HWCamera() : HWCamera(2, 2)
    {

    }

    void HWCamera::Reset()
    {
        m_near   = 0.1;
        m_far    = 1000.0f;
        m_fovDeg = 45.0f; 
        
        m_proj    = GetIdentity();
        m_projInv = GetIdentity();
        m_view    = GetIdentity();
        m_viewInv = GetIdentity();
        m_ortho   = GetIdentity();
    }

    void HWCamera::Resize(int _width, int _height)
    {
        m_width = _width;
        m_height = _height;
    }

    void HWCamera::SetFov(float _fovDeg /*= 45*/)
    {
        m_fovDeg = _fovDeg;
    }


    void HWCamera::Update()
    {
        m_viewInv = Inverse4x4(m_view);
        m_proj    = Perspective4x4(ToRadians(m_fovDeg), m_width /(float) m_height, m_near, m_far );
        m_projInv = Inverse4x4(m_proj);       

        m_worldToView    = Transpose4x4(m_viewInv);
        m_worldToViewInv = Inverse4x4(m_worldToView);
   //     SetPos({ 0.f, 0.f, 0.f }, m_worldToView);       

        m_ortho = Orthographic4x4({ m_width, m_height });

    }


    Vector3f HWCamera::ddx() const
    {
        const auto p1 = ScreenToWorld(m_width / 2, m_height / 2, 0.0f);
        const auto p2 = ScreenToWorld(m_width / 2 + 1, m_height / 2, 0.0f);
        return p2 - p1;
    }

    Vector3f HWCamera::ddy() const
    {
        const auto p1 = ScreenToWorld(m_width / 2, m_height / 2, 0.0f);
        const auto p2 = ScreenToWorld(m_width / 2, m_height / 2 + 1, 0.0f);
        return p2 - p1;
    }

    void HWCamera::LookAt(const Vector3f& _eye, const Vector3f& _poi, const Vector3f& _up)
    {
        assert(_poi != _eye);
        auto dir   = Normalize(_poi - _eye);
        auto side  = Normalize(Cross(dir, _up));
        auto newUp = Normalize(Cross(dir, side));
        SetPos(_eye, m_view );
        SetForward(dir, m_view);
        SetRight(side, m_view);
        SetUp(newUp, m_view);      
    }

    Vector3f HWCamera::GetRight() const
    {
        return RayTrace::GetRight(m_view);
    }

    const Matrix4x4& HWCamera::GetWorldToView() const
    {
        return m_worldToView;     
    }

    Vector3f HWCamera::GetUp() const
    {
        return RayTrace::GetUp(m_view);
    }

    Vector3f HWCamera::GetForward() const
    {
        return RayTrace::GetForward(m_view);
    }

    Vector3f HWCamera::GetPosition() const
    {
        return RayTrace::GetPos(m_view);
    }

    Vector3f HWCamera::ScreenToWorld(float _winX, float _winY, float _ndcZ) const
    {
        //static const Matrix4x4 Identity;
        const Vector3f screen(_winX, m_height - _winY, _ndcZ);
        const Vector4f viewport(0.f, 0.f, m_width, m_height);
        return  glm::unProject(screen, m_worldToView, m_proj, viewport);
    }

    Vector3f HWCamera::WorldToScreen(const Vector3f& _world) const
    {
        const Vector4f viewport(0.f, 0.f, m_width, m_height);
        auto result = glm::project(_world, m_worldToView, m_proj, viewport);   
        result.y = m_height - result.y;
        return result;        
    }

    Matrix4x4 HWCamera::GetSkyView() const
    {
        auto view = m_view;       
        SetPos(Vector3f{ 0.0f, 0.0f, 0.0f }, view);
        return view;
    }

    const Matrix4x4& HWCamera::GetWorldToViewInv() const
    {
        return m_worldToViewInv;
    }

    const Matrix4x4& HWCamera::GetProj() const
    {
        return m_proj;
    }

   
    const Matrix4x4& HWCamera::GetProjInv() const
    {
        return m_projInv;
    }

    const Matrix4x4& HWCamera::GetOrtho2D() const
    {
        return m_ortho;
    }

    const Matrix4x4& HWCamera::GetView() const
    {
        return m_view;
    }

    const Matrix4x4& HWCamera::GetViewInv() const
    {
        return m_viewInv;
    }

    void HWCamera::SetView(const Matrix4x4& _view)
    {
        m_view = _view;
    }

    void HWCamera::SetPosition(const Vector3f& _pos)
    {
        SetPos(_pos, m_view);
    }

    void HWCamera::setWidth(int _width)
    {
        m_width = _width;
    }

    void HWCamera::setHeight(int _height)
    {
        m_height = _height;
    }

    int HWCamera::GetWidth() const
    {
        return m_width;
    }

    int HWCamera::GetHeight() const
    {
        return m_height;
    }

    float HWCamera::GetNear() const
    {
        return m_near;
    }

    float HWCamera::GetFar() const
    {
        return m_far;
    }

    void HWCamera::SetNear(float _n)
    {
        m_near = _n;
    }

    void HWCamera::SetFar(float _f)
    {
        m_far = _f;
    }

    bool HWCamera::IsOrtho() const
    {
        return m_isOrtho;
    }

    CubemapMatrices::CubemapMatrices(const Vector3f& _pos)
    {
        m_proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        m_views[0] = glm::lookAt(_pos, _pos + Vector3f(1.0f, 0.0f, 0.0f),  Vector3f(0.0f, -1.0f,  0.0f));
        m_views[1] = glm::lookAt(_pos, _pos + Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.0f, -1.0f,  0.0f));
        m_views[2] = glm::lookAt(_pos, _pos + Vector3f(0.0f, 1.0f, 0.0f),  Vector3f(0.0f,  0.0f,  1.0f));
        m_views[3] = glm::lookAt(_pos, _pos + Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f,  0.0f, -1.0f));
        m_views[4] = glm::lookAt(_pos, _pos + Vector3f(0.0f, 0.0f, 1.0f),  Vector3f(0.0f, -1.0f,  0.0f));
        m_views[5] = glm::lookAt(_pos, _pos + Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, -1.0f,  0.0f));
    }

    Plane::Plane(const Vector4f& _eq) : m_normal(_eq)
        , m_distance(_eq.w)
    {
        const auto lengthInv = 1.0f / glm::length(m_normal);
        m_normal *= lengthInv;
        m_distance *= lengthInv;
    }

    Plane::Plane()
    {

    }

    Frustum::Frustum(const Matrix4x4& _view, const Matrix4x4& _proj)
    {
        const auto viewProj = _view * _proj;

        m_planes[0] = Plane(viewProj[3] + viewProj[0]);       // left
        m_planes[1] = Plane(viewProj[3] - viewProj[0]);       // right
        m_planes[2] = Plane(viewProj[3] - viewProj[1]);       // top
        m_planes[3] = Plane(viewProj[3] + viewProj[1]);       // bottom
        m_planes[4] = Plane(viewProj[3] + viewProj[2]);       // near
        m_planes[5] = Plane(viewProj[3] - viewProj[2]);       // far
    }

    void CenterOnBounds( HWCamera* const _pCamera, const Vector3f& _min, const Vector3f& _max)
    {
        const auto center = (_max - _min) * 0.5f;
        auto size = Length(_max - _min) * 1.25f;
        if (size == 0.0f)
            size = 1.0f;     
        
        const auto  viewOffset = _pCamera->GetForward();     
        _pCamera->SetPosition(center + (viewOffset * size));
        
    }

}

