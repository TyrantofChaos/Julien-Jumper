#include "Model.h"

namespace JK
{
	class Level_Objects
	{

	public:

		// store all our models
		std::list<Model> objectsInLevel;
		std::list<Model> texturedObjectsInLevel;
		std::list<Model> skyBox;

		GW::GReturn gRet;

		// gateware variables
		GW::MATH::GMatrix proxyMatrix;
		GW::MATH::GMATRIXF viewMatrix;
		GW::MATH::GVECTORF cameraPos;
		GW::SYSTEM::GWindow win;
		GW::GRAPHICS::GDirectX11Surface d3d;
		GW::SYSTEM::GLog errorLog;
		GW::INPUT::GInput gInput;

		// Imports the default level txt format and creates a Model from each .h2b
		bool LoadLevel(const char* gameLevelPath,
			const char* h2bFolderPath,
			GW::SYSTEM::GLog log) {

			// What this does:
			// Parse GameLevel.txt 
			// For each model found in the file...
				// Create a new Model class on the stack.
				// Read matrix transform and add to this model.
				// Load all CPU rendering data for this model from .h2b
				// Move the newly found Model to our list of total models for the level 

			log.LogCategorized("EVENT", "LOADING GAME LEVEL [OBJECT ORIENTED]");
			log.LogCategorized("MESSAGE", "Begin Reading Game Level Text File.");

			UnloadLevel();// clear previous level data if there is any
			GW::SYSTEM::GFile file;
			file.Create();
			if (-file.OpenTextRead(gameLevelPath)) {
				log.LogCategorized(
					"ERROR", (std::string("Game level not found: ") + gameLevelPath).c_str());
				return false;
			}
			char linebuffer[1024];
			while (+file.ReadLine(linebuffer, 1024, '\n'))
			{
				// having to have this is a bug, need to have Read/ReadLine return failure at EOF
				if (linebuffer[0] == '\0')
					break;
				if (std::strcmp(linebuffer, "MESH") == 0)
				{
					Model newModel;
					file.ReadLine(linebuffer, 1024, '\n');
					log.LogCategorized("INFO", (std::string("Model Detected: ") + linebuffer).c_str());
					// create the model file name from this (strip the .001)
					newModel.SetName(linebuffer);
					std::string modelFile = linebuffer;
					modelFile = modelFile.substr(0, modelFile.find_last_of("."));
					modelFile += ".h2b";

					// now read the transform data as we will need that regardless
					GW::MATH::GMATRIXF transform;
					for (int i = 0; i < 4; ++i) {
						file.ReadLine(linebuffer, 1024, '\n');
						// read floats
						sscanf_s(linebuffer + 13, "%f, %f, %f, %f",
							&transform.data[0 + i * 4], &transform.data[1 + i * 4],
							&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
					}
					std::string loc = "Location: X ";
					loc += std::to_string(transform.row4.x) + " Y " +
						std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);
					log.LogCategorized("INFO", loc.c_str());

					// *NEW* finally read in the boundry data for this model
					//NOTE: Data might be -90 degrees off on the X axis. Not verified.
					for (int i = 0; i < 8; ++i) {
						file.ReadLine(linebuffer, 1024, '\n');
						// read floats
						std::sscanf(linebuffer + 9, "%f, %f, %f",
							&newModel.boundary[i].x, &newModel.boundary[i].y, &newModel.boundary[i].z);
					}
					std::string bounds = "Boundary: Left ";
					bounds += std::to_string(newModel.boundary[0].x) +
						" Right " + std::to_string(newModel.boundary[4].x) +
						" Bottom " + std::to_string(newModel.boundary[0].y) +
						" Top " + std::to_string(newModel.boundary[1].y) +
						" Near " + std::to_string(newModel.boundary[0].z) +
						" Far " + std::to_string(newModel.boundary[2].z);

					log.LogCategorized("INFO", bounds.c_str());

					// Add new model to list of all Models
					log.LogCategorized("MESSAGE", "Begin Importing .H2B File Data.");
					modelFile = std::string(h2bFolderPath) + "/" + modelFile;
					newModel.SetWorldMatrix(transform);
					// If we find and load it add it to the level
					if (newModel.LoadModelDataFromDisk(modelFile.c_str())) {

						// if map_Kd is NOT null, that means this model has a texture map, that means it is a textured item
						// you can also check for map_Ks so specular map, but our models don't have that currently, but I'm going to add the check anyways
						for (int i = 0; i < newModel.cpuModel.materialCount; i++)
						{
							// if model is skybox, add it to its own list - this is so we can draw it FIRST
							if (newModel.name == "SkyBox")
							{
								newModel.skyBoxBool = true;
								skyBox.push_back(std::move(newModel));
								break;
							}

							// TODO: Uncomment this if we want to implement texturing
							//else if (newModel.cpuModel.materials[i].map_Kd != nullptr || newModel.cpuModel.materials[i].map_Ks != nullptr)
							//{
							//	// add this model to texturedObjectsInLevel instead of nonTexturedObjectsInLevel
							//	newModel.textured = true;
							//	texturedObjectsInLevel.push_back(std::move(newModel));
							//	break;
							//}

							else
							{
								// add to our non textured level objects, we use std::move since Model::cpuModel is not copy safe.
								objectsInLevel.push_back(std::move(newModel));
								break;
							}
						}

						log.LogCategorized("INFO", (std::string("H2B Imported: ") + modelFile).c_str());
					}
					else {
						// notify user that a model file is missing but continue loading
						log.LogCategorized("ERROR",
							(std::string("H2B Not Found: ") + modelFile).c_str());
						log.LogCategorized("WARNING", "Loading will continue but model(s) are missing.");
					}
					log.LogCategorized("MESSAGE", "Importing of .H2B File Data Complete.");
				}
			}
			log.LogCategorized("MESSAGE", "Game Level File Reading Complete.");
			// level loaded into CPU ram
			log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU [OBJECT ORIENTED]");

			return true;
		}

		// used to wipe CPU & GPU level data between levels
		void UnloadLevel() 
		{
			objectsInLevel.clear();

			texturedObjectsInLevel.clear();

			skyBox.clear();

			CoUninitialize();
		}

	};
};