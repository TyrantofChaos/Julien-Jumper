#pragma pack_matrix(row_major)


Texture2D diffuseTex : register(t0);
//Texture2D specularTex : register(t2);
//TextureCube skyBox : register(t0);
SamplerState texSamplerState : register(s0);
//SamplerState skyBoxSamplerState : register(s0);

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
    PSOutput output = (PSOutput) 0;
    
    // sampling the diffuse map to get pixel color
    output.rgba = diffuseTex.Sample(texSamplerState, input.uvw);
    
    // lambertian light
    float lightRatio = saturate(dot(-lightData.direction, float4(input.normW, 1))); // determining light ratio on current pixel based on light direction and this pixels normal

    float amountOfRed = mul(saturate(lightRatio + sunAmbient.x), lightData.color.x); // determining amount of red applied to this pixel based on directional lights ratio
    amountOfRed = mul(output.rgba.x, amountOfRed);

    float amountOfGreen = mul(saturate(lightRatio + sunAmbient.y), lightData.color.y); // determining amount of green applied to this pixel based on directional lights ratio
    amountOfGreen = mul(output.rgba.y, amountOfGreen);

    float amountOfBlue = mul(saturate(lightRatio + sunAmbient.z), lightData.color.z); // determining amount of blue applied to this pixel based on directional lights ratio
    amountOfBlue = mul(output.rgba.z, amountOfBlue);

    float3 resultPixelColor = float3(amountOfRed, amountOfGreen, amountOfBlue); // combining RGB of pixel into one vector
    
    //float3 specularColor = specularTex.Sample(texSamplerState, input.uvw);
    
    // specular light
    // if there is a specular map apply specular reflection, sampling the skybox for what gets reflected
    //if (specularColor.x != 0 || specularColor.y != 0 || specularColor.z != 0)
    //{
    //    float3 viewDir = normalize(cameraPos - input.posW);
    //    float3 halfVector = normalize(viewDir - normalize(lightData.direction));
    //    float intensity = max(pow(saturate(dot(input.normW, halfVector)), 10.0f + 0.0000001f), 0);
    //    float3 reflectLight = mul(intensity, specularColor);
    //    resultPixelColor = saturate(resultPixelColor + max(reflectLight, 0));
        
    //    float3 reflectionVector = normalize(reflect(-viewDir, input.normW));
    //    output.rgba = lerp(float4(resultPixelColor, 1), skyBox.Sample(skyBoxSamplerState, reflectionVector), intensity);
    //}
    
    output.rgba = float4(resultPixelColor, 1);
  
    
    return output;
}