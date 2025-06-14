// Pulled directly from the "VulkanDescriptorSets" sample.
// Removing the arrays & using HLSL StructuredBuffer<> would be better.
#define MAX_INSTANCE_PER_DRAW 240

#pragma pack_matrix(row_major)

//cbuffer INSTANCE_UNIFORMS
//{
//	matrix instance_transforms[MAX_INSTANCE_PER_DRAW];
//	vector instance_colors[MAX_INSTANCE_PER_DRAW];
//};

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

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : WORLD;
    float3 uvw : TEXCOORD;
    float3 normW : NORMAL;
};

struct PSOutput
{
    float4 rgba : SV_TARGET;
};

PSOutput main(PSInput input)
{
    PSOutput output = (PSOutput)0;
    
	// lambertian light
    float lightRatio = saturate(dot(-lightData.direction, float4(input.normW, 1))); // determining light ratio on current pixel based on light direction and this pixels normal

    float amountOfRed = mul(saturate(lightRatio + sunAmbient.x), lightData.color.x); // determining amount of red applied to this pixel based on directional lights ratio
    amountOfRed = mul(materialInfo.Kd.x, amountOfRed);

    float amountOfGreen = mul(saturate(lightRatio + sunAmbient.y), lightData.color.y); // determining amount of green applied to this pixel based on directional lights ratio
    amountOfGreen = mul(materialInfo.Kd.y, amountOfGreen);

    float amountOfBlue = mul(saturate(lightRatio + sunAmbient.z), lightData.color.z); // determining amount of blue applied to this pixel based on directional lights ratio
    amountOfBlue = mul(materialInfo.Kd.z, amountOfBlue);

    float3 resultPixelColor = float3(amountOfRed, amountOfGreen, amountOfBlue); // combining RGB of pixel into one vector

	// specular light
    float4 viewDir = normalize(cameraPos - float4(input.posW, 1));
    float4 halfVector = normalize(viewDir - normalize(lightData.direction));
    float intensity = max(pow(saturate(dot(float4(input.normW, 1), halfVector)), materialInfo.Ns + 0.0000001f), 0);
    float3 reflectLight = mul(intensity, materialInfo.Ks);

    resultPixelColor = saturate(resultPixelColor + max(reflectLight, 0) + materialInfo.Ke);
   
    //output.rgba = textTexture.Sample(sampState, input.uvw);       // FOR TEXTURED ITEMS
    
    output.rgba = float4(resultPixelColor, 1);
    
    return output;
}