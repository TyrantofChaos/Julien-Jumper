#ifndef APPLICATION_H
#define APPLICATION_H

// include events
#include "Events/Playevents.h"
// Contains our global game settings
#include "GameConfig.h"
// Load all entities+prefabs used by the game 
#include "Entities/BulletData.h"
#include "Entities/PlayerData.h"
#include "Entities/EnemyData.h"
// Include all systems used by the game and their associated components
#include "Systems/PlayerLogic.h"
#include "Systems/DirectXRendererLogic.h"
#include "Systems/LevelLogic.h"
#include "Systems/PhysicsLogic.h"
#include "Systems/BulletLogic.h"
#include "Systems/EnemyLogic.h"
//#include "Systems/PauseSystem.h"

// Allocates and runs all sub-systems essential to operating the game

class Application
{
	// gateware libs used to access operating system
	GW::SYSTEM::GWindow window; // gateware multi-platform window
	GW::GRAPHICS::GVulkanSurface vulkan; // gateware vulkan API wrapper
	GW::GRAPHICS::GDirectX11Surface directX; // gateware directX API wrapper
	GW::INPUT::GController gamePads; // controller support
	GW::INPUT::GInput immediateInput; // twitch keybaord/mouse
	GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse
	GW::AUDIO::GAudio audioEngine; // can create music & sound effects
	// third-party gameplay & utility libraries
	std::shared_ptr<flecs::world> game; // ECS database for gameplay
	std::shared_ptr<GameConfig> gameConfig; // .ini file game settings
	// ECS Entities and Prefabs that need to be loaded
	JK::BulletData weapons;
	JK::PlayerData players;
	JK::EnemyData enemies;
	// specific ECS systems used to run the game
	JK::PlayerLogic playerSystem;
	JK::DirectXRendererLogic dxRenderingSystem;
	JK::LevelLogic levelSystem;
	JK::PhysicsLogic physicsSystem;
	JK::BulletLogic bulletSystem;
	JK::EnemyLogic enemySystem;
	JK::PauseSystem pauseSystem;
	JK::StartSystem startSystem;
	JK::WinSystem winSystem;
	// EventGenerator for Game Events
	GW::CORE::GEventGenerator eventPusher;

public:
	//bool Init();	// NOT IN USE
	bool MyInit();
	//bool Run();		// NOT IN USE
	bool MyRun();
	bool MyShutdown();
private:
	bool InitWindow();
	bool InitInput();
	bool InitAudio();
	//bool InitGraphics();	// NOT IN USE
	bool MyInitGraphics();
	bool InitEntities();
	bool InitSystems();
	bool GameLoop();
};

#endif 