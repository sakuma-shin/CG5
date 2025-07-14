#include"Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); //SRV register=>t
SamplerState gSampler : register(s0); //Sampler register=>s

Texture2D<float32_t4> gMaskTexture : register(t1); //SRV register=>t



struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord);
    
    if (mask <= 0.5f)
    {
        discard;
    }
    
    PixelShaderOutPut output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}