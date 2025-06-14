#include "h2bParser.h"
#include "GameConfig.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
namespace JK
{
	class Model
	{	// class Model contains everyhting needed to draw a single 3D model
			// Name of the Model in the GameLevel (useful for debugging
			// Loads and stores CPU model data from .h2b file
	public:
		std::string name;
		H2B::Parser cpuModel; // reads the .h2b format
		bool textured = false;
		bool skyBoxBool = false;

		GW::MATH2D::GVECTOR3F boundary[8];

		// proxy variables
		GW::MATH::GMatrix proxyMatrix;
		GW::MATH::GVector proxyVector;

		// class structs
		struct MESH_DATA
		{
			GW::MATH::GMATRIXF worldMatrix;
			H2B::ATTRIBUTES materialInfo;
		}meshData;

		struct DIRECTIONAL_LIGHT
		{
			GW::MATH::GVECTORF direction;
			GW::MATH::GVECTORF color;
		}directionalLight;

		struct SCENE_DATA
		{
			DIRECTIONAL_LIGHT lightData;						// light data (GVECTORF for both direction and color)
			GW::MATH::GMATRIXF viewMatrix, projectionMatrix;	// viewing info
			GW::MATH::GVECTORF sunAmbient, cameraPos;			// for ambient and specular lighting
		}sceneData;

		// buffers
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;	// this ComPtr is just a 'safe' pointer that cleans up memory so we don't have to
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> meshBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer;

		// standard shaders and input layout
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> nonTexturedPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;


		std::weak_ptr<const GameConfig> gameConfig;		// unchanged

		// skybox
		Microsoft::WRL::ComPtr <ID3D11Texture2D> skyBoxTexture;
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> skyBoxView;
		Microsoft::WRL::ComPtr <ID3D11SamplerState> skyBoxSampState;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> skyVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> skyPixelShader;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyBoxRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> defaultRasterizerState;

		// texturing
		Microsoft::WRL::ComPtr<ID3D11PixelShader> texturedPixelShader;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> diffTexture;
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> diffuseTextureView;
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> specularTextureView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> specTexture;
		Microsoft::WRL::ComPtr <ID3D11SamplerState> texSampState;

		// mini map
		// main camera pos is: { 20.0f, 10.0f, 20.0f, 1 };
		GW::MATH::GVECTORF overHeadCameraPos = { 1.0f, 40.0f, 1.0f, 0.0f };
		GW::MATH::GMATRIXF overHeadViewMatrix;
		bool drawingMiniMap;

		// geometry shader
		bool geometryShaderUsed = false;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShader;

		// misc
		HRESULT hr;		// used to check if D3D11 method calls succeed or not
	public:

		// this methods sets the string name for the current model - especially useful for detecting skybox 
		inline void SetName(std::string modelName) {
			name = modelName;

			if (name == "Stan_Cube.000")
			{
				geometryShaderUsed = true;
			}
		}

		// this method sets the current models world matrix to the matrix passed in
		inline void SetWorldMatrix(GW::MATH::GMATRIXF worldMatrix)
		{
			if (name == "SkyBox")
			{
				meshData.worldMatrix = GW::MATH::GIdentityMatrixF;
			}
			else
			{
				meshData.worldMatrix = worldMatrix;
			}
		}

		// this method parses through the model data via h2bParser.h and loads that model onto the CPU
		bool LoadModelDataFromDisk(const char* h2bPath) {
			// if this succeeds "cpuModel" should now contain all the model's info
			return cpuModel.Parse(h2bPath);
		}
	};
}