#include <random>
#include "LevelLogic.h"
#include "../Components/Identification.h"
#include "../Entities/Prefabs.h"
#include "../Utils/Macros.h"
#include "../Components/Gameplay.h"

using namespace JK; // Example Space Game

int JK::LevelLogic::gameScore = -10; 
const char* JK::LevelLogic::levels[3]{ "../GameLevels/ProjectAndPortfolioLevels/GameLevel1WithPlayer.txt", "../GameLevels/ProjectAndPortfolioLevels/GameLevel2WithPlayer.txt", "../GameLevels/ProjectAndPortfolioLevels/GameLevel3WithPlayer.txt" };
int JK::LevelLogic::levelIndex = 2;

// Connects logic to traverse any players and allow a controller to manipulate them
bool JK::LevelLogic::Init(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	audioEngine = _audioEngine;
	// create an asynchronus version of the world
	gameAsync = game->async_stage(); // just used for adding stuff, don't try to read data
	gameLock.Create();
	// Pull enemy Y start location from config file
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	float enemy1startY = (*readCfg).at("Enemy1").at("ystart").as<float>();
	float enemy1accmax = (*readCfg).at("Enemy1").at("accmax").as<float>();
	float enemy1accmin = (*readCfg).at("Enemy1").at("accmin").as<float>();
	// level one info
	float spawnDelay = (*readCfg).at("Level1").at("spawndelay").as<float>();
	
	queryCache = game->query<Collidable, EntityName>();

	// spins up a job in a thread pool to invoke a function at a regular interval
	timedEvents.Create(spawnDelay * 1000, [this, enemy1startY, enemy1accmax, enemy1accmin]() {
		// compute random spawn location
		std::random_device rd;  // Will be used to obtain a seed for the random number engine
		std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
		std::uniform_real_distribution<float> x_range(-0.9f, +0.9f);
		std::uniform_real_distribution<float> a_range(enemy1accmin, enemy1accmax);
		float Xstart = x_range(gen); // normal rand() doesn't work great multi-threaded
		float accel = a_range(gen);
		// grab enemy type 1 prefab
		flecs::entity et1; 
		if (RetreivePrefab("Enemy Type1", et1)) {
			// you must ensure the async_stage is thread safe as it has no built-in synchronization
			gameLock.LockSyncWrite();
			// this method of using prefabs is pretty conveinent
			gameAsync.entity().is_a(et1)
				.set<Velocity>({ 0,0 })
				.set<Acceleration>({ 0, -accel })
				.set<Position>({ Xstart, enemy1startY });
			// be sure to unlock when done so the main thread can safely merge the changes
			gameLock.UnlockSyncWrite();
		}
	}, 5000); // wait 5 seconds to start enemy wave

	// create a system the runs at the end of the frame only once to merge async changes
	struct LevelSystem {}; // local definition so we control iteration counts
	game->entity("Level System").add<LevelSystem>();
	// only happens once per frame at the very start of the frame
	game->system<LevelSystem>().kind(flecs::OnLoad) // first defined phase
		.each([this](flecs::entity e, LevelSystem& s) {
		// merge any waiting changes from the last frame that happened on other threads
		gameLock.LockSyncWrite();
		gameAsync.merge();
		gameLock.UnlockSyncWrite();
		HandleScore();
		HandleWinCondition();
		HandleResetGame();
	});

	const char* musicLoc = (*readCfg).at("Level1").at("music").as<const char*>();

	switch (levelIndex) {
	case 0:
		musicLoc = (*readCfg).at("Level1").at("music").as<const char*>();
		break;
	case 1:
		musicLoc = (*readCfg).at("Level2").at("music").as<const char*>();
		break;
	case 2:
		musicLoc = (*readCfg).at("Level3").at("music").as<const char*>();
		break;
	default:
		// Handle default case or add error handling
		break;
	}

	currentTrack.Create(musicLoc, audioEngine, 0.10f);
	currentTrack.Play(true);

	return true;
}

// Free any resources used to run this system
bool JK::LevelLogic::Shutdown()
{
	timedEvents = nullptr; // stop adding enemies
	gameAsync.merge(); // get rid of any remaining commands
	game->entity("Level System").destruct();
	// invalidate the shared pointers
	game.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool JK::LevelLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Level System").enable();
	}
	else {
		game->entity("Level System").disable();
	}
	return false;
}

// this method filters through all objects that have been collided with, checks to see if 
void JK::LevelLogic::HandleScore()
{
	queryCache.each([this](flecs::entity e, Collidable& c, EntityName& n)
		{
			// if we collide with the deathbox
			if (e.has<CollidedWith>(flecs::Wildcard) && n.name == "DeathBox" && n.name != "Lemur_LemurMeshData")
			{
				// reset score (set to -10 because it automatically adds 10 points to the beginning of the game because you collide with a new platform right away)
				gameScore = -10;

				// clear entities we've already collided with
				collidedWithEntities.clear();

				// remove the collided with tag (this ensures that it doesn't fall into this if check every frame - 
				// if we remove it every time we bump into it it only gets added again once you hit it again)
				e.remove<CollidedWith>(flecs::Wildcard);
			}
			else if (e.has<CollidedWith>(flecs::Wildcard) && n.name != "Lemur_LemurMeshData")
			{
				// if entity hasn't been collided with yet
				if (EntityAlreadyInVector(n.name, collidedWithEntities) == false)
				{
					// increment score and add this entity to list of collided with items
					gameScore += 10;
					collidedWithEntities.push_back(n.name);
				}
				
				// remove collided with tag so we don't automatically reset to our previous score when we die
				e.remove<CollidedWith>(flecs::Wildcard);
			}
		});

}

void JK::LevelLogic::HandleWinCondition()
{
	queryCache.each([this](flecs::entity e, Collidable& c, EntityName& n)
		{
			// if we bump into flag
			if (e.has<CollidedWith>(flecs::Wildcard) && n.name == "flag.001")
			{
				auto w = game->filter<WinMenu, WinBool>();
				w.each([this](WinMenu& w, WinBool& wb)
					{
						wb.winBool = true;
					});
				e.remove<CollidedWith>(flecs::Wildcard);
			}
		});
}

void JK::LevelLogic::HandleResetGame()
{
	auto w = game->filter<WinMenu, WinBool>();
	w.each([this](WinMenu& w, WinBool& wb)
		{
			resetGame = wb.resetGame;
		});

	if (resetGame)
	{
		// reset score (set to -10 because it automatically adds 10 points to the beginning of the game because you collide with a new platform right away)
		gameScore = -10;

		queryCache.each([this](flecs::entity e, Collidable& c, EntityName& n)
			{
				// if we collide with the deathbox
				if (e.has<CollidedWith>(flecs::Wildcard) && n.name != "Lemur_LemurMeshData")
				{
					// clear entities we've already collided with
					collidedWithEntities.clear();

					// remove the collided with tag (this ensures that it doesn't fall into this if check every frame - 
					// if we remove it every time we bump into it it only gets added again once you hit it again)
					e.remove<CollidedWith>(flecs::Wildcard);
				}
			});
	}
}

// this method iterates through the vector of entities passed in and determines if the entity name passed in is already included in the vector
bool JK::LevelLogic::EntityAlreadyInVector(std::string name, std::vector<std::string> list)
{
	for (int i = 0; i < list.size(); i++)
	{
		// if name of entity is already in list, return true
		if (name == list[i])
		{
			return true;
		}
	}
	
	// if we don't find name of entity in list, return false
	return false;
}
// **** SAMPLE OF MULTI_THREADED USE ****
//flecs::world world; // main world
//flecs::world async_stage = world.async_stage();
//
//// From thread
//lock(async_stage_lock);
//flecs::entity e = async_stage.entity().child_of(parent)...
//unlock(async_stage_lock);
//
//// From main thread, periodic
//lock(async_stage_lock);
//async_stage.merge(); // merge all commands to main world
//unlock(async_stage_lock);