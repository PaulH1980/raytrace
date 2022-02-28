#pragma once
#include "FrontEndDef.h"

#include "Transform.h"
#include "SystemBase.h"
#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "InputHandler.h"

namespace RayTrace
{

	struct Interpolation
	{
		Matrix4x4    m_from,
			         m_to;
		float        m_interpolationTime = 0.0f;
		float        m_invInterpolation = 0.0f;
		float        m_distance = 0.0f;

		bool         m_interpolating = false;
		bool         IsInterpolating(float _currentTime) const;
	};
    
    class ControllerBase : public ObjectBase,  public InputReceiver
    {
        RT_OBJECT(ControllerBase, ObjectBase)

    public:
        ControllerBase(Context* _pContext);
        virtual ~ControllerBase(){};

        virtual void Update(float _dt) = 0;   
       
        void         SetViewportId(int _id);
        int          GetViewportId() const { return m_viewportId; }


        bool         IsActive() const;
        void         SetActive(bool _v);


    protected:
        bool m_active       = true;      
        int  m_viewportId   = INVALID_ID;
    };    
  



    class CameraControllerBase : public ControllerBase
    {
        RT_OBJECT(CameraControllerBase, ControllerBase)
    public:
        CameraControllerBase(Context* _pContext, HWCamera* _camera = nullptr);
		void         LookAt(const Matrix4x4& _from, const Matrix4x4& _to, float _distance, float _time );

        HWCamera*    GetCamera() const;
        void         SetCamera(HWCamera* _camera);
        void         Update(float _dt) override;
      

    protected:
        HWCamera*    m_pCamera = nullptr;    
    
        float        m_dt = 0;
        float        m_totalTime = 0;
        Interpolation    m_interpolation;
    };

    class ArcballCameraController : public CameraControllerBase
    {
        RT_OBJECT(ArcballCameraController, CameraControllerBase)
    public:
        ArcballCameraController(Context* _pContext, HWCamera* _pCamera = nullptr);

        void         Update(float _dt) override;


    private:
        bool         HandleInterpolation();

        bool         KeyDown(const KeyInfo& _info) override;
        bool         KeyUp(const KeyInfo& _info) override;
        bool         MouseMove(const MouseInfo& _info) override;
        bool         MouseDown(const MouseInfo& _info)override;
        bool         MouseUp(const MouseInfo& _info)override;
        bool         MouseWheel(const MouseInfo& _info) override;
        bool         Resize(const Vector2i& _size) override;
        bool         MouseDoubleClicked(const MouseInfo& _info) override;

      //ector3f     MouseOnSphere(const Vector3f& _mouse, const Vector2f& _center, float _radius = 0.75f) const;
        Vector3f     MouseOnSphere(const Vector2f& _mouse, float _radius = 0.75f) const;



        Quatf        m_qDown,
                     m_qNow;
                       
        Matrix4x4    m_transform;
                    
        Vector3f     m_poi;
      
        float        m_distance = { 1.0f };
        float        m_arcballScale   = 0.75f;
        bool         m_mouseDrag      = false;
        bool         m_leftMouseDown  = false;
        bool         m_rightMouseDown = false;     

        Vector2f     m_mouseDownPos;       
        Vector2f     m_curMousePos;

    };
    

    /*
        @brief: Simple camera controller uses AWSD keys to pan & dolly, LMB for look around
    */
    class FPSCameraController : public CameraControllerBase
    {
        RT_OBJECT(FPSCameraController, CameraControllerBase )
    public:
        FPSCameraController(Context* _pContext, HWCamera* _camera = nullptr);
        void         Update(float _dt) override;
        

    private:    

        bool         KeyDown(const KeyInfo& _info) override;
        bool         KeyUp(const KeyInfo& _info) override;
        bool         MouseMove(const MouseInfo& _info) override;
        bool         MouseDown(const MouseInfo& _info)override;
        bool         MouseUp(const MouseInfo& _info)override;
        bool         MouseWheel(const MouseInfo& _info) override;
        bool         Resize(const Vector2i& _size) override;
        bool         MouseDoubleClicked(const MouseInfo& _info) override;


        Vector2i  m_mouseDownPos;
        Vector2i  m_lastMousePos;
        Matrix4x4 m_rotInv;            
        bool      m_mouseDown = false;
      
       
        float     m_mouseMoveScale = 15.f;
        float     m_speed = 5.0f;
        float     m_yaw   = 0.0f;  //[0...360]
        float     m_pitch = 0.0f; //[0..180]
    };
}