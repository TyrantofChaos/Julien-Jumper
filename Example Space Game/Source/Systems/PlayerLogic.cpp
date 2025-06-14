#include "PlayerLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Gameplay.h"
#include "../Entities/Prefabs.h"
#include "../Events/Playevents.h"
#include "../Components/ModelComponents.h"

using namespace JK; 
using namespace GW::INPUT; // input libs
using namespace GW::AUDIO; // audio libs

// Connects logic to traverse any players and allow a controller to manipulate them
bool JK::PlayerLogic::Init(	std::shared_ptr<flecs::world> _game, std::weak_ptr<const GameConfig> _gameConfig, 
							GW::INPUT::GInput _immediateInput, GW::INPUT::GBufferedInput _bufferedInput, 
							GW::INPUT::GController _controllerInput, GW::AUDIO::GAudio _audioEngine,
							GW::CORE::GEventGenerator _eventPusher)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	immediateInput = _immediateInput;
	bufferedInput = _bufferedInput;
	controllerInput =	_controllerInput;
	audioEngine = _audioEngine;

	// Init any helper systems required for this task
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	int width = (*readCfg).at("Window").at("width").as<int>();
	float speed = (*readCfg).at("Player1").at("speed").as<float>();
	/*float mass = (*readCfg).at("Player1").at("mass").as<float>();*/
	//chargeTime = (*readCfg).at("Player1").at("chargeTime").as<float>();
	// add logic for updating players
	playerSystem = game->system<Player, EntityMeshData, ControllerID>("Player System")
		// THIS USED MASS AS A CAPTURE - I removed it (Liam)
		.iter([this, speed](flecs::iter it, Player*, EntityMeshData* m, ControllerID* c) {

		for (auto i : it) {
			//Players original position for all levels is 0,1 
			GW::MATH2D::GVECTOR2F originalPos = { 0, 1 };
			// left-right movement
			float xaxis = 0, yaxis = 0, input = 0;
			// Use the controller/keyboard to move the player around the screen			
			if (c[i].index == 0) { // enable keyboard controls for player 1

				//check to see if the player has the tag StopMovingLeft or StopMovingRight
				if (it.entity(i).has<StopMovingLeft>())
				{
					//dont move left anymore
				}
				else
				{
					immediateInput.GetState(G_KEY_LEFT, input); xaxis -= input;
				}
				if (it.entity(i).has<StopMovingRight>())
				{
					//dont move right anymore
				}
				else
				{
					immediateInput.GetState(G_KEY_RIGHT, input); xaxis += input;
				}
				//std::cout << "Left Input: " << input << std::endl;
				//std::cout << "Right Input: " << input << std::endl;
				//immediateInput.GetState(G_KEY_UP,	input); yaxis += input;
				//std::cout << "Up Input: " << input << std::endl;
				//immediateInput.GetState(G_KEY_DOWN, input); yaxis -= input;
				//std::cout << "Down Input: " << input << std::endl;
			
			}
			// grab left-thumb stick
			controllerInput.GetState(c[i].index, G_LX_AXIS,			input); xaxis += input;
			controllerInput.GetState(c[i].index, G_DPAD_LEFT_BTN,	input); xaxis -= input;
			controllerInput.GetState(c[i].index, G_DPAD_RIGHT_BTN,	input); xaxis += input;
			xaxis = G_LARGER(xaxis, -1);// cap right motion
			xaxis = G_SMALLER(xaxis, 1);// cap left motion

			// apply movement
			//p[i].value.x += xaxis * it.delta_time() * speed;
			//p[i].value.y += yaxis * it.delta_time() * speed;
			m->worldMatrix.row4.z += xaxis * it.delta_time() * speed;
			m->worldMatrix.row4.y += yaxis * it.delta_time() * speed;
			//// limit the player to stay within -1 to +1 NDC
			//p[i].value.x = G_LARGER(p[i].value.x, -0.8f);
			//p[i].value.x = G_SMALLER(p[i].value.x, +0.8f);
			//p[i].value.y = G_LARGER(p[i].value.y, -0.8f);
			//p[i].value.y = G_SMALLER(p[i].value.y, +0.8f);

			// fire weapon if they are in a firing state
			//if (it.entity(i).has<Firing>()) {
			//	Position offset = p[i];
			//	offset.value.y += 0.05f;
			//	FireLasers(it.world(), offset);
			//	it.entity(i).remove<Firing>();
			//}
		
			//Make player jump if they are in a jumping state
			if (it.entity(i).has<Jumping>() && jumping)
			{
				
				it.entity(i).remove<CollidedWith>(flecs::Wildcard);
				it.entity(i).remove<StopMovingLeft>();
				it.entity(i).remove <StopMovingRight>();
				it.entity(i).remove<Grounded>();
				jumping = Jump(it.world(), m->worldMatrix.row4.y, jumping, it.entity(i));
				landSound = false;
			}

			//is jumping done? yes then stop jumping and apply gravity 
			if (!jumping)
			{
				it.entity(i).remove<Jumping>();
				it.entity(i).remove<CollidedWith>(flecs::Wildcard);
				//psudo gravity change this value to make the model fall faster
				m->worldMatrix.row4.y -= 0.035f;
				
			}
			//Collider check
			if (it.entity(i).has<CollidedWith>(flecs::Wildcard))
			{
				//does the entity have the tag StopMovingLeft or StopMoving right?
				if (it.entity(i).has<StopMovingLeft>() || it.entity(i).has<StopMovingRight>())
				{
					//dont counteract gravity. because it collided with a wall
				}
				else
				{
					it.entity(i).add<Grounded>();
					//does it collide with something? counteract gravity
					m->worldMatrix.row4.y += 0.035f;
					jumpNumber = 0;
					if (!landSound && !jumpSound)
					{
						flecs::entity land;
						RetreivePrefab("LandSFX", land);
						GW::AUDIO::GSound landing = *land.get<GW::AUDIO::GSound>();
						landing.Play();
						landSound = true;
					}
				}
				
			}
			//reset check
			if (it.entity(i).has<Reset>())
			{

				m->worldMatrix.row4.z = originalPos.x;
				m->worldMatrix.row4.y = originalPos.y;
				it.entity(i).remove <Reset>();
				flecs::entity reset;
				RetreivePrefab("ResetSFX", reset);
				GW::AUDIO::GSound resetting = *reset.get<GW::AUDIO::GSound>();
				resetting.Play();
			}

		}
		// process any cached button events after the loop (happens multiple times per frame)
		ProcessInputEvents(it.world());
	});

	// Create an event cache for when the spacebar/'A' button is pressed
	pressEvents.Create(Max_Frame_Events); // even 32 is probably overkill for one frame
		
	// register for keyboard and controller events
	bufferedInput.Register(pressEvents);
	controllerInput.Register(pressEvents);

	// create the on explode handler
	onExplode.Create([this](const GW::GEvent& e) 
	{
		JK::PLAY_EVENT event; JK::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData)) 
		{
			// only in here if event matches
			std::cout << "Enemy Was Destroyed!\n";
		}
	});
	_eventPusher.Register(onExplode);

	return true;
}

// Free any resources used to run this system
bool JK::PlayerLogic::Shutdown()
{
	playerSystem.destruct();
	game.reset();
	gameConfig.reset();

	return true;
}

// Toggle if a system's Logic is actively running
bool JK::PlayerLogic::Activate(bool runSystem)
{
	if (playerSystem.is_alive()) 
	{
		(runSystem) ? playerSystem.enable() : playerSystem.disable();
		return true;
	}
	return false;
}

bool JK::PlayerLogic::ProcessInputEvents(flecs::world& stage)
{
	// pull any waiting events from the event cache and process them
	GW::GEvent event;
	while (+pressEvents.Pop(event)) {
		bool fire = false;
		GController::Events controller;
		GController::EVENT_DATA c_data;
		GBufferedInput::Events keyboard;
		GBufferedInput::EVENT_DATA k_data;
		// these will only happen when needed
		if (+event.Read(keyboard, k_data)) 
		{
			if (keyboard == GBufferedInput::Events::KEYPRESSED) 
			{
				if (k_data.data == G_KEY_SPACE) 
				{
					flecs::entity e = stage.entity("Lemur_LemurMeshData");
					//fire = true;
					if (!jumping && e.has<Grounded>())
					{
						jumping = true;
						std::cout << "Charging jump" << std::endl;
						chargeStart = stage.time();
					}
					
				}
			}
			//if (keyboard == GBufferedInput::Events::KEYRELEASED) 
			//{
			//	if (k_data.data == G_KEY_SPACE) 
			//	{
			//		chargeEnd = stage.time();
			//		if (chargeEnd - chargeStart >= chargeTime) 
			//		{
			//			//fire = true;
			//			jumping = true;
			//			std::cout << "is charged" << std::endl;
			//			std::cout << "jumping enabled" << std::endl;
			//		}
			//	}
			//}
		}
		else if (+event.Read(controller, c_data)) 
		{
			if (controller == GController::Events::CONTROLLERBUTTONVALUECHANGED) 
			{
				if (c_data.inputValue > 0 && c_data.inputCode == G_SOUTH_BTN)
					jumping = true;
					//fire = true;
				//std::cout << "jumping enabled" << std::endl;
			}
		}
		//if (fire) {
		//	// grab player one and set them to a firing state
		//	stage.entity("Player One").add<Firing>();
		//}
		if (jumping) 
		{
			//grab player one and set them to jumping state
			stage.entity("Lemur_LemurMeshData").add<Jumping>();
		}
	}
	return true;
}

// play sound and launch two laser rounds
//bool ESG::PlayerLogic::FireLasers(flecs::world& stage, Position origin)
//{
//	// Grab the prefab for a laser round
//	flecs::entity bullet;
//	RetreivePrefab("Lazer Bullet", bullet);
//
//	origin.value.x -= 0.05f;
//	auto laserLeft = stage.entity().is_a(bullet).set<Position>(origin);
//	origin.value.x += 0.1f;
//	auto laserRight = stage.entity().is_a(bullet).set<Position>(origin);
//	// if this shot is charged
//	if (chargeEnd - chargeStart >= chargeTime) {
//		chargeEnd = chargeStart;
//		laserLeft.set<ChargedShot>({ 2 })
//			.set<Material>({1,0,0});
//		laserRight.set<ChargedShot>({ 2 })
//			.set<Material>({ 1,0,0 });
//	}
//
//	// play the sound of the Lazer prefab
//	GW::AUDIO::GSound shoot = *bullet.get<GW::AUDIO::GSound>();
//	shoot.Play();
//
//	return true;
//}

//simple lerp function
float lerp(float a, float b, float t)
{
	return a + t * (b - a);
}
//play sound and make player jump
bool JK::PlayerLogic::Jump(flecs::world& stage, float& yPosition, bool jumping, flecs::entity e)
{
	//load prefab of jump sound

	const float jumpHeight = 0.4f; // Adjust as needed
	const float jumpDuration = 0.5f; // Adjust as needed

	static float jumpStartTime = 0.0f;

	if (jumping)
	{
		if (!jumpSound)
		{
			flecs::entity jumping;
			RetreivePrefab("JumpSFX", jumping);
			GW::AUDIO::GSound shoot = *jumping.get<GW::AUDIO::GSound>();
			shoot.Play();
			jumpSound = true;
		}
		// Initialize jump start time
		if (!jumpStartTime)
		{
			jumpStartTime = stage.time();
		}
	
		
		// Calculate jump progress
		float elapsed = stage.time() - jumpStartTime;
		float jumpProgress = (elapsed < jumpDuration) ? (elapsed / jumpDuration) : 1.0f;

		//check to see if player has tag<StopMovingUP> to stop the player from moving up
		if (e.has <StopMovingUP>())
		{
			yPosition -= 0.05;
		}
		else
		{
			// Apply linear interpolation to Y-axis position
			yPosition = lerp(yPosition, yPosition + jumpHeight, jumpProgress);
		}

		// Check if the jump is complete
		if (jumpProgress >= 0.5f)
		{
			jumpNumber++;
			jumping = false;
			jumpSound = false;
			jumpStartTime = 0.0f; // Reset jump start time
			std::cout << "Jump complete" << std::endl;
		}
	}
	else
	{
		jumpStartTime = 0.0f; // Reset jump start time if not jumping
	}

	return jumping;
}
