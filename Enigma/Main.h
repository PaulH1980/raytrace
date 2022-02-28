#pragma once

#define GLEW_STATIC
#include  <glew.h>
#include <imgui.h>
#include <backends\imgui_impl_sdl.h>


//Using SDL, SDL OpenGL, GLEW, standard IO, and strings
#include <SDL.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ShapeImp.h"
#include "TriangleMesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Film.h"
#include "Filter.h"
#include "Spectrum.h"
#include "Renderer.h"
#include "Volume.h"
#include "Lights.h"
#include "Integrator.h"
#include "DirectLightingIntegrator.h"
#include "PathIntegrator.h"
#include "BdptIntegrator.h"
#include "VolPathIntegrator.h"
#include "Sample.h"
#include "Sampler.h"
#include "RNG.h"
#include "Timer.h"
#include "Texture.h"
#include "Material.h"
#include "HwMaterial.h"
#include "BVH.h"
#include "Primitive.h"
#include "Scene.h"
#include "Defines.h"
#include "Medium.h"
#include "Parser.h"
#include "Api.h"
#include "ImageFilmSDL.h"
#include "HWMesh.h"
#include "HWTexture.h"
#include "HWFrameBuffer.h"
#include "ResourceManager.h"
#include "Context.h"
#include "ParameterSet.h"
#include "HWShader.h"
#include "ResourceManager.h"
#include "AppInit.h"
#include "Geometry.h"
#include "HWCamera.h"
#include "ModelLoader.h"
#include "HWRenderer.h"
#include "io.h"
#include "WrappedEntity.h"
#include "HWLight.h"
#include "FilePaths.h"


//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

#define OPENGL_MAJOR   4
#define OPENGL_MINOR   3

SDL_Renderer* g_renderer = nullptr;
SDL_Window* g_window = nullptr;
SDL_GLContext  g_glContext = nullptr;

using namespace RayTrace;

void Exit(const char* _errString)
{
    std::cerr << _errString << std::endl;
    exit(1);
}


void InitializeOpenGL()
{
    uint32_t valid = 0;
    valid |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    valid |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR);
    valid |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR);
    valid |= SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    valid |= SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    valid |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    g_glContext = SDL_GL_CreateContext(g_window);
    if (valid != 0 || g_glContext == nullptr)
        Exit("Error Initializing SDL-OpenGL");

    glewExperimental = GL_TRUE;
    auto status = glewInit();
    if (status != GLEW_OK) {
        auto errString = glewGetErrorString(status);
        Exit((const char*)errString);
    }


    SDL_GL_MakeCurrent(g_window, g_glContext);
    valid |= SDL_GL_SetSwapInterval(1);
    if (valid != 0)
        Exit("Swap Interval Failed");
}

void InitializeRayTrace()
{

    SampledSpectrum::Init();
}

void Render(Context& context) {
    context.PreRender();
    context.Render();
    context.PostRender();
}


bool HandleEvents(Context& context) 
{
    if (context.ExitRequested())
        return false;

    const float time = context.GetElapsedTime();
    const ImGuiIO& io = ImGui::GetIO();
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        if (event.type == SDL_QUIT) //exit requested
            return false;

        ImGui_ImplSDL2_ProcessEvent(&event);


        if (event.type == SDL_WINDOWEVENT) //handle window event
        {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

                context.Resize({ event.window.data1, event.window.data2 });
            }
        }

#if 0 
        if (!io.WantCaptureMouse) //handle mouse input
        {
            if (event.type == SDL_MOUSEMOTION)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                EventBase evt(eEvents::EVENT_MOUSE_MOTION, time);
                evt.m_data["x"] = x;
                evt.m_data["y"] = y;
                evt.m_data["windowId"] = (int)INVALID_ID;
                evt.m_data["mask"] = (int)event.motion.state;
                context.AddEvent(evt);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                EventBase evt(eEvents::EVENT_MOUSE_DOWN, time);
                evt.m_data["x"] = x;
                evt.m_data["y"] = y;
                evt.m_data["windowId"] = (int)INVALID_ID;
                evt.m_data["button"] = (int)event.button.button;
                context.AddEvent(evt);

            }
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                EventBase evt(eEvents::EVENT_MOUSE_UP, time);
                evt.m_data["x"] = x;
                evt.m_data["y"] = y;
                evt.m_data["windowId"] = (int)INVALID_ID;
                evt.m_data["button"] = (int)event.button.button;
                context.AddEvent(evt);
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);

                EventBase evt(eEvents::EVENT_MOUSE_WHEEL, time);
                evt.m_data["x"] = x;
                evt.m_data["y"] = y;
                evt.m_data["windowId"] = (int)INVALID_ID;
                evt.m_data["delta"] = (int)event.wheel.direction;
                context.AddEvent(evt);
            }
        }
        if (!io.WantCaptureKeyboard) //handle keyboard input
        {
            if (event.type == SDL_KEYDOWN)
            {
                EventBase evt(eEvents::EVENT_KEY_DOWN, time);
                evt.m_data["scanCode"] = event.key.keysym.sym;
                evt.m_data["windowId"] = (int)INVALID_ID;
                context.AddEvent(evt);
            }
            else if (event.type == SDL_KEYUP)
            {
                EventBase evt(eEvents::EVENT_KEY_UP, time);
                evt.m_data["scanCode"] = event.key.keysym.sym;
                evt.m_data["windowId"] = (int)INVALID_ID;
                context.AddEvent(evt);
            }
        }
#endif
    } //while

    return true;
}



Film* CreateFilm()
{
    return  new ImageFilmSDL({ 128,128 },
        BBox2f(Vector2f(0, 0), Vector2f(1, 1)),
        std::make_unique<TriangleFilter>(Vector2f(1, 1)),
        g_renderer,
        "test.png");
}



void SetupTestScene(Context* _pContext)
{
#if 0
    InitializeRayTrace();
    pbrtInit(PbrtOptions);
    PbrtOptions.film = CreateFilm();
    pbrtParseFile("scenes/cornell-box/scene.pbrt");
    renderOptions->m_integrator->Render(*renderOptions->m_scene);
#endif
    {
        CreateEnvironmentData(_pContext, GetTextureDirectory() + "skybox/barcelona/barcelona.hdr", 
            512, 128, 256);
    }

    auto pMat = _pContext->GetResourceManager().GetMaterial("__DefaultSkyBoxMaterial");
    pMat->m_textures[eTextureTypes::ALBEDO] = _pContext->GetResourceManager().GetTexture("Env_Cube");

   // skyBoxMaterial->m_textures[eTextureTypes::ALBEDO] = GetResourceManager().GetTexture("__DefaultSeaSkybox");
   // valid &= GetResourceManager().AddMaterial("__DefaultSkyBoxMaterial", std::unique_ptr<HWMaterial>(skyBoxMaterial));


}