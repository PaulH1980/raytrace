
#include "SceneManager.h"
#include "Main.h"






int main(int argc, char* args[])
{
  	std::uint32_t valid     = 0;
	valid |= SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER| SDL_INIT_GAMECONTROLLER);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	g_window = SDL_CreateWindow("RayTracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, window_flags);
	if (valid != 0 || g_window == nullptr) 
		Exit("Error Initializing SDL");	
	InitializeOpenGL(); 
    AppInit appInit = { g_window, g_glContext, SCREEN_WIDTH, SCREEN_HEIGHT };

    

    //Init Context
    Context context(appInit);
    context.PostConstructor({ SCREEN_WIDTH, SCREEN_HEIGHT });

    SetupTestScene(&context);

    {
        
        
        
        const auto brdfFile = GetTextureDirectory() + "generated/brdf_lut.hdr";
        
        HWTextureUPtr brdfLut;
        if (!Exists(brdfFile)) {
            auto buffer = std::unique_ptr<HWFrameBuffer>(CreateBrdfLut(&context, 512));
            brdfLut = std::move(buffer->MoveTextures()[0]);
            SaveTexture(ChangeExtension( brdfFile, "" ), brdfLut.get(), true, 0);
        }
        else
        {
            HWTexInfo params;           
            params.m_minFilter = eMinFilter::MIN_FILTER_LINEAR;
            params.m_magFilter = eMagFilter::MAG_FILTER_NEAREST;
            params.m_wrapR = params.m_wrapS = params.m_wrapT = eWrapMode::WRAP_MODE_CLAMP_TO_EDGE;
            params.m_target = eTarget::TARGET_TEXTURE_2D;    
            params.m_mips = false;
            brdfLut.reset( CreateTexture( &context, brdfFile, params) );            
        }
        context.GetResourceManager().AddTexture("BRDF_Lut", std::move(brdfLut));
    }


#if 1
    auto meshes = LoadModel(&context, "gltf/DamagedHelmet/DamagedHelmet.gltf");
   //// auto meshes = LoadModel(&context, "gltf/WaterBottle/WaterBottle.gltf");
   //auto meshes = LoadModel(&context, "gltf/MetalRoughSpheres/MetalRoughSpheres.gltf");
   //auto meshes = LoadModel(&context, "gltf/Sponza/Sponza.gltf");

   // //auto meshes = LoadModel(&context, "Sponza/Sponza.obj");
    for (auto& mesh : meshes)
    {
        context.AddObject(mesh);      
    }


    auto bounds = context.GetSceneManager().GetSceneBounds();
  
    for (int i = 0; i < 20; ++i)
    {
        //context.AddObject(CreateRandomLight(&context), eLayer::LAYER_WORLD_SOLID);
        auto pEnt = CreateLightEntity(&context);
        auto lightComp = pEnt->GetComponent<LightComponent>();
        auto spriteComp = pEnt->GetComponent<EditorSpriteComponent>();
        auto transComp = pEnt->GetComponent<TransformComponent>();
       
       // auto pos = Vector3f(i * 1.0f + 5.0f, 0.0f, 0.0f);  //Vector3f(g_rng.randomFloat(-20.0f, 20.0f), g_rng.randomFloat(-20.0f, 20.0f), g_rng.randomFloat(-20.0f, 20.0f));

        auto pos = glm::linearRand(bounds.m_min, bounds.m_max);
        transComp->m_transform = Translate4x4(pos);

        //lightComp->m_position.x = g_rng.randomFloat(-20.0f, 20.0f);
        //lightComp->m_position.y = g_rng.randomFloat(-20.0f, 20.0f);
        //lightComp->m_position.z = g_rng.randomFloat(-20.0f, 20.0f);
       // lightComp->m_type = eLightType::SPOT_LIGHT;
        lightComp->m_color.x = g_rng.randomFloat(0.2f, 1.0f);
        lightComp->m_color.y = g_rng.randomFloat(0.2f, 1.0f);
        lightComp->m_color.z = g_rng.randomFloat(0.2f, 1.0f);

        //spriteComp->m_texId.m_pTexture = context.GetResourceManager().GetTexture("LightIcon");
       // spriteComp->m_scale = Vector2f(64.0f, 64.0f);
       
		
        context.AddObject(pEnt);	

    }

	

    
#else
   

    /*auto shaderPtr = context.GetResourceManager().GetShader("__DefaultShader");
    auto material = context.GetResourceManager().GetMaterial("__DefaultMaterial");
    for (const auto& m : renderOptions->m_meshes)
    {
        auto meshPtr = CreateHWMesh(&context, m, material, shaderPtr);
        if (meshPtr)
            context.AddObject(meshPtr, eLayer::LAYER_WORLD_SOLID);

    }*/
#endif

 


	Timer timer;	
    timer.start();
	while (true) {
       
        //scene change, new scene etc.
        context.BeginFrame();

        if (!HandleEvents(context))
            break;     
        context.PostEvents();
        //update context
        context.Update(timer.elapsedSeconds());
        timer.reset();      
        //render
		Render(context);	 
        //End Frame
        context.EndFrame();

        //swap buffers
        SDL_GL_SwapWindow(g_window);
	}
  
    SDL_GL_DeleteContext(g_glContext);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}


