#pragma once
#include "../Components/Gameplay.h"
namespace JK {
	
	class PauseSystem {
		GW::AUDIO::GMusic currentTrack;
		GW::CORE::GEventCache  pressEvents;
		flecs::system playerSystem;
		flecs::system playSystem;
		flecs::system pSystem;
		std::shared_ptr<flecs::world> game;
		GW::INPUT::GBufferedInput buffInput;
		bool musicPlaying = true;
	public:
		bool Init(std::shared_ptr<flecs::world> _game, GW::INPUT::GBufferedInput _bufferedInput,flecs::system &player,GW::AUDIO::GMusic &_currentTrack, GW::INPUT::GController _controllerInput);
		bool ProcessInputPauseEvents(PauseBool pauseState, flecs::world& stage);
		// bool paused = false;
	};
}