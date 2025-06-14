// pixel shader used for texturing 

Texture2D color : register(t0);
SamplerState filter : register(s0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TESTTEX;
};

struct PSOutput
{
    float4 rgba : SV_TARGET;
};

PSOutput main(PSInput input) 
{	
    PSOutput output = (PSOutput) 0;
    output.rgba = color.Sample(filter, input.uv);
    return output;
}
