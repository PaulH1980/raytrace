#pragma once
#include "Transform.h"
namespace RayTrace
{
    class HWCamera;
    
    enum eCubeSides
    {
        SIDE_NEG_X,
        SIDE_POS_X,
        SIDE_NEG_Y,
        SIDE_POS_Y,
        SIDE_NEG_Z,
        SIDE_POS_Z
    };
    
    struct CubemapMatrices
    {
        CubemapMatrices(const Vector3f& _pos = { 0.f, 0.f, 0.f });

        Matrix4x4 m_proj;
        Matrix4x4 m_views[6];
    };
    

    struct Plane
    {
        Plane();
        
        Plane(const Vector4f& _eq);        
        
        Vector3f m_normal;
        float    m_distance;
    };

    struct Frustum
    {
        Frustum(const Matrix4x4& _view, const Matrix4x4& _proj);        
        
        Plane    m_planes[6];
    };

    void  CenterOnBounds(HWCamera* const _pCamera, const Vector3f& _min, const Vector3f& _max );
    
    
    //////////////////////////////////////////////////////////////////////////
    //HWCamera declaration
    //////////////////////////////////////////////////////////////////////////
    class HWCamera
    {
    public:
        HWCamera();


        HWCamera(int _width, int _height);

        void      Reset();

        void      Resize(int _width, int _height);
        void      SetFov(float _fovDeg = 45);
        void      Update();

        Vector3f  ddx() const;
        Vector3f  ddy() const;
              
        void      LookAt(const Vector3f& _eye, const Vector3f& _poi, const Vector3f& _up);
        Vector3f  GetRight() const;
        Vector3f  GetUp() const;
        Vector3f  GetForward() const;
        Vector3f  GetPosition() const;

        Vector3f  ScreenToWorld(float _winx, float _winY, float _ndcZ ) const;
        Vector3f  WorldToScreen(const Vector3f& _world) const;
        
        Matrix4x4        GetSkyView() const;

        const Matrix4x4& GetWorldToViewInv() const;
        const Matrix4x4& GetWorldToView() const;
        const Matrix4x4& GetProj() const;
        const Matrix4x4& GetProjInv() const;
        const Matrix4x4& GetOrtho2D() const;
        const Matrix4x4& GetView() const;
        const Matrix4x4& GetViewInv() const;

        void      SetView(const Matrix4x4& _view);
        void      SetPosition(const Vector3f& _pos);

        void      setWidth(int _width);
        void      setHeight(int _height);

        int       GetWidth() const;
        int       GetHeight() const;

        float     GetNear() const;
        float     GetFar() const;

        void      SetNear(float _n);
        void      SetFar( float _f );
        bool      IsOrtho() const;

    private:


        int       m_width = 1,
                  m_height = 1;
        float     m_fovDeg = 45.0f;
        float     m_near = 0.1f;
        float     m_far = 10.0f;
        bool      m_isOrtho = false;
        Matrix4x4 m_proj;
        Matrix4x4 m_projInv;
        Matrix4x4 m_view;
        Matrix4x4 m_viewInv;
        Matrix4x4 m_worldToView;
        Matrix4x4 m_worldToViewInv;
        Matrix4x4 m_ortho;
    };
}