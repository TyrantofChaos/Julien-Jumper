#include "DirectXRendererLogic.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../DDSTextureLoader.h"
#include "../FileIntoString.h"

using namespace JK; // Example Space Game

// initialize rendering surface
// load shaders
// set up pipeline
// set up render

// FOR DEBUGGING
void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// this method assigns all variables necessary for use in the renderer, and it initializes the pipeline for EACH entity in the level
bool JK::DirectXRendererLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::GRAPHICS::GDirectX11Surface _dx11,
	GW::SYSTEM::GWindow _window)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	directX = _dx11;
	window = _window;

	// getting all direct X items and storing them in respective variables
	directX.GetDevice(&device);
	directX.GetImmediateContext(&devContext);
	directX.GetRenderTargetView(&rendTargetView);
	directX.GetSwapchain(&swapChain);
	directX.GetDepthStencilView(&depthStencView);

	myLevel.LoadLevel("../GameLevels/ProjectAndPortfolioLevels/GameLevel3WithPlayer.txt", "../GameLevels/ProjectAndPortfolioLevels/Models", errorLog);

	//myLevel.LoadLevel(JK::LevelLogic::levels[JK::LevelLogic::levelIndex], "../GameLevels/ProjectAndPortfolioLevels/Models", errorLog);
	//myLevel.LoadLevel("../GameLevels/ProjectAndPortfolioLevels/TestLevelWithPlayer.txt", "../GameLevels/ProjectAndPortfolioLevels/Models", errorLog);
	// looping through list of level models and creating an entity from each one

	if(LoadOriginalViewMatrix() == false)
		return false;

	for (auto& e : myLevel.objectsInLevel)
	{
		// setting certain model values so that I can copy them into an entity
		SetSceneData(e);
		LoadShaders(e);
		
		// copying model values into an entity
		CreateEntityFromModel(e);
	}

	// querying all my level object entities - using this to set each of their pipeline values and assign that value to their components
	auto f = game->filter<EntityName, EntityMeshData, EntitySceneData, EntityBuffers, EntityShaders, EntityInputLayout, EntityBoundary, EntityCPUModelData>();
	f.each([this](EntityName& name, EntityMeshData& meshData, EntitySceneData& sceneData, EntityBuffers& buff, EntityShaders& shaders, EntityInputLayout& inpLayout, EntityBoundary& bounds, EntityCPUModelData& cpu)
		{
			// creating vertex buffer using current entity, and assigning it to current entities vertex buffer variable
			if (CreateVertexBuffer(cpu.vertices.data(), sizeof(H2B::VERTEX) * cpu.vertices.size(), &buff.vertBuffer) == false)
				return false;
			if (CreateConstantBuffer(&meshData, sizeof(EntityMeshData), &buff.meshBuffer) == false)
				return false;
			if (CreateConstantBuffer(&sceneData, sizeof(EntitySceneData), &buff.sceneBuffer) == false)
				return false;
			if (CreateIndexBuffer(cpu.indices.data(), sizeof(unsigned int) * cpu.indices.size(), &buff.indexBuffer) == false)
				return false;

			Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(&shaders.vertexShader);
			Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(&shaders.pixelShader);
			CreateVertexInputLayout(vsBlob, &inpLayout.inputLayout);

			// creating vector of bounding box data
			std::vector<GW::MATH2D::GVECTOR3F> boundaryVector;
			for (int i = 0; i < 8; i++)
			{
				boundaryVector.push_back(bounds.boundary[i]);
			}

			if (debug)
			{
				// creating vertex buffer, input layout, and shaders for bounding box vertices
				if (CreateVertexBuffer(boundaryVector.data(), sizeof(GW::MATH2D::GVECTOR3F) * boundaryVector.size(), &buff.boundingVertBuffer) == false)
					return false;
				vsBlob = CompileBoundingBoxVertexShader(&shaders.boundingBoxVertexShader);
				psBlob = CompileBoundingBoxPixelShader(&shaders.boundingBoxPixelShader);
				if (CreateVertexInputLayout(vsBlob, &inpLayout.boundingBoxInputLayout) == false)
					return false;
			}

#pragma region PRINTING ENTITY DATA
			// TESTING ENTITY DATA
			// testing if entity name is correct
			std::string entName = "\n\nENTITY NAME: " + name.name;
			std::cout << entName;

			// testing if entity world pos is correct
			std::string entWorldPos = "\nENTITY WORLD POSITION: ";
			entWorldPos += "(" + std::to_string(meshData.worldMatrix.row4.x) + ", " + std::to_string(meshData.worldMatrix.row4.y) + ", " + std::to_string(meshData.worldMatrix.row4.z) + ")";
			std::cout << entWorldPos;

			// testing if material data is correct
			std::string entMatData = "\nENTITY MATERIAL DATA:";
			entMatData += "\nKd: (" + std::to_string(meshData.materialInfo.Kd.x) + "," + std::to_string(meshData.materialInfo.Kd.y) + "," + std::to_string(meshData.materialInfo.Kd.z) + ")";
			entMatData += "\nd: " + std::to_string(meshData.materialInfo.d);
			entMatData += "\nKs: (" + std::to_string(meshData.materialInfo.Ks.x) + "," + std::to_string(meshData.materialInfo.Ks.y) + "," + std::to_string(meshData.materialInfo.Ks.z) + ")";
			entMatData += "\nNs: " + std::to_string(meshData.materialInfo.Ns);
			entMatData += "\nKa: (" + std::to_string(meshData.materialInfo.Ka.x) + "," + std::to_string(meshData.materialInfo.Ka.y) + "," + std::to_string(meshData.materialInfo.Ka.z) + ")";
			entMatData += "\nSharpness: " + std::to_string(meshData.materialInfo.sharpness);
			entMatData += "\nTf: (" + std::to_string(meshData.materialInfo.Tf.x) + "," + std::to_string(meshData.materialInfo.Tf.y) + "," + std::to_string(meshData.materialInfo.Tf.z) + ")";
			entMatData += "\nNi: " + std::to_string(meshData.materialInfo.Ni);
			entMatData += "\nKe: (" + std::to_string(meshData.materialInfo.Ke.x) + "," + std::to_string(meshData.materialInfo.Ke.y) + "," + std::to_string(meshData.materialInfo.Ke.z) + ")";
			entMatData += "\nIllum: " + std::to_string(meshData.materialInfo.illum);
			std::cout << entMatData;

			// testing if buffer addresses are correct
			std::string entBuffAddresses = "\nENTITY VERTEX, INDEX, MESH AND SCENE BUFFER ADDRESSES:";
			std::cout << entBuffAddresses;

			const void* temp = buff.vertBuffer.Get();
			std::stringstream vertAddress;
			vertAddress << temp;

			const void* temp2 = buff.indexBuffer.Get();
			std::stringstream indexAddress;
			indexAddress << temp2;

			const void* temp3 = buff.meshBuffer.Get();
			std::stringstream meshAddress;
			meshAddress << temp3;

			const void* temp4 = buff.sceneBuffer.Get();
			std::stringstream sceneAddress;
			sceneAddress << temp4;

			std::cout << "\nVERTEX: " + vertAddress.str();
			std::cout << "\nINDEX: " + indexAddress.str();
			std::cout << "\nMESH: " + meshAddress.str();
			std::cout << "\nSCENE: " + sceneAddress.str();


			// testing if entity bounding data is correct
			std::string entBoundingData = "\nENTITY BOUNDARY DATA: \n";
			entBoundingData += "(" + std::to_string(bounds.boundary[0].x) + ", " + std::to_string(bounds.boundary[0].y) + ", " + std::to_string(bounds.boundary[0].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[1].x) + ", " + std::to_string(bounds.boundary[1].y) + ", " + std::to_string(bounds.boundary[1].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[2].x) + ", " + std::to_string(bounds.boundary[2].y) + ", " + std::to_string(bounds.boundary[2].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[3].x) + ", " + std::to_string(bounds.boundary[3].y) + ", " + std::to_string(bounds.boundary[3].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[4].x) + ", " + std::to_string(bounds.boundary[4].y) + ", " + std::to_string(bounds.boundary[4].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[5].x) + ", " + std::to_string(bounds.boundary[5].y) + ", " + std::to_string(bounds.boundary[5].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[6].x) + ", " + std::to_string(bounds.boundary[6].y) + ", " + std::to_string(bounds.boundary[6].z) + ")" + '\n';
			entBoundingData += "(" + std::to_string(bounds.boundary[7].x) + ", " + std::to_string(bounds.boundary[7].y) + ", " + std::to_string(bounds.boundary[7].z) + ")";

			std::cout << entBoundingData;

			// testing if cpuModel data is correct
			std::string cpuModelMessage = "\nCPU Model Data:";
			std::cout << cpuModelMessage;

			std::string vertexCount = "\nVERTEX COUNT: " + std::to_string(cpu.vertexCount);
			std::string indexCount = "\nINDEX COUNT: " + std::to_string(cpu.indexCount);
			std::string materialCount = "\nMATERIAL COUNT: " + std::to_string(cpu.materialCount);
			std::string meshCount = "\nMESH COUNT: " + std::to_string(cpu.meshCount);
			std::cout << vertexCount + indexCount + materialCount + meshCount;

			std::string firstFiveVerts = "\nFIRST FIVE VERTICES: ";
			firstFiveVerts += "\nPOS: (" + std::to_string(cpu.vertices[0].pos.x) + ", " + std::to_string(cpu.vertices[0].pos.y) + ", " + std::to_string(cpu.vertices[0].pos.z) + ")";
			firstFiveVerts += "\nUVW: (" + std::to_string(cpu.vertices[0].uvw.x) + ", " + std::to_string(cpu.vertices[0].uvw.y) + ", " + std::to_string(cpu.vertices[0].uvw.z) + ")";
			firstFiveVerts += "\nNRML: (" + std::to_string(cpu.vertices[0].nrm.x) + ", " + std::to_string(cpu.vertices[0].nrm.y) + ", " + std::to_string(cpu.vertices[0].nrm.z) + ")";

			firstFiveVerts += "\nPOS: (" + std::to_string(cpu.vertices[1].pos.x) + ", " + std::to_string(cpu.vertices[1].pos.y) + ", " + std::to_string(cpu.vertices[1].pos.z) + ")";
			firstFiveVerts += "\nUVW: (" + std::to_string(cpu.vertices[1].uvw.x) + ", " + std::to_string(cpu.vertices[1].uvw.y) + ", " + std::to_string(cpu.vertices[1].uvw.z) + ")";
			firstFiveVerts += "\nNRML: (" + std::to_string(cpu.vertices[1].nrm.x) + ", " + std::to_string(cpu.vertices[1].nrm.y) + ", " + std::to_string(cpu.vertices[1].nrm.z) + ")";

			firstFiveVerts += "\nPOS: (" + std::to_string(cpu.vertices[2].pos.x) + ", " + std::to_string(cpu.vertices[2].pos.y) + ", " + std::to_string(cpu.vertices[2].pos.z) + ")";
			firstFiveVerts += "\nUVW: (" + std::to_string(cpu.vertices[2].uvw.x) + ", " + std::to_string(cpu.vertices[2].uvw.y) + ", " + std::to_string(cpu.vertices[2].uvw.z) + ")";
			firstFiveVerts += "\nNRML: (" + std::to_string(cpu.vertices[2].nrm.x) + ", " + std::to_string(cpu.vertices[2].nrm.y) + ", " + std::to_string(cpu.vertices[2].nrm.z) + ")";

			firstFiveVerts += "\nPOS: (" + std::to_string(cpu.vertices[3].pos.x) + ", " + std::to_string(cpu.vertices[3].pos.y) + ", " + std::to_string(cpu.vertices[3].pos.z) + ")";
			firstFiveVerts += "\nUVW: (" + std::to_string(cpu.vertices[3].uvw.x) + ", " + std::to_string(cpu.vertices[3].uvw.y) + ", " + std::to_string(cpu.vertices[3].uvw.z) + ")";
			firstFiveVerts += "\nNRML: (" + std::to_string(cpu.vertices[3].nrm.x) + ", " + std::to_string(cpu.vertices[3].nrm.y) + ", " + std::to_string(cpu.vertices[3].nrm.z) + ")";

			firstFiveVerts += "\nPOS: (" + std::to_string(cpu.vertices[4].pos.x) + ", " + std::to_string(cpu.vertices[4].pos.y) + ", " + std::to_string(cpu.vertices[4].pos.z) + ")";
			firstFiveVerts += "\nUVW: (" + std::to_string(cpu.vertices[4].uvw.x) + ", " + std::to_string(cpu.vertices[4].uvw.y) + ", " + std::to_string(cpu.vertices[4].uvw.z) + ")";
			firstFiveVerts += "\nNRML: (" + std::to_string(cpu.vertices[4].nrm.x) + ", " + std::to_string(cpu.vertices[4].nrm.y) + ", " + std::to_string(cpu.vertices[4].nrm.z) + ")";
			std::cout << firstFiveVerts;

			// quick test to see if the entity has access to ALL of the materials on the model
			for (int i = 0; i < cpu.materialCount; i++)
			{
				std::cout << "\nKD " << std::to_string(i + 1) << ": " << "(" << std::to_string(cpu.materials[i].attrib.Kd.x) << ", " << std::to_string(cpu.materials[i].attrib.Kd.y) << ", " << std::to_string(cpu.materials[i].attrib.Kd.z) << ")";
			}

			// testing scene data values
			std::string sceneDat = "\nSCENE DATA: \n";
			sceneDat += "AMBIENT LIGHT: (" + std::to_string(sceneData.sunAmbient.x) + ", " + std::to_string(sceneData.sunAmbient.y) + ", " + std::to_string(sceneData.sunAmbient.z) + ")\n";
			sceneDat += "CAMERA POS: (" + std::to_string(sceneData.cameraPos.x) + ", " + std::to_string(sceneData.cameraPos.y) + ", " + std::to_string(sceneData.cameraPos.z) + ")";
			std::cout << sceneDat;

			// testing shader data
			std::string shaderMessage = "\nSHADER ADDRESSES:";
			const void* temp5 = shaders.vertexShader.Get();
			std::stringstream vertShaderAddress;
			vertShaderAddress << temp5;
			shaderMessage += "\nVERTEX: " + vertShaderAddress.str();

			const void* temp6 = shaders.pixelShader.Get();
			std::stringstream pixShaderAddress;
			pixShaderAddress << temp6;
			shaderMessage += "\nPIXEL: " + pixShaderAddress.str();
			std::cout << shaderMessage;

			// testing input layout data
			std::string inputLayoutMessage = "\nINPUT LAYOUT ADDRESS: ";
			const void* temp7 = inpLayout.inputLayout.Get();
			std::stringstream iLayoutAddress;
			iLayoutAddress << temp7;
			inputLayoutMessage += iLayoutAddress.str();
			std::cout << inputLayoutMessage;

#pragma endregion
		});

	// loading textures and sampler states into each textured entity and overwriting which pixel shader this entity should be using
	auto filter = game->filter<Textured, EntityBuffers, EntityShaders>();
	filter.each([this](Textured& t, EntityBuffers& buff, EntityShaders& shaders)
		{
			LoadTexture(device.Get(), t.texturePath, t.diffTexture.GetAddressOf(), t.diffuseTextureView.GetAddressOf());
			CreateSamplerState(t.texSampState);
			CompileTexturedPixelShader(&shaders.pixelShader);
		});

	// creating shaders for text items
	textVsBlob = CompileTextVertexShader(&textVertexShader);
	textPsBlob = CompileTextPixelShader(&textPixelShader);

	CreateTextPipeline();

	// Setup drawing engine
	if (SetupDrawcalls() == false)
		return false;

	return true;
}

// KEEP - Used for activating the drawing system - not currently in use but needed
bool JK::DirectXRendererLogic::Activate()
{
	/*if (startDraw.is_alive() &&
		updateDraw.is_alive() &&
		completeDraw.is_alive()) {
		if (runSystem) {
			startDraw.enable();
			updateDraw.enable();
			completeDraw.enable();
		}
		else {
			startDraw.disable();
			updateDraw.disable();
			completeDraw.disable();
		}
		return true;
	}*/
	return false;
}

// KEEP - Used for shutting down the drawing system
bool JK::DirectXRendererLogic::Shutdown()
{
	preDraw.destruct();
	draw.destruct();
	postDraw.destruct();

	game->defer_begin(); // required when removing while iterating!
	game->each([](flecs::entity e) {
		e.destruct(); // destroy this entitiy (happens at frame end)
		});
	game->defer_end(); // required when removing while iterating!

	return true;
}

// KEEP
std::string JK::DirectXRendererLogic::ShaderAsString(const char* shaderFilePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file; file.Create();
	file.GetFileSize(shaderFilePath, stringLength);
	if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
	return output;
}


// MY STUFF
// this method calls each shader compilation method and calls 'CreateVertexInputLayout'
bool JK::DirectXRendererLogic::LoadShaders(Model& e)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(&e.vertexShader);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(&e.nonTexturedPixelShader);

	CreateVertexInputLayout(vsBlob, &e.inputLayout);

	return true;
}

// this method CREATES a VERTEX shader based on the file value that it reads from game config
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader)
{
	//Microsoft::WRL::ComPtr<ID3D11Device> thisDev;
	//directX.GetDevice((void**)&thisDev);	// I don't believe this works because we are not creating a GDirectX11Surface yet

	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif

	// reading in shader from game config file
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	std::string vertexShaderSource = (*readCfg).at("Shaders").at("vertex").as<std::string>();

	// checking if it read in properly
	if (vertexShaderSource.empty())
		return false;

	vertexShaderSource = ShaderAsString(vertexShaderSource.c_str());

	// checking if it converted the string shader source to proper shader source
	if (vertexShaderSource.empty())
		return false;

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	hr = D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "vs_4_0", compilerFlags, 0, vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vShader->GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

// this method CREATES a PIXEL shader based on the file value that it reads from game config
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompilePixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif

	// reading in shader from game config file
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	std::string pixelShaderSource = (*readCfg).at("Shaders").at("pixel").as<std::string>();

	if (pixelShaderSource.empty())
		return false;

	// checking if it read in properly
	pixelShaderSource = ShaderAsString(pixelShaderSource.c_str());

	// checking if it converted the string shader source to proper shader source
	if (pixelShaderSource.empty())
		return false;

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	hr = D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "ps_4_0", compilerFlags, 0, psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, pShader->GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}

// this method CREATES the vertex shader for my text objects
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileTextVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	std::string vertexShaderSource = ReadFileIntoString("../Shaders/TextVertexShader.hlsl");

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	hr = D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "vs_4_0", compilerFlags, 0, vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, textVertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

// this method CREATES the pixel shader for my text objects
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileTextPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	std::string pixelShaderSource = ReadFileIntoString("../Shaders/TextPixelShader.hlsl");

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	hr = D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "ps_4_0", compilerFlags, 0, psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, textPixelShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}

// this method CREATES the vertex shader for the bounding box of current entity
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileBoundingBoxVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	std::string vertexShaderSource = ReadFileIntoString("../Shaders/BoundingBoxVertexShader.hlsl");

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	hr = D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "vs_4_0", compilerFlags, 0, vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vShader->GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

// this method CREATES the pixel shader for the bounding box of current entity
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileBoundingBoxPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	std::string pixelShaderSource = ReadFileIntoString("../Shaders/BoundingBoxPixelShader.hlsl");

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	hr = D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "ps_4_0", compilerFlags, 0, psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, pShader->GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}

// this method CREATES a PIXEL shader based on the file value that it reads from game config
Microsoft::WRL::ComPtr<ID3DBlob> JK::DirectXRendererLogic::CompileTexturedPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader)
{
	// flags for creating shader
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	std::string pixelShaderSource = ReadFileIntoString("../Shaders/TexturedPixelShader.hlsl");

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	hr = D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(), nullptr, nullptr, nullptr,
		"main", "ps_4_0", compilerFlags, 0, psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, pShader->GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}

// this method CREATES the drawing system 
bool JK::DirectXRendererLogic::SetupDrawcalls()
{
	// setting up background color of window
	float bckgrdClr[] = { 1.0f, 0.0f, 0.0f, 1.0f };		// TEST - this will make the background red if the color doesn't read properly from the gameConfig file

	// set background color from settings
	const char* channels[] = { "red", "green", "blue" };
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();

	// changing the background color to the color from the game config file
	for (int i = 0; i < std::size(channels); ++i)
	{
		bckgrdClr[i] = (*readCfg).at("BackGroundColor").at(channels[i]).as<float>();
	}

	// creating rendering system entity so we can call a system on it
	struct RenderingSystem {};
	game->entity("Rendering System").add<RenderingSystem>();

	// this is called ONCE per frame
	preDraw = game->system<RenderingSystem>().kind(flecs::PreUpdate).each([this, bckgrdClr](RenderingSystem& r)
		{
			devContext->ClearRenderTargetView(rendTargetView.Get(), bckgrdClr);
			devContext->ClearDepthStencilView(depthStencView.Get(), D3D11_CLEAR_DEPTH, 1, 0);

			// setting pipeline samplers and SRV's for any textured items - theoretically if something had multiple textures on this wouldn't work
			// as you would need to set it in between multiple draws on the same model - currently not of concern
			auto filter = game->filter<Textured, EntityBuffers, EntityShaders>();
			filter.each([this](Textured& t, EntityBuffers& buff, EntityShaders& shaders)
				{
					devContext->PSSetSamplers(0, 1, t.texSampState.GetAddressOf());
					devContext->PSSetShaderResources(0, 1, t.diffuseTextureView.GetAddressOf());
				});

			UpdateCameraPos();
		});

	// this system SETS up the pipeline for EACH entity using its component values, then draws the entity
	// this is called multiple times per frame, based on how many entities are in level
	draw = game->system<EntityMeshData, EntitySceneData, EntityBuffers, EntityShaders, EntityInputLayout, EntityCPUModelData>()
		.kind(flecs::OnUpdate).each([this](EntityMeshData& meshData, EntitySceneData& sceneData, EntityBuffers& buff, EntityShaders& shaders,
			EntityInputLayout& inpLayout, EntityCPUModelData& cpu)
			{
				// mapping and updating the view matrix for each entity
				D3D11_MAPPED_SUBRESOURCE mappedSub;
				mappedSub.pData = &sceneData;
				mappedSub.DepthPitch = 0;
				mappedSub.RowPitch = 0;

				// changing the material for the specific sub mesh on this ONE entity
				devContext->Map(buff.sceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSub);
				sceneData.viewMatrix = viewMatrix;
				memcpy(mappedSub.pData, &sceneData, sizeof(sceneData));
				devContext->Unmap(buff.sceneBuffer.Get(), 0);

				devContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				if (SetRenderTargets() == false)
					return false;
				if (SetVertexBuffers(buff.vertBuffer, 9) == false)
					return false;
				if (SetShaders(shaders.vertexShader, shaders.pixelShader) == false)
					return false;
				if (SetIndexBuffers(buff.indexBuffer) == false)
					return false;
				if (SetConstantBuffers(buff.meshBuffer, buff.sceneBuffer) == false)
					return false;
				if (SetInputLayout(inpLayout.inputLayout) == false)
					return false;

				mappedSub.pData = &meshData;

				//std::cout << meshData.worldMatrix.row4.x << meshData.worldMatrix.row4.y 
				//	<< meshData.worldMatrix.row4.z << std::endl;

				for (int i = 0; i < cpu.meshCount; i++)
				{
					// changing the material for the specific sub mesh on this ONE entity
					devContext->Map(buff.meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSub);
					meshData.materialInfo = cpu.materials[cpu.meshes[i].materialIndex].attrib;
					memcpy(mappedSub.pData, &meshData, sizeof(meshData));
					devContext->Unmap(buff.meshBuffer.Get(), 0);

					devContext->DrawIndexed(cpu.meshes[i].drawInfo.indexCount, cpu.meshes[i].drawInfo.indexOffset, 0);
				}

				if (debug)
				{
					// rendering bounding box of current entity
					if (SetVertexBuffers(buff.boundingVertBuffer, 3) == false)
						return false;
					if (SetShaders(shaders.boundingBoxVertexShader, shaders.boundingBoxPixelShader) == false)
						return false;
					if (SetInputLayout(inpLayout.boundingBoxInputLayout) == false)
						return false;

					devContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
					devContext->Draw(8, 0);
				}
			});

	postDraw = game->system<RenderingSystem>().kind(flecs::PostUpdate).each([this](RenderingSystem& r) {
		// rendering text last to ensure it is renderer on top of everything
		RenderText();
		swapChain->Present(1, 0);
		});

	return true;
}

// this method CREATES the input layout and assigns it to the current model passed in
bool JK::DirectXRendererLogic::CreateVertexInputLayout(Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob, Microsoft::WRL::ComPtr<ID3D11InputLayout>* inpLayout)
{
	D3D11_INPUT_ELEMENT_DESC attributes[3];

	attributes[0].SemanticName = "POSITION";
	attributes[0].SemanticIndex = 0;
	attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[0].InputSlot = 0;
	attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[0].InstanceDataStepRate = 0;

	attributes[1].SemanticName = "UVW";
	attributes[1].SemanticIndex = 0;
	attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[1].InputSlot = 0;
	attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[1].InstanceDataStepRate = 0;

	attributes[2].SemanticName = "NRML";
	attributes[2].SemanticIndex = 0;
	attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[2].InputSlot = 0;
	attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[2].InstanceDataStepRate = 0;

	hr = device->CreateInputLayout(attributes, ARRAYSIZE(attributes), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inpLayout->GetAddressOf());

	if (SUCCEEDED(hr))
		return true;
	else
		return false;
}

// this method CREATES a vertex buffer using the passed in values
bool JK::DirectXRendererLogic::CreateVertexBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName)
{
	D3D11_SUBRESOURCE_DATA vBuffSubData;
	ZeroMemory(&vBuffSubData, sizeof(vBuffSubData));		// zeroing out memory before using it
	vBuffSubData.pSysMem = data;
	vBuffSubData.SysMemPitch = 0;
	vBuffSubData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC vBuffDesc;
	ZeroMemory(&vBuffDesc, sizeof(vBuffDesc));		// zeroing out memory before using it
	vBuffDesc.ByteWidth = sizeInBytes;
	vBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;		// what type of buffer this is
	vBuffDesc.CPUAccessFlags = 0;
	vBuffDesc.MiscFlags = 0;
	vBuffDesc.StructureByteStride = 0;
	vBuffDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = device->CreateBuffer(&vBuffDesc, &vBuffSubData, buffName->GetAddressOf());

	if (SUCCEEDED(hr))
		return true;
	else
		return false;
}

// this method CREATES a generic index buffer using the passed in values
bool JK::DirectXRendererLogic::CreateIndexBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
	hr = device->CreateBuffer(&bDesc, &bData, buffName->GetAddressOf());

	if (SUCCEEDED(hr))
		return true;
	else
		return false;
}

// this method CREATES a generic constant buffer using the passed in values
bool JK::DirectXRendererLogic::CreateConstantBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName)
{
	D3D11_SUBRESOURCE_DATA subData;
	subData.pSysMem = data;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC buffDesc;
	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeInBytes;
	buffDesc.Usage = D3D11_USAGE_DYNAMIC;
	buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffDesc.MiscFlags = 0;
	buffDesc.StructureByteStride = 0;

	hr = device->CreateBuffer(&buffDesc, &subData, buffName->GetAddressOf());

	if (SUCCEEDED(hr))
		return true;
	else
		return false;
}

// this method SETS the pipeline index buffer to the buffer that is passed in
bool JK::DirectXRendererLogic::SetIndexBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer)
{
	devContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	if (indexBuffer.Get() == nullptr)
		return false;
	else
		return true;
}

// this method SETS the render target and depth stencil view for each model - consider moving this to init because I think it only needs to be called once
bool JK::DirectXRendererLogic::SetRenderTargets()
{
	ID3D11RenderTargetView* const views[] = { rendTargetView.Get() };
	devContext->OMSetRenderTargets(ARRAYSIZE(views), views, depthStencView.Get());

	if (depthStencView.Get() != NULL)
		return true;
	else
		return false;
}

// this method SETS the pipeline vertex buffer to the buffer that is passed in
bool JK::DirectXRendererLogic::SetVertexBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> vBuff, int numOfFloatsPerVertex)
{
	ID3D11Buffer* const buffers[] = { vBuff.Get() };
	const UINT  strides[] = { sizeof(float) * numOfFloatsPerVertex };
	const UINT offsets[] = { 0 };

	devContext->IASetVertexBuffers(0, ARRAYSIZE(buffers), buffers, strides, offsets);

	if (vBuff.Get() == nullptr)
		return false;
	else
		return true;
}

// this method SETS the pipeline shaders to the shaders that are passed in
bool JK::DirectXRendererLogic::SetShaders(Microsoft::WRL::ComPtr<ID3D11VertexShader> vShader, Microsoft::WRL::ComPtr<ID3D11PixelShader> pShader)
{
	// vertex
	devContext->VSSetShader(vShader.Get(), nullptr, 0);

	// pixel
	devContext->PSSetShader(pShader.Get(), nullptr, 0);

	if (vShader.Get() == nullptr || pShader.Get() == nullptr)
		return false;
	else
		return true;
}

bool JK::DirectXRendererLogic::LoadOriginalViewMatrix()
{
	lookAtVector = { 0.0f, 10.0f, 0.0f, 0 };
	upDirectionVector = { 0.0f, 1.0f, 0.0f, 0 };
	posOfCameraVector = { 25.0f, 10.0f, 0.0f, 0 };

	viewMatrix = GW::MATH::GIdentityMatrixF;
	GW::GReturn gr = proxyMatrix.LookAtLHF(posOfCameraVector, lookAtVector, upDirectionVector, viewMatrix);

	if (SUCCEEDED(gr))
		return true;
	return false;
}

// this creates the projection matrix, camera pos, directional light info, and ambient light info and assigns it to the current model passed in
bool JK::DirectXRendererLogic::SetSceneData(Model& e)
{

	// building projection matrix
	GW::MATH::GMATRIXF projectionMatrix = GW::MATH::GIdentityMatrixF;

	float fovY = G_DEGREE_TO_RADIAN_F(65);
	float aspectRatio;
	directX.GetAspectRatio(aspectRatio);
	float zNear = 0.1f;
	float zFar = 100.0f;

	proxyMatrix.ProjectionDirectXLHF(fovY, aspectRatio, zNear, zFar, projectionMatrix);

	// building directional light
	e.directionalLight.color = { 0.8f, 0.8f, 0.9f, 1.0f };
	e.directionalLight.direction = { -10.0f, -10.0f, 0.0f, 0.0f };
	proxyVector.NormalizeF(e.directionalLight.direction, e.directionalLight.direction);

	e.sceneData.viewMatrix = viewMatrix;
	e.sceneData.projectionMatrix = projectionMatrix;
	e.sceneData.lightData = e.directionalLight;
	e.sceneData.sunAmbient = { 0.8f, 0.8f, 0.8f, 1 };

	e.sceneData.cameraPos = posOfCameraVector;

	return true;
}

// this maps the mesh buffer, reassigns the material for the mesh based on the current selected mesh, then unmaps the mesh buffer
void JK::DirectXRendererLogic::SetMeshData(Model& e, D3D11_MAPPED_SUBRESOURCE* mappedSub, int index)
{
	// changing the material for the specific sub mesh on this ONE model
	devContext->Map(e.meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, mappedSub);
	e.meshData.materialInfo = e.cpuModel.materials[e.cpuModel.meshes[index].materialIndex].attrib;
	memcpy(mappedSub->pData, &e.meshData, sizeof(e.meshData));
	devContext->Unmap(e.meshBuffer.Get(), 0);
}

// this method is used to SET the pipeline constant buffers (mesh and scene buffers) to the buffers that are passed in
bool JK::DirectXRendererLogic::SetConstantBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> meshBuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer)
{
	ID3D11Buffer* constantBuffers[2] = { sceneBuffer.Get(), meshBuffer.Get() };
	devContext->VSSetConstantBuffers(0, 2, constantBuffers);
	devContext->PSSetConstantBuffers(0, 2, constantBuffers);

	if (sceneBuffer.Get() == nullptr || meshBuffer.Get() == false)
		return false;
	else
		return true;
}

// this method SETS the pipeline input layout to the input layout that is passed in
bool JK::DirectXRendererLogic::SetInputLayout(Microsoft::WRL::ComPtr<ID3D11InputLayout> inpLayout)
{
	devContext->IASetInputLayout(inpLayout.Get());

	if (inpLayout.Get() == nullptr)
		return false;
	else
		return true;
}

// this method creates an entity using all of the values contained on a model in the list of level models
void JK::DirectXRendererLogic::CreateEntityFromModel(Model& e)
{
	if (e.name == "SkyBox")
		return;

	auto ent = game->entity(e.name.c_str());
	ent.set<EntityName>({ e.name });
	ent.set<EntityBuffers>({ e.vertexBuffer, e.indexBuffer, e.meshBuffer, e.sceneBuffer });
	ent.set<EntityShaders>({ e.vertexShader, e.nonTexturedPixelShader });
	ent.set<EntityInputLayout>({ e.inputLayout });
	ent.set<EntityBoundary>({ e.boundary[0], e.boundary[1], e.boundary[2], e.boundary[3], e.boundary[4], e.boundary[5], e.boundary[6], e.boundary[7] });
	ent.set<EntityCPUModelData>({
		e.cpuModel.vertexCount,
		e.cpuModel.indexCount,
		e.cpuModel.materialCount,
		e.cpuModel.meshCount,
		e.cpuModel.vertices,
		e.cpuModel.indices,
		e.cpuModel.materials,
		e.cpuModel.batches,
		e.cpuModel.meshes });
	ent.set<EntityMeshData>({ e.meshData.worldMatrix, e.meshData.materialInfo });
	ent.set<EntitySceneData>({ e.sceneData.lightData.direction, e.sceneData.lightData.color, e.sceneData.viewMatrix, e.sceneData.projectionMatrix, e.sceneData.sunAmbient, e.sceneData.cameraPos });
	ent.add<Collidable>();
	//ent.set<Position>({ e.meshData.worldMatrix.row4.x, e.meshData.worldMatrix.row4.y });
	// tbd on whether this will work or not
	if (e.name == "Lemur_LemurMeshData")
	{
		ent.add<Player>();
		ent.set<Position>({ e.meshData.worldMatrix.row4.x, e.meshData.worldMatrix.row4.y });
		ent.set<ControllerID>({ 0 });
	}

	// if an entity has a map to a textur (aka it is textured)
	if (e.cpuModel.materials[0].map_Kd != nullptr)
	{
		// add a Textured component and set the map to the texture, the 
		ent.set<Textured>({ e.cpuModel.materials[0].map_Kd, e.diffTexture, e.diffuseTextureView, e.texSampState });
	}

}

// this method CREATES all pipeline variables needed for rendering text
void JK::DirectXRendererLogic::CreateTextPipeline()
{
	CreateScoreText();
	CreatePauseText();
	CreateControlsText();
	CreateStartMenuText();
	CreateTextVertexBuffer(scoreText, scoreTextVertexBuffer);
	CreateTextVertexBuffer(pauseText, pauseTextVertexBuffer);
	CreateTextVertexBuffer(controlsLabelText, controlsLabelTextBuffer);
	CreateTextVertexBuffer(leftArrowText, leftArrowTextBuffer);
	CreateTextVertexBuffer(rightArrowText, rightArrowTextBuffer);
	CreateTextVertexBuffer(spaceText, spaceTextBuffer);
	CreateTextVertexBuffer(escText, escTextBuffer);
	CreateTextVertexBuffer(startMenuLabelText, startMenuLabelTextVertexBuffer);
	CreateTextVertexBuffer(startMenuButtonText, startMenuButtonTextVertexBuffer);
	CreateTextVertexInputLayout();
	CreateTextConstantBuffer();
	CreateTextTexture();
	CreateTextBlendState();
	CreateDepthStencilState();
	CreateSamplerState(textSamplerState);
}

// this method CREATES our text variable and sets all of its values
void JK::DirectXRendererLogic::CreateScoreText()
{	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = "../XMLfiles/";
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	scoreText = Text();
	scoreText.SetText("Score: 0000");
	scoreText.SetFont(&consolas32);
	scoreText.SetPosition(-0.8f, 0.9f);
	scoreText.SetScale(0.75f, 0.75f);
	scoreText.SetRotation(0.0f);
	scoreText.SetDepth(0.1f);

	// 'update' will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	scoreText.Update(width, height);
}

// this method CREATES our text variable and sets all of its values
void JK::DirectXRendererLogic::CreatePauseText()
{	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = "../XMLfiles/";
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	pauseText = Text();
	pauseText.SetText("PAUSE GAME");
	pauseText.SetFont(&consolas32);
	pauseText.SetPosition(0, 0);
	pauseText.SetScale(0.75f, 0.75f);
	pauseText.SetRotation(0.0f);
	pauseText.SetDepth(0.1f);

	// 'update' will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	pauseText.Update(width, height);
}

// this method CREATES our text variable and sets all of its values
void JK::DirectXRendererLogic::CreateControlsText()
{	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = "../XMLfiles/";
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	controlsLabelText = Text();
	controlsLabelText.SetText("CONTROLS:");
	controlsLabelText.SetFont(&consolas32);
	controlsLabelText.SetPosition(0, -0.1f);
	controlsLabelText.SetScale(0.5f, 0.5f);
	controlsLabelText.SetRotation(0.0f);
	controlsLabelText.SetDepth(0.1f);

	leftArrowText = Text();
	leftArrowText.SetText("Left Arrow or Push Left on Left Joystick = Strafe Left");
	leftArrowText.SetFont(&consolas32);
	leftArrowText.SetPosition(0, -0.2f);
	leftArrowText.SetScale(0.5f, 0.5f);
	leftArrowText.SetRotation(0.0f);
	leftArrowText.SetDepth(0.1f);

	rightArrowText = Text();
	rightArrowText.SetText("Right Arrow or Push Right on Left Joystick = Strafe Right");
	rightArrowText.SetFont(&consolas32);
	rightArrowText.SetPosition(0, -0.3f);
	rightArrowText.SetScale(0.5f, 0.5f);
	rightArrowText.SetRotation(0.0f);
	rightArrowText.SetDepth(0.1f);

	spaceText = Text();
	spaceText.SetText("Space/A = Jump!");
	spaceText.SetFont(&consolas32);
	spaceText.SetPosition(0, -0.4f);
	spaceText.SetScale(0.5f, 0.5f);
	spaceText.SetRotation(0.0f);
	spaceText.SetDepth(0.1f);

	escText = Text();
	escText.SetText("Esc/Select = Pause");
	escText.SetFont(&consolas32);
	escText.SetPosition(0, -0.5f);
	escText.SetScale(0.5f, 0.5f);
	escText.SetRotation(0.0f);
	escText.SetDepth(0.1f);

	// 'update' will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	controlsLabelText.Update(width, height);
	leftArrowText.Update(width, height);
	rightArrowText.Update(width, height);
	spaceText.Update(width, height);
	escText.Update(width, height);
}

// this method CREATES our text variable and sets all of its values
void JK::DirectXRendererLogic::CreateStartMenuText()
{	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = "../XMLfiles/";
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	startMenuLabelText = Text();
	startMenuLabelText.SetText("START MENU");
	startMenuLabelText.SetFont(&consolas32);
	startMenuLabelText.SetPosition(0, 0.1f);
	startMenuLabelText.SetScale(1, 1);
	startMenuLabelText.SetRotation(0.0f);
	startMenuLabelText.SetDepth(0.1f);

	startMenuButtonText = Text();
	startMenuButtonText.SetText("Press 'ENTER' or 'START' to Play!");
	startMenuButtonText.SetFont(&consolas32);
	startMenuButtonText.SetPosition(0, -0.05f);
	startMenuButtonText.SetScale(0.75f, 0.75f);
	startMenuButtonText.SetRotation(0.0f);
	startMenuLabelText.SetDepth(0.1f);

	// 'update' will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	startMenuLabelText.Update(width, height);
	startMenuButtonText.Update(width, height);
}

// this method CREATES our text variable and sets all of its values
void JK::DirectXRendererLogic::CreateWinMenuText()
{	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = "../XMLfiles/";
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	winMenuText = Text();
	winMenuText.SetText("You Win!");
	winMenuText.SetFont(&consolas32);
	winMenuText.SetPosition(0, 0.1f);
	winMenuText.SetScale(1, 1);
	winMenuText.SetRotation(0.0f);
	winMenuText.SetDepth(0.1f);

	finalScoreText = Text();
	finalScoreText.SetText("Your Final Score is: " + std::to_string(JK::LevelLogic::gameScore));
	finalScoreText.SetFont(&consolas32);
	finalScoreText.SetPosition(0, -0.1f);
	finalScoreText.SetScale(0.7f, 0.7f);
	finalScoreText.SetRotation(0.0f);
	finalScoreText.SetDepth(0.1f);

	resetGameLabel = Text();
	resetGameLabel.SetText("Press 'R'or 'B' to reset game");
	resetGameLabel.SetFont(&consolas32);
	resetGameLabel.SetPosition(0, -0.2f);
	resetGameLabel.SetScale(0.7f, 0.7f);
	resetGameLabel.SetRotation(0.0f);
	resetGameLabel.SetDepth(0.1f);

	// 'update' will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	winMenuText.Update(width, height);
	finalScoreText.Update(width, height);
	resetGameLabel.Update(width, height);
}

// this method CREATES a vertex buffer based on our text variables value
void JK::DirectXRendererLogic::CreateTextVertexBuffer(Text& textVar, Microsoft::WRL::ComPtr<ID3D11Buffer>& textVertBuffer)
{
	// vertex buffer creation for the staticText
	const auto& textVerts = textVar.GetVertices();

	D3D11_SUBRESOURCE_DATA vBuffSubData;
	ZeroMemory(&vBuffSubData, sizeof(vBuffSubData));		// zeroing out memory before using it
	vBuffSubData.pSysMem = textVerts.data();
	vBuffSubData.SysMemPitch = 0;
	vBuffSubData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC vBuffDesc;
	ZeroMemory(&vBuffDesc, sizeof(vBuffDesc));		// zeroing out memory before using it
	vBuffDesc.ByteWidth = sizeof(TextVertex) * textVerts.size();
	vBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;		// what type of buffer this is
	vBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vBuffDesc.MiscFlags = 0;
	vBuffDesc.StructureByteStride = 0;
	vBuffDesc.Usage = D3D11_USAGE_DYNAMIC;

	device->CreateBuffer(&vBuffDesc, &vBuffSubData, textVertBuffer.GetAddressOf());
}

// this method CREATES the vertex input layout for the text variables
void JK::DirectXRendererLogic::CreateTextVertexInputLayout()
{
	// creating vertex input layout for staticText
	D3D11_INPUT_ELEMENT_DESC textInpLayoutFormat[2];

	textInpLayoutFormat[0].SemanticName = "POSITION";
	textInpLayoutFormat[0].SemanticIndex = 0;
	textInpLayoutFormat[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	textInpLayoutFormat[0].InputSlot = 0;
	textInpLayoutFormat[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	textInpLayoutFormat[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	textInpLayoutFormat[0].InstanceDataStepRate = 0;

	textInpLayoutFormat[1].SemanticName = "TEXCOORD";
	textInpLayoutFormat[1].SemanticIndex = 0;
	textInpLayoutFormat[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	textInpLayoutFormat[1].InputSlot = 0;
	textInpLayoutFormat[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	textInpLayoutFormat[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	textInpLayoutFormat[1].InstanceDataStepRate = 0;

	hr = device->CreateInputLayout(textInpLayoutFormat, ARRAYSIZE(textInpLayoutFormat), textVsBlob->GetBufferPointer(), textVsBlob->GetBufferSize(), textInputLayout.GetAddressOf());
}

// this method CREATES the texture for our text using DDSTextureLoader method
void JK::DirectXRendererLogic::CreateTextTexture()
{
	std::wstring texturePath = L"../Textures/font_consolas_32.dds";
	hr = CreateDDSTextureFromFile(device.Get(), texturePath.c_str(), (ID3D11Resource**)textTexture.GetAddressOf(), textShaderResourceView.GetAddressOf());
}

// this method CREATES the blend state for rendering our text
void JK::DirectXRendererLogic::CreateTextBlendState()
{
	// this is used to alpha blend objects with transparency
	CD3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = device->CreateBlendState(&blendDesc, textBlendState.GetAddressOf());
}

// this method CREATES the depth stencil state for rendering our text
void JK::DirectXRendererLogic::CreateDepthStencilState()
{
	CD3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = device->CreateDepthStencilState(&depthStencilDesc, textDepthStencilState.GetAddressOf());
}

// this method CREATES the constant buffer used for our text - this buffer possess the position, scale, rotation, and depth of our text
void JK::DirectXRendererLogic::CreateTextConstantBuffer()
{
	// update the constant buffer data for the text
	textConstantBufferData = UpdateTextConstantBufferData(scoreText);

	CreateConstantBuffer(&textConstantBufferData, sizeof(textConstantBufferData), &textConstantBuffer);
}

// this method CREATES the sampler state for rendering text
void JK::DirectXRendererLogic::CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& sampState)
{
	D3D11_SAMPLER_DESC sampDesc;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MinLOD = -FLT_MAX;
	sampDesc.MaxLOD = FLT_MAX;

	hr = device->CreateSamplerState(&sampDesc, sampState.GetAddressOf());
}

// this method SETS the shaders used for rendering text
void JK::DirectXRendererLogic::SetTextShaders()
{
	devContext->VSSetShader(textVertexShader.Get(), nullptr, 0);
	devContext->PSSetShader(textPixelShader.Get(), nullptr, 0);
}

// this method updates the vertex buffer with the proper values (the uv's and positions change if the text changes)
// and then SETS the vertex buffer necessary for rendering text 
void JK::DirectXRendererLogic::UpdateAndSetTextVertexBuffer(Text& textVar, Microsoft::WRL::ComPtr<ID3D11Buffer>& textVertBuff)
{
	// updating the texts values based on the new text 
	unsigned int width = 0;
	unsigned int height = 0;
	window.GetWidth(width);
	window.GetHeight(height);

	textVar.Update(width, height);
	const auto& textVerts = textVar.GetVertices();

	// mapping the vertex buffer and updating its info
	D3D11_MAPPED_SUBRESOURCE mappedSub;
	mappedSub.pData = &textVar;
	mappedSub.DepthPitch = 0;
	mappedSub.RowPitch = 0;

	devContext->Map(textVertBuff.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSub);
	memcpy(mappedSub.pData, textVerts.data(), sizeof(TextVertex) * textVerts.size());
	devContext->Unmap(textVertBuff.Get(), 0);

	// set the vertex buffer for the static text
	const UINT strides[] = { sizeof(float) * 4 };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { textVertBuff.Get() };

	devContext->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
}

// this method SETS the input layout for rendering text
void JK::DirectXRendererLogic::SetTextInputLayout()
{
	devContext->IASetInputLayout(textInputLayout.Get());
}

// this method updates the data within our constant buffer, CREATES a buffer using that data, and SETS the vertex shader constant buffers - need to fix this
void JK::DirectXRendererLogic::UpdateAndSetTextConstantBuffer(Text& textVar)
{
	// create mapped subresource so we can map the text constant buffer
	D3D11_MAPPED_SUBRESOURCE mappedSub;
	mappedSub.pData = &textConstantBufferData;
	mappedSub.DepthPitch = 0;
	mappedSub.RowPitch = 0;

	// map and update the constant buffer data for the text
	devContext->Map(textConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSub);
	textConstantBufferData = UpdateTextConstantBufferData(textVar);
	memcpy(mappedSub.pData, &textConstantBufferData, sizeof(textConstantBufferData));
	devContext->Unmap(textConstantBuffer.Get(), 0);

	ID3D11Buffer* constBuffers[1] = { textConstantBuffer.Get() };
	devContext->VSSetConstantBuffers(0, 1, textConstantBuffer.GetAddressOf());
}

// this method SETS the SRV for rendering text
void JK::DirectXRendererLogic::SetTextShaderResourceView()
{
	// bind the texture used for rendering the font
	ID3D11ShaderResourceView* shadViews[1] = { textShaderResourceView.Get() };
	devContext->PSSetShaderResources(0, 1, shadViews);
}

// this method SETS the blend state for rendering text - this allows us to blend letters using transparency values
void JK::DirectXRendererLogic::SetTextBlendState()
{
	// set the blend state in order to use transparency
	devContext->OMSetBlendState(textBlendState.Get(), NULL, 0xFFFFFFFF);
}

// this method SETS the depth stencil state necessary for rendering text - this allows us to actually use blending
void JK::DirectXRendererLogic::SetDepthStencilState()
{
	// set the depth stencil state for depth comparison [useful for transparency with the hud objects]
	devContext->OMSetDepthStencilState(textDepthStencilState.Get(), 0xFFFFFFFF);
}

// this method SETS the sampler state necessary for rendering text 
void JK::DirectXRendererLogic::SetTextSamplerState()
{
	ID3D11SamplerState* textSampState[1] = { textSamplerState.Get() };
	devContext->PSSetSamplers(0, 1, textSampState);
}

void::JK::DirectXRendererLogic::UpdateScoreText()
{
	scoreText.SetText("Score: " + std::to_string(JK::LevelLogic::gameScore));
}

// this method SETS up our pipeline for rendering text and actually draws the text, this is called from within our drawing system
void JK::DirectXRendererLogic::RenderText()
{
	devContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetTextShaders();
	SetTextInputLayout();
	SetTextShaderResourceView();
	SetTextBlendState();
	SetDepthStencilState();
	SetTextSamplerState();

	UpdateScoreText();
	UpdateAndSetTextVertexBuffer(scoreText, scoreTextVertexBuffer);
	UpdateAndSetTextConstantBuffer(scoreText);

	// draw the score text using the number of vertices
	devContext->Draw(scoreText.GetVertices().size(), 0);

	// if paused - render paused text
	auto p = game->filter<Pause, PauseBool>();
	p.each([this](Pause& p, PauseBool& pb)
		{
			paused = pb.paused;
		});

	auto s = game->filter<StartMenu, StartBool>();
	s.each([this](StartMenu& s, StartBool& sb) 
		{
			startMenu = sb.startMenu;
		});

	auto w = game->filter<WinMenu, WinBool>();
	w.each([this](WinMenu& w, WinBool& wb)
		{
			winMenu = wb.winBool;
		});

	if (startMenu)
	{
		UpdateAndSetTextVertexBuffer(startMenuLabelText, startMenuLabelTextVertexBuffer);
		UpdateAndSetTextConstantBuffer(startMenuLabelText);
		devContext->Draw(startMenuLabelText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(startMenuButtonText, startMenuButtonTextVertexBuffer);
		UpdateAndSetTextConstantBuffer(startMenuButtonText);
		devContext->Draw(startMenuButtonText.GetVertices().size(), 0);
	}

	if (paused && !winMenu)
	{
		UpdateAndSetTextVertexBuffer(pauseText, pauseTextVertexBuffer);
		UpdateAndSetTextConstantBuffer(pauseText);
		devContext->Draw(pauseText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(controlsLabelText, controlsLabelTextBuffer);
		UpdateAndSetTextConstantBuffer(controlsLabelText);
		devContext->Draw(controlsLabelText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(leftArrowText, leftArrowTextBuffer);
		UpdateAndSetTextConstantBuffer(leftArrowText);
		devContext->Draw(leftArrowText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(rightArrowText, rightArrowTextBuffer);
		UpdateAndSetTextConstantBuffer(rightArrowText);
		devContext->Draw(rightArrowText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(spaceText, spaceTextBuffer);
		UpdateAndSetTextConstantBuffer(spaceText);
		devContext->Draw(spaceText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(escText, escTextBuffer);
		UpdateAndSetTextConstantBuffer(escText);
		devContext->Draw(escText.GetVertices().size(), 0);
	}

	if (winMenu)
	{
		CreateWinMenuText();

		CreateTextVertexBuffer(winMenuText, winMenuTextBuffer);
		CreateTextVertexBuffer(finalScoreText, finalScoreTextBuffer);
		CreateTextVertexBuffer(resetGameLabel, resetGameLabelBuffer);

		UpdateAndSetTextVertexBuffer(winMenuText, winMenuTextBuffer);
		UpdateAndSetTextConstantBuffer(winMenuText);
		devContext->Draw(winMenuText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(finalScoreText, finalScoreTextBuffer);
		UpdateAndSetTextConstantBuffer(finalScoreText);
		devContext->Draw(finalScoreText.GetVertices().size(), 0);

		UpdateAndSetTextVertexBuffer(resetGameLabel, resetGameLabelBuffer);
		UpdateAndSetTextConstantBuffer(resetGameLabel);
		devContext->Draw(resetGameLabel.GetVertices().size(), 0);
	}

}

// this method returns all the data of the Text variable passed in
JK::DirectXRendererLogic::SPRITE_DATA JK::DirectXRendererLogic::UpdateTextConstantBufferData(const Text& s)
{
	SPRITE_DATA temp = { 0 };
	temp.pos_scale.x = s.GetPosition().x;
	temp.pos_scale.y = s.GetPosition().y;
	temp.pos_scale.z = s.GetScale().x;
	temp.pos_scale.w = s.GetScale().y;
	temp.rotation_depth.x = s.GetRotation();
	temp.rotation_depth.y = s.GetDepth();
	return temp;
}

// this method CREATES the input layout and assigns it to the current model passed in
bool JK::DirectXRendererLogic::CreateBoundBoxVertexInputLayout(Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob, Microsoft::WRL::ComPtr<ID3D11InputLayout>* inpLayout)
{
	D3D11_INPUT_ELEMENT_DESC attributes[1];

	attributes[0].SemanticName = "POSITION";
	attributes[0].SemanticIndex = 0;
	attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[0].InputSlot = 0;
	attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[0].InstanceDataStepRate = 0;

	hr = device->CreateInputLayout(attributes, ARRAYSIZE(attributes), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inpLayout->GetAddressOf());

	if (SUCCEEDED(hr))
		return true;
	else
		return false;
}

// this method uses the current entities texture map to grab a file associated with that texture map, create a string from that file path, and create a dds texture from that file
// it then assigns this texture to the passed in d3d11 texture and shader resource view
void JK::DirectXRendererLogic::LoadTexture(ID3D11Device* device, const char* texName, ID3D11Texture2D** texture, ID3D11ShaderResourceView** shadResView)
{
	std::string texFileName = CreateTextureFileName(texName);

	// converting texture string to const wchar_t*
	std::wstring wideString = std::wstring(texFileName.begin(), texFileName.end());

	// creating texture
	hr = CreateDDSTextureFromFile(device, wideString.c_str(), (ID3D11Resource**)texture, shadResView);
}

// this method takes in a sys file path to a texture name as a const char *, parses through the sys file path until it finds the "Textures" file
// appends "../" to the front of the file path, appends .dds to the end of the string, and returns it as an std::string
std::string JK::DirectXRendererLogic::CreateTextureFileName(const char* textureName)
{
	std::string currString = "";
	std::string finalString = "";
	std::string stringTexName = textureName;
	bool texturesFolderFound = false;

	// method 1: this iterates through each CHARACTER in the sys file path
	for (char c : stringTexName)
	{
		if (currString == "Textures")
		{
			texturesFolderFound = true;
		}

		else if (c == '.')
		{
			break;
		}

		if (c == '/' && !texturesFolderFound)
		{
			currString = "";
			continue;
		}

		currString += c;
	}

	finalString = "../" + currString;

	// appending .dds to original texture name
	finalString += ".dds";

	return finalString;
}

// this method finds the player entity and updates the cameras position and look at vector based on the players position
void JK::DirectXRendererLogic::UpdateCameraPos()
{
	game->filter<Player, EntityMeshData>().each([this](Player& p, EntityMeshData& m) 
		{
			posOfCameraVector = { 25.0f, m.worldMatrix.row4.y, m.worldMatrix.row4.z, 0};
			lookAtVector = { m.worldMatrix.row4.x, m.worldMatrix.row4.y, m.worldMatrix.row4.z };
			proxyMatrix.LookAtLHF(posOfCameraVector, lookAtVector, upDirectionVector, viewMatrix);
		});
}
