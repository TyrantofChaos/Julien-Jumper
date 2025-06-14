// Contains our global game settings
#include "../GameConfig.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// example space game (avoid name collisions)
#include "../Level_Objects.h"
#include <wrl\client.h>
#include "../Font.h"
#include "../Systems/PauseSystem.h"
#include "../Systems/StartMenuSystem.h"
#include "../Systems/LevelLogic.h"
#include "../Systems/WinSystem.h"

namespace JK
{
	class DirectXRendererLogic
	{
		// shared connection to the main ECS engine	
		std::shared_ptr<flecs::world> game;		// unchanged

		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;		// unchanged

		// handle to our running ECS systems
		flecs::system preDraw;
		flecs::system draw;
		flecs::system postDraw;

		// Used to query screen dimensions
		GW::SYSTEM::GWindow window;		// unchange

		GW::GRAPHICS::GDirectX11Surface directX;

		// swap chain
		Microsoft::WRL::ComPtr<IDXGISwapChain>  swapChain;

		// device
		Microsoft::WRL::ComPtr<ID3D11Device>  device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext;

		// view data
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rendTargetView;
		D3D11_VIEWPORT viewport;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> textDepthStencilState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> textBlendState;

		// proxys
		GW::MATH::GMatrix proxyMatrix;
		GW::MATH::GVector proxyVector;

		// view matrix
		GW::MATH::GMATRIXF viewMatrix;
		GW::MATH::GVECTORF posOfCameraVector;
		GW::MATH::GVECTORF lookAtVector;
		GW::MATH::GVECTORF upDirectionVector;

		// misc
		HRESULT hr;
		GW::SYSTEM::GLog errorLog;
		bool paused = false;
		bool startMenu = true;
		bool winMenu = false;
		bool debug = false;

		// level data
		Level_Objects myLevel;

		// used to trigger clean up of vulkan resources
		GW::CORE::GEventReceiver shutdown;

	public:

		// control if the system is actively running
		bool Activate();		// currently not in use but needed

		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::GRAPHICS::GDirectX11Surface _dx11,
			GW::SYSTEM::GWindow _window);

		bool Shutdown();

	private:
		bool LoadShaders(Model& e);
		bool LoadOriginalViewMatrix();
		Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader);
		Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader);
		Microsoft::WRL::ComPtr<ID3DBlob> CompileTextVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader);
		Microsoft::WRL::ComPtr<ID3DBlob> CompileTextPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader);
		bool CreateVertexBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName);
		bool CreateIndexBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName);
		bool CreateVertexInputLayout(Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob, Microsoft::WRL::ComPtr<ID3D11InputLayout>* buffName);
		bool CreateConstantBuffer(const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>* buffName);
		bool SetSceneData(Model& e);
		void SetMeshData(Model& e, D3D11_MAPPED_SUBRESOURCE* mappedSub, int index);
		bool SetRenderTargets();
		bool SetVertexBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> vBuff, int numOfFloatsPerVertex);
		bool SetShaders(Microsoft::WRL::ComPtr<ID3D11VertexShader> vShader, Microsoft::WRL::ComPtr<ID3D11PixelShader> pShader);
		bool SetIndexBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer);
		bool SetConstantBuffers(Microsoft::WRL::ComPtr<ID3D11Buffer> meshBuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer);
		bool SetInputLayout(Microsoft::WRL::ComPtr<ID3D11InputLayout> inpLayout);
		bool SetupDrawcalls();
		void CreateEntityFromModel(Model& e);
		void UpdateCameraPos();

		// TEXT
		// text methods
		void CreateTextPipeline();
		void CreateScoreText();
		void CreatePauseText(); 
		void CreateControlsText();
		void CreateStartMenuText();
		void CreateWinMenuText();
		void CreateTextVertexBuffer(Text& textVar, Microsoft::WRL::ComPtr<ID3D11Buffer>& textVertBuffer);
		void CreateTextVertexInputLayout();
		void CreateTextTexture();
		void CreateTextBlendState();
		void CreateDepthStencilState();
		void CreateTextConstantBuffer();
		void CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& sampState);

		void SetTextShaders();
		void UpdateAndSetTextVertexBuffer(Text& textVar, Microsoft::WRL::ComPtr<ID3D11Buffer>& textVertBuff);
		void SetTextInputLayout();
		void UpdateAndSetTextConstantBuffer(Text& textVar);
		void SetTextShaderResourceView();
		void SetTextBlendState();
		void SetDepthStencilState();
		void SetTextSamplerState(); 
		void UpdateScoreText();

		// text data
		struct SPRITE_DATA
		{
			GW::MATH::GVECTORF pos_scale;
			GW::MATH::GVECTORF rotation_depth;
		};

		Font consolas32;

	public:
		Text scoreText;		// making this public so we can change the text from other classes
		Text pauseText;
		Text controlsLabelText;
		Text leftArrowText;
		Text rightArrowText;
		Text spaceText;
		Text escText;
		Text startMenuLabelText;
		Text startMenuButtonText;
		Text winMenuText;
		Text finalScoreText;
		Text resetGameLabel;

	private:
		// text pipeline variables
		Microsoft::WRL::ComPtr<ID3D11Buffer> scoreTextVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> pauseTextVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> controlsLabelTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> leftArrowTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> rightArrowTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> spaceTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> escTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> textConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> startMenuLabelTextVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> startMenuButtonTextVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> winMenuTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> finalScoreTextBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> resetGameLabelBuffer;
		SPRITE_DATA	textConstantBufferData = { 0 };

		Microsoft::WRL::ComPtr<ID3D11VertexShader> textVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>  textPixelShader;
		Microsoft::WRL::ComPtr<ID3DBlob> textVsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> textPsBlob;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> textInputLayout;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> textSamplerState;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> textTexture;

		// bounding box rendering
		bool CreateBoundBoxVertexInputLayout(Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob, Microsoft::WRL::ComPtr<ID3D11InputLayout>* inpLayout);
		Microsoft::WRL::ComPtr<ID3DBlob> CompileBoundingBoxVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>* vShader);
		Microsoft::WRL::ComPtr<ID3DBlob> CompileBoundingBoxPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader);
		void RenderText();
		SPRITE_DATA UpdateTextConstantBufferData(const Text& s);

		// TEXTURING
		void LoadTexture(ID3D11Device* device, const char* texName, ID3D11Texture2D** texture, ID3D11ShaderResourceView** shadResView);
		std::string CreateTextureFileName(const char* textureName);
		Microsoft::WRL::ComPtr<ID3DBlob> CompileTexturedPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>* pShader);

		// Utility funcs
		std::string ShaderAsString(const char* shaderFilePath);
	};
};



