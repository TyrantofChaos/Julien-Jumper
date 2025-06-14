#pragma once
#include "../Components/Gameplay.h"
namespace JK {

	class StartSystem {
		GW::CORE::GEventCache  pressEvents;
		flecs::system playerSystem;
		flecs::system startSystem;
		std::shared_ptr<flecs::world> game;
		GW::INPUT::GBufferedInput buffInput;
	public:
		bool Init(std::shared_ptr<flecs::world> _game, GW::INPUT::GBufferedInput _bufferedInput, flecs::system& player, GW::INPUT::GController _controllerInput);
		bool ProcessInputEvents(StartBool startBool, flecs::world& stage);
	};
}
