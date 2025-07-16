#include"Test.hlsli"

struct randomTime
{
    float32_t time;
};

Texture2D<float32_t4> gTexture : register(t0); //SRV register=>t
SamplerState gSampler : register(s0); //Sampler register=>s

ConstantBuffer<randomTime> gRandomTime : register(b1);

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    
    PixelShaderOutPut output;
    
    //乱数生成
    float32_t random = rand2dTo1d(input.texcoord * gRandomTime.time);
    
    output.color = gTexture.Sample(gSampler, input.texcoord);
    output.color.rgb *= random;
    return output;
}