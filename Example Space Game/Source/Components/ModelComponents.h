#pragma once
#include "../h2bParser.h"
namespace JK
{
	struct EntityName { std::string name; };
	struct EntityMeshData { GW::MATH::GMATRIXF worldMatrix;
							H2B::ATTRIBUTES materialInfo;
							};
	struct DIRECTIONAL_LIGHT
	{
		GW::MATH::GVECTORF direction;
		GW::MATH::GVECTORF color;
	};
	struct EntitySceneData
	{
		GW::MATH::GVECTORF direction;
		GW::MATH::GVECTORF color;// NEED TO PLACE DIRECTIONAL LIGHT INFO HERE?		// light data (GVECTORF for both direction and color)
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix;	// viewing info
		GW::MATH::GVECTORF sunAmbient, cameraPos;			// for ambient and specular lighting
	};
	struct EntityBuffers {
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> boundingVertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> meshBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer;
	};
	struct EntityShaders {
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> boundingBoxVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> boundingBoxPixelShader;
	};
	struct EntityInputLayout {
		Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> boundingBoxInputLayout;
	};
	struct EntityBoundary { GW::MATH2D::GVECTOR3F boundary[8]; };
	struct EntityCPUModelData {
		unsigned vertexCount;
		unsigned indexCount;
		unsigned materialCount;
		unsigned meshCount;
		std::vector<H2B::VERTEX> vertices;
		std::vector<unsigned> indices;
		std::vector<H2B::MATERIAL> materials;
		std::vector<H2B::BATCH> batches;
		std::vector<H2B::MESH> meshes;
		
	};
	struct Textured { const char* texturePath;
					  Microsoft::WRL::ComPtr<ID3D11Texture2D> diffTexture;
					  Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> diffuseTextureView;
					  Microsoft::WRL::ComPtr <ID3D11SamplerState> texSampState;
	};


}
