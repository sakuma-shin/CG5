#include"Test.hlsli"

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    PixelShaderOutPut output;
    float32_t2 uv = input.texcoord;
    output.color =float32_t4(uv.x, uv.y, 0.0f, 1.0f);
    return output;
}

//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}