// Pulled directly from the "VulkanDescriptorSets" sample.
// Removing the arrays & using HLSL StructuredBuffer<> would be better.
#define MAX_INSTANCE_PER_DRAW 240

#pragma pack_matrix(row_major)

struct ATTRIBUTES
{
    float3 Kd;
    float d;
    float3 Ks;
    float Ns;
    float3 Ka;
    float sharpness;
    float3 Tf;
    float Ni;
    float3 Ke;
    unsigned int illum;
};

struct DIRECTIONAL_LIGHT
{
    float4 direction;
    float4 color;
};

cbuffer SCENE_DATA : register(b0)
{
    DIRECTIONAL_LIGHT lightData; // light data (GVECTORF for both direction and color)
    matrix viewMatrix, projectionMatrix; // viewing info
    float4 sunAmbient, cameraPos; // for ambient and specular lighting
};

cbuffer MESH_DATA : register(b1)
{
    matrix worldMatrix;
    ATTRIBUTES materialInfo;
};


struct InputVertex
{
    float3 pos : POSITION;
    float3 uvw : UVW;
    float3 nrm : NRML; 
};

struct OutputVertex
{
    float4 posH : SV_POSITION;
    float3 posW : WORLD;
    float3 uvw : TEXCOORD;
    float3 normW : NORMAL;
};

//cbuffer INSTANCE_UNIFORMS
//{
//	matrix instance_transforms[MAX_INSTANCE_PER_DRAW];
//	vector instance_colors[MAX_INSTANCE_PER_DRAW];
//};

//struct V_OUT { 
//	float4 hpos : SV_POSITION;
//	nointerpolation uint pixelInstanceID : INSTANCE;
//}; 

OutputVertex main(InputVertex inputVertex)
{
    OutputVertex output = (OutputVertex) 0; // zeroing out our output vertex instance
    output.posH = float4(inputVertex.pos, 1);
    output.posH = mul(output.posH, worldMatrix);    // placing vertex in world space
    output.posW = output.posH; // output.posH is only in world space at this moment

    output.posH = mul(output.posH, viewMatrix);     // placing vertex in view space
    output.posH = mul(output.posH, projectionMatrix);   // placing vertex in projection space

    output.uvw = inputVertex.uvw;     // setting output vertex's tex coordinates to incoming vertex's tex coordinates (no change) 
	
    output.normW = inputVertex.nrm; // setting output vertex's normals to incoming vertex's normals (no change) 
    output.normW = mul(output.normW, worldMatrix); // placing my normal into world space
    output.normW = normalize(output.normW);     // renormalizing outgoing normal

    return output;
    
	//V_OUT send = (V_OUT)0;
	//send.hpos = mul(instance_transforms[vertexInstanceID], 
	//				float4(inputVertex,0,1));
	//send.pixelInstanceID = vertexInstanceID;
}