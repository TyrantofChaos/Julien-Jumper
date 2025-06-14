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
};

struct OutputVertex
{
    float4 posH : SV_POSITION;
};

OutputVertex main(InputVertex inputVertex)
{
    OutputVertex output = (OutputVertex) 0; // zeroing out our output vertex instance
    
    output.posH = float4(inputVertex.pos, 1);
    output.posH = mul(output.posH, worldMatrix); // placing vertex in world space
    output.posH = mul(output.posH, viewMatrix); // placing vertex in view space
    output.posH = mul(output.posH, projectionMatrix); // placing vertex in projection space
    return output;
}