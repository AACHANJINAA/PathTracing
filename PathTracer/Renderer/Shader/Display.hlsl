// Accumulated linear radiance -> exposure -> tone mapping -> sRGB -> UNORM back buffer.

struct DISPLAY_CONSTANTS
{
    float exposure;
    uint tonemap;
    uint flags;
    uint padding;
};

ConstantBuffer<DISPLAY_CONSTANTS> g_display : register(b0);
Texture2D<float4> g_accumulation : register(t0);

static const uint tonemap_none = 0;
static const uint tonemap_reinhard = 1;
static const uint tonemap_aces = 2;

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT vs_main(uint vertex_id : SV_VertexID)
{
    VS_OUTPUT output;
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// Narkowicz 2015 ACES filmic fit.
float3 tonemap_aces_fitted(float3 x)
{
    return saturate((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f));
}

float linear_to_srgb(float c)
{
    return c <= 0.0031308f ? 12.92f * c : 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
    float4 value = g_accumulation.Load(int3(int2(input.position.xy), 0));
    if ((g_display.flags & 1u) != 0u && value.a > 0.0f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
    float3 color = max(value.rgb, 0.0f) * exp2(g_display.exposure);
    if (g_display.tonemap == tonemap_reinhard) color = color / (1.0f + color);
    else if (g_display.tonemap == tonemap_aces) color = tonemap_aces_fitted(color);
    color = saturate(color);
    return float4(linear_to_srgb(color.r), linear_to_srgb(color.g), linear_to_srgb(color.b), 1.0f);
}
