#include "WinSystem.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
using namespace JK;

bool JK::WinSystem::Init(std::shared_ptr<flecs::world> _game, GW::INPUT::GBufferedInput _bufferedInput, flecs::system& _playerSystem, GW::INPUT::GController _controllerInput) {

	playerSystem = _playerSystem;
	game = _game;
	buffInput = _bufferedInput;
	auto ent = game->entity("WinMenuSystem").add<WinMenu>();
	ent.set<WinBool>({ false, false });

	static constexpr unsigned int test = 32;
	pressEvents.Create(test);

	// creating win menu system
	winSystem = game->system<WinMenu, WinBool>().each([this](WinMenu& w, WinBool& wb)
		{
			if (wb.resetGame)
			{
				wb.resetGame = false;
				wb.winBool = false;
			}
			// if we won the game
			if (wb.winBool)
			{
				// if the player system has been created and it currently is enabled
				if (playerSystem.is_alive() && playerSystem.enabled())
				{
					// disable system so we have no control over player in win menu
					playerSystem.disable();
				}

				ProcessInputEvents(wb, *game.get());
			}
		});

	buffInput.Register(pressEvents);
	_controllerInput.Register(pressEvents);
	return true;
}

bool JK::WinSystem::ProcessInputEvents(WinBool winBool, flecs::world& stage)
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
				if (k_data.data == G_KEY_R)
				{
					auto f = stage.filter<WinMenu, WinBool>();
					f.each([](WinMenu& w, WinBool& wb)
						{
							// when we press R, change win bool values
							wb.resetGame = true;
							wb.winBool = false;
						});
					
					auto p = stage.filter<Player>();
					p.each([](flecs::entity e, Player& p)
						{
							// add the Reset tag to the player (this triggers the reset stage)
							e.add<Reset>();
						});

					// re-enabling player system
					if (playerSystem.is_alive())
					{
						playerSystem.enable();
					}
				}
			}

		}
		else if (+event.Read(controller, c_data))
		{
			if (controller == GW::INPUT::GController::Events::CONTROLLERBUTTONVALUECHANGED)
			{
				if (c_data.inputValue > 0 && c_data.inputCode == G_EAST_BTN)
				{
					auto f = stage.filter<WinMenu, WinBool>();
					f.each([](WinMenu& w, WinBool& wb)
						{
							// when we press R, change win bool values
							wb.resetGame = true;
							wb.winBool = false;
						});

					auto p = stage.filter<Player>();
					p.each([](flecs::entity e, Player& p)
						{
							// add the Reset tag to the player (this triggers the reset stage)
							e.add<Reset>();
						});

					// re-enabling player system
					if (playerSystem.is_alive())
					{
						playerSystem.enable();
					}
				}
			}
		}
	}
	return true;
}

