#include "PauseSystem.h"
#include "PlayerLogic.h"
using namespace JK;
bool JK::PauseSystem::Init(std::shared_ptr<flecs::world> _game, GW::INPUT::GBufferedInput _bufferedInput,flecs::system &_playerSystem, GW::AUDIO::GMusic& _currentTrack, GW::INPUT::GController _controllerInput) {
	GW::INPUT::GBufferedInput::Events keyboard;
	GW::INPUT::GBufferedInput::EVENT_DATA k_data;
	
	playerSystem = _playerSystem;
	game = _game;
	buffInput = _bufferedInput;
	currentTrack = _currentTrack;
	auto ent = game->entity("PauseSystem").add<Pause>();
	ent.set<PauseBool>({ false });

	static constexpr unsigned int test = 32;
	pressEvents.Create(test);
	pSystem = game->system<Pause, PauseBool>().each([this,keyboard,k_data](Pause& p, PauseBool& pauseState)
	{
	/*	float input = 0;
		buffInput.GetState(G_KEY_ESCAPE, input);
		bool musicPlaying = false;*/

		//if (keyboard == GW::INPUT::GBufferedInput::Events::KEYPRESSED) {
		//	if (k_data.data == G_KEY_ESCAPE) {
		//		paused = !paused;
		//		std::cout << "Escape Was Pressed";
		//		//Test();
		//		if (paused) {

		//			if (playerSystem.is_alive()) {
		//				playerSystem.disable();
		//			}
		//			else {

		//			}
		//		
		//		}
		//		else {
		//			playerSystem.enable();
		//		}
		//	}
		//}
		//	
		//if (input != 0) {
		//	paused = !paused;
		//	std::cout << "Escape Was Pressed";
		//	//Test();
		//	currentTrack.isPlaying(musicPlaying);
		//	if (paused) {

		//		if (musicPlaying) {
		//			currentTrack.Pause();
		//		}
		//		if (playerSystem.is_alive()) {

		//			playerSystem.disable();
		//		}
		//	}
		//
		//	else {
		//		playerSystem.enable();
		//		if (!musicPlaying) {
		//			currentTrack.Resume();
		//		}
		//	}
		//}
			ProcessInputPauseEvents(pauseState, *game.get());

		
	});
	
	buffInput.Register(pressEvents);
	_controllerInput.Register(pressEvents);
	//playSystem = game->system<Pause>().each([this](Pause& p) {
	//	float input = 0;
	//	ourInput.GetState(G_KEY_P, input);
	//	if (input != 0) {
	//		playerSystem.enable();
	//		std::cout << "p is pressed";
	//	}
	//	});
	return true;

}

bool JK::PauseSystem::ProcessInputPauseEvents(PauseBool pauseState, flecs::world& stage)
{
	// pull any waiting events from the event cache and process them
	GW::GEvent event;

	while (+pressEvents.Pop(event)) 
	{

		bool winState = false;

		GW::INPUT::GController::Events controller;
		GW::INPUT::GController::EVENT_DATA c_data;
		GW::INPUT::GBufferedInput::Events keyboard;
		GW::INPUT::GBufferedInput::EVENT_DATA k_data;
		currentTrack.isPlaying(musicPlaying);
		// these will only happen when needed
		if (+event.Read(keyboard, k_data))
		{
			auto f = stage.filter<WinMenu, WinBool>();
			f.each([&winState](WinMenu& w, WinBool& wb)
				{
					winState = wb.winBool;
				});

			if (keyboard == GW::INPUT::GBufferedInput::Events::KEYPRESSED)
			{
				if (k_data.data == G_KEY_ESCAPE && !winState)
				{
					auto f = stage.filter<Pause, PauseBool>();
					f.each([](Pause& p, PauseBool& pb)
						{
							pb.paused = !pb.paused;
						});

					bool pausedState = !pauseState.paused;
					std::cout << "Escape Was Pressed";
					
					if (pausedState) {

						if (playerSystem.is_alive()) {
						playerSystem.disable();
						
						}
						if (musicPlaying) {
							currentTrack.Pause();
						}
						else {
							
						}
							
						}
					else {
						playerSystem.enable();
						if (!musicPlaying) {
							currentTrack.Resume();
						}
					}
					
				}
			}
			if (keyboard == GW::INPUT::GBufferedInput::Events::KEYRELEASED)
			{
				if (k_data.data == G_KEY_SPACE)
				{
			
				}
			}
		}
		else if (+event.Read(controller, c_data))
		{
			if (controller == GW::INPUT::GController::Events::CONTROLLERBUTTONVALUECHANGED)
			{
				if (c_data.inputValue > 0 && c_data.inputCode == G_SELECT_BTN)
				{ 
					auto f = stage.filter<Pause, PauseBool>();
					f.each([](Pause& p, PauseBool& pb)
						{
							pb.paused = !pb.paused;
						});

					bool pausedState = !pauseState.paused;
					std::cout << "Select Was Pressed";

					if (pausedState) {

						if (playerSystem.is_alive()) {
							playerSystem.disable();

						}
						if (musicPlaying) {
							currentTrack.Pause();
						}
						else {

						}

					}
					else {
						playerSystem.enable();
						if (!musicPlaying) {
							currentTrack.Resume();
						}
					}

				}
					
			}
		}
	}
	return true;
}



