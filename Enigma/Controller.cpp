#include <iostream>
#include "HWRenderer.h"
#include "Context.h"
#include "EventHandler.h"
#include "Logger.h"
#include "Controller.h"

namespace RayTrace
{

    Quatf QuatFromUnitSphere(const Vector3f& from, const Vector3f& to)
    {
        const auto axis  = cross(from, to);
        const auto angle = glm::dot(from, to);
        return Quatf(angle, axis.x, axis.y, axis.z);       
    }

    Vector2f ToNdc(const Vector2f& _pos, const Vector2f& _viewport) {
        Vector2f ndc = _pos / _viewport; //[0....1];
        ndc *= 2.0f;
        ndc -= 1.0f;
        ndc.y *= -1.0f;
        return ndc;
    }




    //////////////////////////////////////////////////////////////////////////
    //ControllerBase
    //////////////////////////////////////////////////////////////////////////
    ControllerBase::ControllerBase(Context* _pContext)
        : ObjectBase(_pContext) 
    {

    }

    void ControllerBase::SetViewportId(int _id)
    {
        m_viewportId = _id;
    }

    bool ControllerBase::IsActive() const
    {
        return m_active;
    }

    void ControllerBase::SetActive(bool _v)
    {
        m_active = _v;
    }


    //////////////////////////////////////////////////////////////////////////
    //CameraControllerBase
    //////////////////////////////////////////////////////////////////////////
    CameraControllerBase::CameraControllerBase(Context* _pContext, HWCamera* _camera)
        : ControllerBase(_pContext)
        , m_pCamera(_camera)
        , m_dt(0.f)
        , m_totalTime(0.f)
    {

    }

	void CameraControllerBase::LookAt(const Matrix4x4& _from, const Matrix4x4& _to, float _distance, float _time)
	{
        m_interpolation.m_from = _from;
        m_interpolation.m_to   = _to;
        m_interpolation.m_distance = _distance;
        m_interpolation.m_interpolationTime = m_totalTime + _time; 
        m_interpolation.m_invInterpolation = 1.0f / _time;
	}

	HWCamera* CameraControllerBase::GetCamera() const
    {
        return m_pCamera;
    }

    void CameraControllerBase::SetCamera(HWCamera* _camera)
    {
        m_pCamera = _camera;
    }


    void CameraControllerBase::Update(float _dt)
    {
        m_dt = _dt;
        m_totalTime += _dt;
    }


	//////////////////////////////////////////////////////////////////////////
    //CameraController Implementation
    //////////////////////////////////////////////////////////////////////////  
    FPSCameraController::FPSCameraController(Context* _pContext, HWCamera* _camera)
        : CameraControllerBase( _pContext, _camera )        
      
    {
       
        SubcriberFunc Focus = [=](EventBase& _evt) {
            if (m_viewportId == INVALID_ID) return;
            auto min = GetVector3f(_evt.m_data["min"]);
            auto max = GetVector3f(_evt.m_data["max"]);
            CenterOnBounds(m_pCamera, min, max);
        };

        SubcriberFunc Update = [=](EventBase& _evt) {  
            if (m_viewportId == INVALID_ID) return;
            auto dt = GetFloat(_evt.m_data["deltaTime"]);
            this->Update(dt);
        };
        _pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_FOCUS_ON_SCENEOBJECT_REQUEST, { this,  Focus });
        _pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_UPDATE, { this,  Update });
          
        GetContext().GetLogger().AddMessage(std::string("Created ") + GetTypeNameStatic());
    } 

    bool FPSCameraController::KeyDown(const KeyInfo& _info)
    {
        UNUSED(_info)
        return true;
    }


    bool FPSCameraController::KeyUp(const KeyInfo& _info)
    {
        UNUSED(_info)
        return true;
    }  

    bool FPSCameraController::MouseMove(const MouseInfo& _info)
    {
        if (!m_mouseDown)
            return false;

        const auto curPos = Vector2i(_info.m_x, _info.m_y);
        const auto deltaPos = curPos - m_lastMousePos;

        m_yaw += deltaPos[0] * m_mouseMoveScale * m_dt;
        m_pitch += deltaPos[1] * m_mouseMoveScale * m_dt;
        //check boundaries
        m_pitch = std::clamp(m_pitch, -89.9f, 89.9f);
        while (m_yaw > 360.f)
            m_yaw -= 360.f;
        while (m_yaw < 0.f)
            m_yaw += 360.f;
        m_lastMousePos = curPos;

        const auto rotX = RotateX4x4(m_pitch);
        const auto rotY = RotateY4x4(m_yaw);
        m_rotInv = (rotX * rotY);
        
        return true;
    }


    bool FPSCameraController::MouseDown(const MouseInfo& _info)
    {
        const auto curPos = Vector2i(_info.m_x, _info.m_y);
        if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON) {
            m_lastMousePos = m_mouseDownPos = curPos;
            m_mouseDown = true;
        }        
        
        return true;
    }
  

    bool FPSCameraController::MouseUp(const MouseInfo& _info)
    {
        const auto curPos = Vector2i(_info.m_x, _info.m_y);
        if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON) {
            m_lastMousePos = m_mouseDownPos = curPos;
            m_mouseDown = false;
        }      
        return true;
    } 

    bool FPSCameraController::MouseWheel(const MouseInfo& _info)
    {
        auto delta = _info.m_mouseWheelDelta;

        if (delta > 0) {
            m_speed *= 1.1f;
        }
        else if (delta < 0)
            m_speed /= 1.1;

        if (m_speed < 0.01)
            m_speed = 0.01f;
        
        return true;
    }
  

    bool FPSCameraController::Resize(const Vector2i& _size)
    {
        m_pCamera->Resize(_size.x, _size.y);
        
        return true;
    }

    bool FPSCameraController::MouseDoubleClicked(const MouseInfo& _info)
    {
        UNUSED(_info)
        return false;
    }

    void FPSCameraController::Update(float _dt)
    {
        CameraControllerBase::Update(_dt);
        const auto& viewMat = m_pCamera->GetView();
        Vector3f camPos = GetPos(viewMat);
        Vector3f side   = GetRight(viewMat);
        Vector3f view   = GetForward (viewMat );
        Vector3f dir;
        if (GetContext().GetInputHandler().KeyIsDown(SDL_SCANCODE_W))
            dir += view * -1.0f;
        if (GetContext().GetInputHandler().KeyIsDown(SDL_SCANCODE_S))
            dir += view * 1.0f;
        if (GetContext().GetInputHandler().KeyIsDown(SDL_SCANCODE_A))
            dir += side * -1.0f;
        if (GetContext().GetInputHandler().KeyIsDown(SDL_SCANCODE_D))
            dir += side * 1.0f;

        if (LengthSqr(dir) > 0.0f) {
            dir = Normalize(dir);
            camPos += dir * (m_speed * _dt);
        }     
        const Matrix4x4 trans = Translate4x4( camPos );        
        m_pCamera->SetView(m_rotInv * trans);
        m_pCamera->Update();
    }

    //////////////////////////////////////////////////////////////////////////
    ///ArcballCameraController
    //////////////////////////////////////////////////////////////////////////
    ArcballCameraController::ArcballCameraController(Context* _pContext, HWCamera* _pCamera)
        : CameraControllerBase( _pContext, _pCamera )
    {
        SubcriberFunc Focus = [=](EventBase& _evt) {
            if (m_viewportId == INVALID_ID) return;
            auto min = GetVector3f(_evt.m_data["min"]);
            auto max = GetVector3f(_evt.m_data["max"]);
            m_poi = (max - min) * 0.5f;          
        };

        SubcriberFunc Update = [=](EventBase& _evt) {
            if (m_viewportId == INVALID_ID) return;
            auto dt = GetFloat(_evt.m_data["deltaTime"]);
            this->Update(dt);
        };
        _pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_FOCUS_ON_SCENEOBJECT_REQUEST, { this,  Focus });
        _pContext->GetEventHandler().RegisterSubscriber(eEvents::EVENT_UPDATE, { this,  Update });


        glm::quat qOne(1.0, 0.0, 0.0, 0.0);
        glm::vec3 vZero(0.0, 0.0, 0.0);
        m_qDown = qOne;
        m_qNow  = qOne;

    }

    void ArcballCameraController::Update(float _dt)
    {
        CameraControllerBase::Update(_dt);
        
        if (HandleInterpolation())
            return;
       
        m_pCamera->SetView( ( m_transform ) );
        auto view = m_pCamera->GetForward() * m_distance;
        m_pCamera->SetPosition(view + m_poi);
        m_pCamera->Update();
    }

	bool ArcballCameraController::HandleInterpolation()
	{
		if ( m_interpolation.IsInterpolating( m_totalTime ) )
		{
			const auto mat = Transpose4x4(RayTrace::Interpolate( m_interpolation.m_to, m_interpolation.m_from, 
                              (m_interpolation.m_interpolationTime - m_totalTime) * m_interpolation.m_invInterpolation));
			const auto pos = GetPos(mat);

			m_pCamera->SetView(mat);
			m_pCamera->Update();
            m_interpolation.m_interpolating = true;
			return true;
		}
		else if ( m_interpolation.m_interpolating ) //interpolation finished
		{
			const auto mat = Transpose4x4(m_interpolation.m_to);
			const auto pos = GetPos(mat);           
            m_distance = m_interpolation.m_distance;
			m_poi = pos + (m_pCamera->GetForward() * -m_interpolation.m_distance);			
			m_qDown = m_qNow = quat_cast(mat);
            m_transform = mat4_cast( m_qNow );
            m_interpolation.m_interpolating = false;			
		}
        return false;
	}


	bool ArcballCameraController::KeyDown(const KeyInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;
        
        return true;
    }

    bool ArcballCameraController::KeyUp(const KeyInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;
        
        
        return true;
    }

    bool ArcballCameraController::MouseMove(const MouseInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;

        const Vector2f curMousePos(_info.m_x, _info.m_y);

        if( m_rightMouseDown )
        {
            const auto center = m_pCamera->GetForward() * m_distance * -1.0f;
            const auto right = center + m_pCamera->GetRight();
            const auto up    = center + m_pCamera->GetUp();
            //world distances
            const auto lengthRight = Length(right - center);
            const auto lengthUp    = Length(up - center);
            //pixel distances
            const auto pixLengthX = (m_pCamera->WorldToScreen(right) - m_pCamera->WorldToScreen(center)).x;
            const auto pixLengthY = (m_pCamera->WorldToScreen(up) - m_pCamera->WorldToScreen(center)).y;

            const auto worldPerPixX = lengthRight / pixLengthX;
            const auto worldPerPixY = lengthUp / pixLengthY;
                        
            const auto deltaPos = m_curMousePos - curMousePos;

            const float xOffset = deltaPos.x * worldPerPixX * 0.5f;
            const float yOffset = deltaPos.y * worldPerPixY * 0.5f;

            m_curMousePos = curMousePos;
            m_poi += (m_pCamera->GetRight() * xOffset) + (m_pCamera->GetUp() * yOffset);

        }
        else if (m_leftMouseDown)
        {
            const Vector2f dims(m_pCamera->GetWidth(), m_pCamera->GetHeight() );
            m_curMousePos = curMousePos;
        
            const auto sphereFrom  = MouseOnSphere( ToNdc( m_mouseDownPos, dims), m_arcballScale );
            const auto sphereTo    = MouseOnSphere( ToNdc( curMousePos, dims), m_arcballScale );         
            const auto qDrag = QuatFromUnitSphere(sphereFrom, sphereTo);

            m_qNow = qDrag * m_qDown;
            m_transform = mat4_cast(( (m_qNow))) ;
        }        
        return true;
    }

    bool ArcballCameraController::MouseDown(const MouseInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;
                
        if ( _info.m_mouseButton == eMouseButtons::RIGHT_MOUSE_BUTTON )
        {
            m_rightMouseDown = true;
        }
        else if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON) {
            m_leftMouseDown = true;
            m_qDown = m_qNow;
        }
        m_mouseDownPos = m_curMousePos = Vector2f(_info.m_x, _info.m_y);
        return true;
    }

    bool ArcballCameraController::MouseUp(const MouseInfo& _info)
    {
        if( _info.m_mouseButton == eMouseButtons::RIGHT_MOUSE_BUTTON)
            m_rightMouseDown = false;
        
        else if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON)
            m_leftMouseDown = false;
       
        m_mouseDownPos = m_curMousePos = Vector2f(_info.m_x, _info.m_y);
        return false; //passthrough
    }

    bool ArcballCameraController::MouseWheel(const MouseInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;
        
        const float delta = _info.m_mouseWheelDelta;
        if (delta > 0.f)
            m_distance *= 1.f / 1.1f; //zoom in
        else
            m_distance *= 1.1; //zoom out
        if (m_distance < m_pCamera->GetNear())
            m_distance = m_pCamera->GetNear();
        
        return true;
    }

    bool ArcballCameraController::Resize(const Vector2i& _size)
    {
        m_pCamera->Resize(_size.x, _size.y);
        return true;
    }

    bool ArcballCameraController::MouseDoubleClicked(const MouseInfo& _info)
    {
        if (!GetContext().GetInputHandler().AltDown())
            return false;
        if (_info.m_mouseButton == eMouseButtons::LEFT_MOUSE_BUTTON)
        {
            bool valid = true;
            auto objData = GetContext().ObjectInfoFromScreen(Vector2i(_info.m_x, _info.m_y), m_viewportId, valid);
            if (valid)
                m_poi = objData.second;           
            return true;
        }       
        return false;       
    }

    Vector3f ArcballCameraController::MouseOnSphere(const Vector2f& _mouse, float _radius /*= 0.75f*/) const
    {
        Vector3f ballMouse(_mouse, 0.0f);
        ballMouse /= _radius;
        const auto mag = Dot(ballMouse, ballMouse);
        if (mag > 1.0f)
            ballMouse *= 1.0 / sqrtf(mag);
        else
            ballMouse.z = sqrtf(1.0 - mag);
        return ballMouse;
    }

	bool Interpolation::IsInterpolating(float _currentTime) const
	{
        return m_interpolationTime > _currentTime;
	}

}

