// define all ECS components related to gameplay
#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// example space game (avoid name collisions)
namespace JK
{
	struct Damage { int value; };
	struct Health { int value; };
	struct Firerate { float value; };
	struct Cooldown { float value; };
	
	// gameplay tags (states)
	struct Firing {};
	struct Charging {};
	struct Jumping {};
	struct Pause {};
	struct PauseBool { bool paused; };
	struct StartMenu {};
	struct StartBool { bool startMenu; };
	struct WinMenu {};
	struct WinBool {
		bool winBool;
		bool resetGame; 
	};


	struct Grounded {};
  
	// powerups
	struct ChargedShot { int max_destroy; };
};

#endif