#include "Shared.hlsli"

// M0: writes a UV gradient into the accumulation texture to prove the compute -> display path.

ConstantBuffer<FRAME_CONSTANTS> g_frame : register(b0);
RWTexture2D<float4> g_accumulation : register(u0);

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_frame.width || id.y >= g_frame.height) return;
    float2 uv = (float2(id.xy) + 0.5f) / float2(g_frame.width, g_frame.height);
    g_accumulation[id.xy] = float4(uv.x, uv.y, 0.25f, 0.0f);
}
