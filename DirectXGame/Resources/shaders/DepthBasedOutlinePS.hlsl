#include"Test.hlsli"

struct InverseProjectionCBuffer
{
    float32_t4x4 inverseProjection;
};

Texture2D<float32_t4> gTexture : register(t0); //SRV register=>t
SamplerState gSampler : register(s0); //Sampler register=>s

Texture2D<float32_t4> gDepthTexture : register(t1); //SRV register=>t
SamplerState gSamplerPoint : register(s1); //Sampler register=>s

ConstantBuffer<InverseProjectionCBuffer> gInverseProjection : register(b0);

static const float32_t kPrewittHorizonalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    uint32_t width, height; //1,uvStepSizeの算出
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    
    float32_t2 difference = float32_t2(0.0f, 0.0f);
    //色を輝度に変換して、畳み込みを行っていく。微分フィルター用のKarnelになっている
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
             //3x3ループで現在のtexcoordを算出
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            //4.色に1/9掛けて足す
            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord).x;
            
            float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f),gInverseProjection.inverseProjection);
            float32_t viewZ = viewSpace.z * rcp(viewSpace.w);
            
            difference.x += viewZ * kPrewittHorizonalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }
    //変化の長さをウェイトとして合成。
    float32_t weight = length(difference);
    weight = saturate(weight);
    
    PixelShaderOutPut output;
    
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    
    return output;
}