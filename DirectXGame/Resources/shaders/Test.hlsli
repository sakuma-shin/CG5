struct VertexShaderOutput
{
    float32_t4 position : SV_Position;
    float32_t2 texcoord : TEXCOORD0;
};

// HLSLで2D座標から1Dの擬似乱数を生成する関数
float32_t rand2dTo1d(float32_t2 value)
{
    float32_t random = dot(value, float32_t2(12.9898, 78.233));
    random = sin(random) * 43758.5453123;
    return frac(random);
}