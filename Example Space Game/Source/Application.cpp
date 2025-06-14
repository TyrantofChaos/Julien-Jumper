#include "Application.h"
// open some Gateware namespaces for conveinence 
// NEVER do this in a header file!
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

//NOT IN USE
//bool Application::Init()
//{
//	eventPusher.Create();
//
//	// load all game settigns
//	gameConfig = std::make_shared<GameConfig>();
//	// create the ECS system
//	game = std::make_shared<flecs::world>();
//	// init all other systems
//	if (InitWindow() == false)
//		return false;
//	if (InitInput() == false)
//		return false;
//	if (InitAudio() == false)
//		return false;
//	//if (InitGraphics() == false)
//	//	return false;
//	if (MyInitGraphics() == false)
//		return false;
//	if (InitEntities() == false)
//		return false;
//	if (InitSystems() == false)
//		return false;
//
//	return true;
//}

bool Application::MyInit()
{
	eventPusher.Create();

	// load all game settigns
	gameConfig = std::make_shared<GameConfig>();
	// create the ECS system
	game = std::make_shared<flecs::world>();
	// init all other systems
	if (InitWindow() == false)
		return false;
	if (InitInput() == false)
		return false;
	if (InitAudio() == false)
		return false;
	if (MyInitGraphics() == false)
		return false;
	if (InitEntities() == false)
		return false;
	if (InitSystems() == false)
		return false;

	return true;
}

// NOT IN USE
//bool Application::Run()
//{
//	//VkClearValue clrAndDepth[2];
//	//clrAndDepth[0].color = { {1, 0, 0, 1} };
//	//clrAndDepth[1].depthStencil = { 1.0f, 0u };
//
//	//// grab vsync selection
//	//bool vsync = gameConfig->at("Window").at("vsync").as<bool>();
//
//	//// set background color from settings
//	//const char* channels[] = { "red", "green", "blue" };
//
//	//for (int i = 0; i < std::size(channels); ++i) {
//	//	clrAndDepth[0].color.float32[i] =
//	//		gameConfig->at("BackGroundColor").at(channels[i]).as<float>();
//	//}
//	//// create an event handler to see if the window was closed early
//	//bool winClosed = false;
//	//GW::CORE::GEventResponder winHandler;
//
//	//winHandler.Create([&winClosed](GW::GEvent e)
//	//	{
//	//		GW::SYSTEM::GWindow::Events ev;
//
//	//		if (+e.Read(ev) && ev == GW::SYSTEM::GWindow::Events::DESTROY)
//	//			winClosed = true;
//	//	});
//
//	//float bckgrdClr[] = { 0.5f, 0.5f, 0.5f, 1.0f };
//
//	//window.Register(winHandler);
//	//while (+window.ProcessWindowEvents())
//	//{
//	//	if (+vulkan.StartFrame(2, clrAndDepth))
//	//	{
//	//		if (GameLoop() == false) {
//	//			vulkan.EndFrame(vsync);
//	//			return false;
//	//		}
//	//		if (-vulkan.EndFrame(vsync)) {
//	//			// failing EndFrame is not always a critical error, see the GW docs for specifics
//	//		}
//	//	}
//	//	else
//	//		return false;
//	//}
//	return true;
//}

bool Application::MyRun()
{
	// grab vsync selection
	bool vsync = gameConfig->at("Window").at("vsync").as<bool>();

	// create an event handler to see if the window was closed early
	bool winClosed = false;
	GW::CORE::GEventResponder winHandler;

	winHandler.Create([&winClosed](GW::GEvent e)
		{
			GW::SYSTEM::GWindow::Events ev;

			if (+e.Read(ev) && ev == GW::SYSTEM::GWindow::Events::DESTROY)
				winClosed = true;
		});

	window.Register(winHandler);

	// OUR GAME LOOP
	while (+window.ProcessWindowEvents())
	{

		// all we need to draw - this method calls game->progress, which calls EACH system we have set up every single frame
		if (GameLoop() == false)
			return false;

		/*
		IDXGISwapChain* swap;
		ID3D11DeviceContext* con;
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* depth;*/

		//if (+directX.GetImmediateContext((void**)&con) &&
		//	+directX.GetRenderTargetView((void**)&rtv) &&
		//	+directX.GetDepthStencilView((void**)&depth) &&
		//	+directX.GetSwapchain((void**)&swap))
		//{
		//	con->ClearRenderTargetView(rtv, bckgrdClr);
		//	con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0); 
		//	//dxRenderingSystem.MyRender();
		//	swap->Present(1, 0);
		//	swap->Release();
		//	rtv->Release();
		//	depth->Release();
		//	con->Release();
		//}
		//else
		//	return false;
	}

	return true;
}

// KEEP
// this method shuts down all systems currently in use
bool Application::MyShutdown()
{
	// disconnect systems from global ECS
	if (playerSystem.Shutdown() == false)
		return false;
	if (levelSystem.Shutdown() == false)
		return false;
	if (dxRenderingSystem.Shutdown() == false)
		return false;
	if (physicsSystem.Shutdown() == false)
		return false;
	if (bulletSystem.Shutdown() == false)
		return false;
	if (enemySystem.Shutdown() == false)
		return false;

	return true;
}

// KEEP
// this method creates our initial window
bool Application::InitWindow()
{
	// grab settings
	int width = gameConfig->at("Window").at("width").as<int>();
	int height = gameConfig->at("Window").at("height").as<int>();
	int xstart = gameConfig->at("Window").at("xstart").as<int>();
	int ystart = gameConfig->at("Window").at("ystart").as<int>();
	std::string title = gameConfig->at("Window").at("title").as<std::string>();
	// open window
	if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDLOCKED) &&
		+window.SetWindowName(title.c_str())) {
		return true;
	}
	return false;
}

// KEEP?
// this method initializes all input variables needed to read input
bool Application::InitInput()
{
	if (-gamePads.Create())
		return false;
	if (-immediateInput.Create(window))
		return false;
	if (-bufferedInput.Create(window))
		return false;
	return true;
}

// KEEP?
// this method initializes our audio variable 
bool Application::InitAudio()
{
	if (-audioEngine.Create())
		return false;
	return true;
}

// NOT IN USE
//bool Application::InitGraphics()
//{
//#ifndef NDEBUG
//	const char* debugLayers[] = {
//		"VK_LAYER_KHRONOS_validation", // standard validation layer
//		//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
//	};
//	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
//		sizeof(debugLayers) / sizeof(debugLayers[0]),
//		debugLayers, 0, nullptr, 0, nullptr, false))
//		return true;
//#else
//	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
//		return true;
//#endif
//	return false;
//}

// this method creates our directX window on top of our window
bool Application::MyInitGraphics()
{
	if (+directX.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		return true;

	return false;
}

// KEEP
// this method initializes all entities used in the game
bool Application::InitEntities()
{
	// Load bullet prefabs
	if (weapons.Load(game, gameConfig, audioEngine) == false)
		return false;
	// Load the player entities
	if (players.Load(game, gameConfig) == false)
		return false;
	// Load the enemy entities
	if (enemies.Load(game, gameConfig, audioEngine) == false)
		return false;

	return true;
}

// KEEP
// this method initializes all systems used in the game
bool Application::InitSystems()
{
	// connect systems to global ECS

	if (levelSystem.Init(game, gameConfig, audioEngine) == false)
		return false;
	if (dxRenderingSystem.Init(game, gameConfig, directX, window) == false)
		return false;
	if (playerSystem.Init(game, gameConfig, immediateInput, bufferedInput,
		gamePads, audioEngine, eventPusher) == false)
		return false;
	if (physicsSystem.Init(game, gameConfig) == false)
		return false;
	if (bulletSystem.Init(game, gameConfig) == false)
		return false;
	if (enemySystem.Init(game, gameConfig, eventPusher) == false)
		return false;
	if (pauseSystem.Init(game, bufferedInput,playerSystem.playerSystem,levelSystem.currentTrack,gamePads) == false)
		return false;
	if (startSystem.Init(game, bufferedInput, playerSystem.playerSystem,gamePads) == false)
		return false;
	if (winSystem.Init(game, bufferedInput, playerSystem.playerSystem,gamePads) == false)
		return false;

	return true;
}

// KEEP
bool Application::GameLoop()
{
	// compute delta time and pass to the ECS system
	static auto start = std::chrono::steady_clock::now();
	double elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();
	start = std::chrono::steady_clock::now();
	// let the ECS system run
	return game->progress(static_cast<float>(elapsed));	// runs all of the systems 
}
