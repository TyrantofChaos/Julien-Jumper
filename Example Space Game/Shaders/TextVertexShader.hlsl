
//	 vertex shader used for rendering textured quads
struct VSInput
{
    float2 position : POSITION;
	float2 tex : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
	float2 uv : TESTTEX;
};

cbuffer SPRITE_DATA : register(b0)
{
	float2 pos_offset;
	float2 scale;
	float rotation;
	float depth;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.uv = input.tex;

    float2 r = float2(cos(rotation), sin(rotation));
    float2x2 rotate = float2x2(r.x, -r.y, r.y, r.x);
    float2 pos = pos_offset + mul(rotate, input.position * scale);

    output.pos = float4(pos, 0.5f, 1.0f);

    return output;
};