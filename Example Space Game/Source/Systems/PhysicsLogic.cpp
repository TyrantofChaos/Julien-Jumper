#include "PhysicsLogic.h"
#include "../Components/Physics.h"

bool JK::PhysicsLogic::Init(	std::shared_ptr<flecs::world> _game, 
								std::weak_ptr<const GameConfig> _gameConfig)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	// **** MOVEMENT ****
	// update velocity by acceleration
	game->system<Velocity, const Acceleration>("Acceleration System")
		.each([](flecs::entity e, Velocity& v, const Acceleration &a) {
		GW::MATH2D::GVECTOR2F accel;
		GW::MATH2D::GVector2D::Scale2F(a.value, e.delta_time(), accel);
		GW::MATH2D::GVector2D::Add2F(accel, v.value, v.value);
	});
	// update position by velocity
	game->system<Position, const Velocity>("Translation System")
		.each([](flecs::entity e, Position& p, const Velocity &v) {
		GW::MATH2D::GVECTOR2F speed;
		GW::MATH2D::GVector2D::Scale2F(v.value, e.delta_time(), speed);
		// adding is simple but doesn't account for orientation
		GW::MATH2D::GVector2D::Add2F(speed, p.value, p.value);
	});
	// **** CLEANUP ****
	// clean up any objects that end up offscreen
	game->system<const Position>("Cleanup System")
		.each([](flecs::entity e, const Position& p) {
		if (p.value.x > 1.5f || p.value.x < -1.5f ||
			p.value.y > 1.5f || p.value.y < -1.5f) {
				e.destruct();
		}
	});
	// **** COLLISIONS ****
	// due to wanting to loop through all collidables at once, we do this in two steps:
	// 1. A System will gather all collidables into a shared std::vector
	// 2. A second system will run after, testing/resolving all collidables against each other
	queryCache = game->query<Collidable, EntityBoundary, EntityMeshData>();
	// only happens once per frame at the very start of the frame
	struct CollisionSystem {}; // local definition so we control iteration count (singular)
	game->entity("Detect-Collisions").add<CollisionSystem>();
	game->system<CollisionSystem>()
		.each([this](CollisionSystem& s) {
		// collect any and all collidable objects
		queryCache.each([this](flecs::entity e, Collidable& c, EntityBoundary& boundary, EntityMeshData& o) {

			//create a vector3 that has the translation data for each entity
			GW::MATH2D::GVECTOR3F translation = { o.worldMatrix.row4.x,o.worldMatrix.row4.y,o.worldMatrix.row4.z };

			SHAPE boundingBox; // compute buffer for this objects polygon
			// This is critical, if you want to store an entity handle it must be mutable
			boundingBox.owner = e; // allows later changes

			//the bounding boxes need a base point so translate the first values
			GW::MATH2D::GVECTOR3F base = { boundary.boundary->x + translation.z, boundary.boundary->y + translation.y };
			boundingBox.rect.min = { base.x, base.y };
			boundingBox.rect.max = { base.x,base.y };
			//
			for(int i = 0; i < 8; i++)
			{
				//transform the bounding box with the translation vector
				GW::MATH2D::GVECTOR3F v = { boundary.boundary[i].x + translation.z, boundary.boundary[i].y + translation.y,boundary.boundary[i].z + translation.x};
				if (e.name() == "Lemur_LemurMeshData")
				{
					//need to change these values. the bounding box on the player is to big (possibly in the txt file not here)
					if (v.x < boundingBox.rect.min.x || v.y < boundingBox.rect.min.y || i == 0)
					{
						boundingBox.rect.min = { v.x, v.y };
					}
					if (v.x > boundingBox.rect.max.x || v.y > boundingBox.rect.max.y || i == 0)
					{
						boundingBox.rect.max = { v.x, v.y };
					}
				}
				else
				{
					//check for the min and max of the bounding boxes
					if (v.x < boundingBox.rect.min.x || v.y < boundingBox.rect.min.y || i == 0)
					{
						boundingBox.rect.min = { v.x, v.y };
					}
					if (v.x > boundingBox.rect.max.x || v.y > boundingBox.rect.max.y || i == 0)
					{
						boundingBox.rect.max = { v.x, v.y };
					}
				}
			}
			// add to vector
			testCache.push_back(boundingBox);
			});
			// loop through the testCahe resolving all collisions
			for (int i = 0; i < testCache.size(); ++i) {
				// the inner loop starts at the entity after you so you don't double check collisions
				for (int j = i + 1; j < testCache.size(); ++j) {

					// test the two world space polygons for collision
					// possibly make this cheaper by leaving one of them local and using an inverse matrix
					GW::MATH2D::GCollision2D::GCollisionCheck2D result;
					//only check collison if it is with the player we dont need to have blocks checking with other blocks
					if (testCache[i].owner.name() == "Lemur_LemurMeshData" || testCache[j].owner.name() == "Lemur_LemurMeshData")
					{
						//since we have rectangles we can use TestRectangleToRectangle
						GW::MATH2D::GCollision2D::TestRectangleToRectangle2F(testCache[i].rect, testCache[j].rect, result);
						//add bools to check if the player collider is colliding with the top, left, right, and top of another
						//collider
						bool bottomCollidesWithTop = false;
						bool leftCollide = false;
						bool rightCollide = false;
						bool topCollidesWithBottom = false;
						//Check if the player is colliding with the top,bottom, left, or right of another collider
						if (testCache[i].owner.name() == "Lemur_LemurMeshData")
						{
							bottomCollidesWithTop = testCache[j].rect.max.y >= testCache[i].rect.min.y;
							leftCollide = testCache[i].rect.min.x <= testCache[j].rect.max.x && testCache[i].rect.max.x >= testCache[j].rect.max.x;
							rightCollide = testCache[i].rect.max.x >= testCache[j].rect.min.x && testCache[i].rect.min.x <= testCache[j].rect.min.x;
							//topCollidesWithBottom = testCache[j].rect.min.y >= testCache[i].rect.max.y;
						}
						else if (testCache[j].owner.name() == "Lemur_LemurMeshData")
						{
							bottomCollidesWithTop = testCache[i].rect.max.y >= testCache[j].rect.min.y;
							leftCollide = testCache[j].rect.min.x <= testCache[i].rect.max.x && testCache[j].rect.max.x >= testCache[i].rect.max.x;
							rightCollide = testCache[j].rect.max.x >= testCache[i].rect.min.x && testCache[j].rect.min.x <= testCache[i].rect.min.x;
							//topCollidesWithBottom = testCache[i].rect.min.y >= testCache[i].rect.max.y;
						}
						bool xCollision = leftCollide || rightCollide;
						//if its not colliding with another collider on the x-axis give the player movement back 
						if (!xCollision)
						{
							testCache[i].owner.remove<StopMovingLeft>();
							testCache[j].owner.remove<StopMovingLeft>();
							testCache[i].owner.remove<StopMovingRight>();
							testCache[j].owner.remove<StopMovingRight>();
						}
 						if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION && bottomCollidesWithTop) {
							// Create an ECS relationship between the colliders
							// Each system can decide how to respond to this info independently
							bool isPlayerDead = testCache[i].owner.name() == "Lemur_LemurMeshData" && testCache[j].owner.name() == "DeathBox" 
								|| testCache[j].owner.name() == "Lemur_LemurMeshData" && testCache[i].owner.name() == "DeathBox";
							if (isPlayerDead)
							{
								//just add reset to both entities 1 of them is the player one is the deathbox. 
								//player gets rid of reset tag when it resets 
								testCache[i].owner.add<Reset>();
								testCache[j].owner.add<Reset>();
							}
							if (xCollision)
							{
								if (leftCollide)
								{
									if (testCache[i].owner.name() == "blockLarge.003" || testCache[j].owner.name() == "blockLarge.003"|| 
										testCache[i].owner.name() == "blockLarge.002" || testCache[j].owner.name() == "blockLarge.002" || testCache[i].owner.name() == "blockLarge.001" 
										|| testCache[j].owner.name() == "blockLarge.001" || testCache[i].owner.name() == "treePine" || testCache[j].owner.name() == "treePine"|| 
										testCache[i].owner.name() == "blockLarge.007" || testCache[j].owner.name() == "blockLarge.007"|| testCache[i].owner.name() == "blockLarge.008" || 
										testCache[j].owner.name() == "blockLarge.008"|| testCache[i].owner.name() == "blockLarge.009" || testCache[j].owner.name() == "blockLarge.009" || 
										testCache[i].owner.name() == "teePine.001" || testCache[j].owner.name() == "treePine.001")
									{
										//do nothing because thats one of the starting blocks that are put together
									}
									else
									{
										testCache[i].owner.add<StopMovingLeft>();
										testCache[j].owner.add<StopMovingLeft>();
									}
									
								}
								else if (rightCollide)
								{
									if (testCache[i].owner.name() == "blockLarge.003" || testCache[j].owner.name() == "blockLarge.003" || testCache[i].owner.name() == "blockLarge.002" || 
										testCache[j].owner.name() == "blockLarge.002" || testCache[i].owner.name() == "blockLarge.001" || testCache[j].owner.name() == "blockLarge.001"|| 
										testCache[i].owner.name() == "treePine" 
										|| testCache[j].owner.name() == "treePine" || testCache[i].owner.name() == "blockLarge.007" || testCache[j].owner.name() == "blockLarge.007" || testCache[i].owner.name() == "blockLarge.008" 
										|| testCache[j].owner.name() == "blockLarge.008" || 
										 testCache[i].owner.name() == "teePine.001" || testCache[j].owner.name() == "treePine.001")
									{
										//do nothing because thats one of the starting blocks that are put together
									}
									else
									{
										testCache[i].owner.add<StopMovingRight>();
										testCache[j].owner.add<StopMovingRight>();
									}
								}
							}
							//if (topCollidesWithBottom)
							//{
							//	std::cout << "right Collide" << std::endl;
							//	std::cout << "Colliding Rectangles - i " << testCache[i].owner.name() << ": " << " min y " << testCache[i].rect.min.y << ", "
							//		<< "max y " << testCache[i].rect.max.y << std::endl;
							//
							//	std::cout << "Colliding Rectangles - j " << testCache[j].owner.name() << ": " << " min y " << testCache[j].rect.min.y << ", "
							//		<< "max y " << testCache[j].rect.max.y << std::endl;
							//	testCache[i].owner.add<StopMovingUP>();
							//	testCache[j].owner.add<StopMovingUP>();
							//}
							testCache[j].owner.add<CollidedWith>(testCache[i].owner);
							testCache[i].owner.add<CollidedWith>(testCache[j].owner);
							//print out which objects are colliding with each other and their respective values
							//std::cout << "Colliding Rectangles - i " << testCache[i].owner.name() << ": " << testCache[i].rect.min.x << ", " << testCache[i].rect.min.y
							//	<< " to " << testCache[i].rect.max.x << ", " << testCache[i].rect.max.y << std::endl;
							//
							//std::cout << "Colliding Rectangles - j " << testCache[j].owner.name() << ": " << testCache[j].rect.min.x << ", " << testCache[j].rect.min.y
							//	<< " to " << testCache[j].rect.max.x << ", " << testCache[j].rect.max.y << std::endl;
						}
					}
					//will need to add a check to see if the top of the bounding box of the Lemur is what is being collided with
				}
			}
			// wipe the test cache for the next frame (keeps capacity intact)
			testCache.clear();
			});
		return true;
}


bool JK::PhysicsLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Acceleration System").enable();
		game->entity("Translation System").enable();
		game->entity("Cleanup System").enable();
	}
	else {
		game->entity("Acceleration System").disable();
		game->entity("Translation System").disable();
		game->entity("Cleanup System").disable();
	}
	return true;
}

bool JK::PhysicsLogic::Shutdown()
{
	queryCache.destruct(); // fixes crash on shutdown
	game->entity("Acceleration System").destruct();
	game->entity("Translation System").destruct();
	game->entity("Cleanup System").destruct();
	return true;
}
