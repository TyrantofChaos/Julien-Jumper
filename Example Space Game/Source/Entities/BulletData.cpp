#include "BulletData.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "Prefabs.h"
#include "../Components/Gameplay.h"

bool JK::BulletData::Load(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	
	// Create prefab for lazer weapon(repurposing for jump and land sfx)

	std::string jumpFX = (*readCfg).at("Player1").at("jumpFX").as<std::string>();
	std::string landFX = (*readCfg).at("Player1").at("landFX").as<std::string>();
	std::string resetFX = (*readCfg).at("Player1").at("resetFX").as<std::string>();
	// Load sound effect used by this bullet prefab
	GW::AUDIO::GSound jump;
	jump.Create(jumpFX.c_str(), _audioEngine, 0.15f); // we need a global music & sfx volumes
	GW::AUDIO::GSound land;
	land.Create(landFX.c_str(), _audioEngine, 0.15f);
	GW::AUDIO::GSound reset;
	reset.Create(resetFX.c_str(), _audioEngine, 0.15f);
	// add jumpSFX prefab to ECS
	auto jumpPrefab = _game->prefab()
	.set<GW::AUDIO::GSound>(jump.Relinquish());
	// register this prefab by name so other systems can use it
	RegisterPrefab("JumpSFX", jumpPrefab);
	//add landSFX prefab to ECS
	auto landPrefab = _game->prefab()
		.set<GW::AUDIO::GSound>(land.Relinquish());
	RegisterPrefab("LandSFX", landPrefab);
	auto resetPrefab = _game->prefab()
		.set<GW::AUDIO::GSound>(reset.Relinquish());
	RegisterPrefab("ResetSFX", resetPrefab);
	return true;
}

bool JK::BulletData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all bullets and their prefabs
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Bullet&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
	});
	_game->defer_end(); // required when removing while iterating!

	// unregister this prefab by name
	UnregisterPrefab("Lazer Bullet");

	return true;
}
