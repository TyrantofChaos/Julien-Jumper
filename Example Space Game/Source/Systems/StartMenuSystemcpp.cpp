#include "StartMenuSystem.h"
using namespace JK;


bool JK::StartSystem::Init(std::shared_ptr<flecs::world> _game, GW::INPUT::GBufferedInput _bufferedInput, flecs::system& _playerSystem, GW::INPUT::GController _controllerInput) {
	GW::INPUT::GBufferedInput::Events keyboard;
	GW::INPUT::GBufferedInput::EVENT_DATA k_data;

	playerSystem = _playerSystem;
	game = _game;
	buffInput = _bufferedInput;
	auto ent = game->entity("StartMenuSystem").add<StartMenu>();
	ent.set<StartBool>({ true });

	static constexpr unsigned int test = 32;
	pressEvents.Create(test);
	
	// creating start menu system
	startSystem = game->system<StartMenu, StartBool>().each([this, keyboard, k_data](StartMenu& s, StartBool& startBool)
		{
			// if we are in the start menu
			if (startBool.startMenu)
			{
				// if the player system has been created and it currently is enabled
				if (playerSystem.is_alive() && playerSystem.enabled())
				{
					// disable system so we have no control over player in start menu
					playerSystem.disable();
				}
			}

			ProcessInputEvents(startBool, *game.get());
		});

	buffInput.Register(pressEvents);
	_controllerInput.Register(pressEvents);
	return true;

}

bool JK::StartSystem::ProcessInputEvents(StartBool startBool, flecs::world& stage)
{
	// pull any waiting events from the event cache and process them
	GW::GEvent event;

	while (+pressEvents.Pop(event))
	{
		GW::INPUT::GController::Events controller;
		GW::INPUT::GController::EVENT_DATA c_data;
		GW::INPUT::GBufferedInput::Events keyboard;
		GW::INPUT::GBufferedInput::EVENT_DATA k_data;

		// these will only happen when needed
		if (+event.Read(keyboard, k_data))
		{
			if (keyboard == GW::INPUT::GBufferedInput::Events::KEYPRESSED)
			{
				if (k_data.data == G_KEY_ENTER)
				{
					auto f = stage.filter<StartMenu, StartBool>();
					f.each([](StartMenu& s, StartBool& sb)
						{
							// when we press enter, we are no longer in start menu
							sb.startMenu = false;	
						});

						// re-enabling player system
						if (playerSystem.is_alive()) 
						{
							playerSystem.enable();
						}

						std::cout << "Start System Destroyed";

						// destroy start system because we no longer need it running
						startSystem.destruct();

				}
			}

		}
		else if (+event.Read(controller, c_data))
		{
			if (controller == GW::INPUT::GController::Events::CONTROLLERBUTTONVALUECHANGED)
			{
				if (c_data.inputValue > 0 && c_data.inputCode == G_START_BTN)
				{
					auto f = stage.filter<StartMenu, StartBool>();
					f.each([](StartMenu& s, StartBool& sb)
						{
							// when we press enter, we are no longer in start menu
							sb.startMenu = false;
						});

					// re-enabling player system
					if (playerSystem.is_alive())
					{
						playerSystem.enable();
					}

					std::cout << "Start System Destroyed";

					// destroy start system because we no longer need it running
					startSystem.destruct();

				}

			}
		}
	}
	return true;
}



