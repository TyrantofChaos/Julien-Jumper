struct PSInput
{
    float4 posH : SV_POSITION;
};

struct PSOutput
{
    float4 rgba : SV_TARGET;
};


PSOutput main() 
{
    PSOutput output = (PSOutput) 0;
    output.rgba = float4(1.0f, 0, 0, 1.0f);
    return output;
}