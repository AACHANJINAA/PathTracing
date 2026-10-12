#include "Platform.hlsli"
#include "Shared.hlsli"

// Path tracing compute kernel: samples_per_frame paths per pixel, running-mean accumulation.
// Accumulation texture: rgb = mean radiance, a = number of discarded non-finite samples.

ConstantBuffer<FRAME_CONSTANTS> g_frame : register(b0);
StructuredBuffer<PRIMITIVE> g_primitives : register(t0);
StructuredBuffer<MATERIAL> g_materials : register(t1);
RWTexture2D<float4> g_accumulation : register(u0);
RWByteAddressBuffer g_counters : register(u1);   // [0] = non-finite samples since reset

PRIMITIVE get_primitive(uint index) { return g_primitives[index]; }
MATERIAL get_material(uint index) { return g_materials[index]; }

#include "Intersection.hlsli"
#include "Camera.hlsli"
#include "Random.hlsli"
#include "Sampling.hlsli"
#include "Bsdf.hlsli"
#include "PathTrace.hlsli"

float3 heat_map(float t)
{
    t = saturate(t);
    return make_float3(saturate(1.5f - abs(4.0f * t - 3.0f)), saturate(1.5f - abs(4.0f * t - 2.0f)),
        saturate(1.5f - abs(4.0f * t - 1.0f)));
}

float3 shade_debug(uint pixel_x, uint pixel_y)
{
    RAY ray = generate_camera_ray(float(pixel_x) + 0.5f, float(pixel_y) + 0.5f);
    if (g_frame.debug_view == view_path_length)
    {
        uint path_length;
        RNG rng = make_rng(pixel_x, pixel_y, g_frame.frame_index, g_frame.seed);
        trace_path(ray, rng, path_length);
        return heat_map(float(path_length) / float(max(g_frame.max_depth, 1u)));
    }
    HIT hit;
    if (!intersect_scene(ray, 0.0f, float_max, hit)) return splat3(0.0f);
    MATERIAL material = get_material(hit.material);
    if (g_frame.debug_view == view_normal) return hit.normal * 0.5f + 0.5f;
    if (g_frame.debug_view == view_albedo)
        return material.type == material_emissive ? saturate(material.emission) : material.albedo;
    return splat3(saturate(hit.t / max(g_frame.depth_range, 1e-3f)));
}

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_frame.width || id.y >= g_frame.height) return;
    if (g_frame.debug_view != view_lit && g_frame.debug_view != view_nan)
    {
        g_accumulation[id.xy] = float4(shade_debug(id.x, id.y), 0.0f);
        return;
    }

    float4 previous = g_frame.sample_count == 0 ? float4(0.0f, 0.0f, 0.0f, 0.0f) : g_accumulation[id.xy];
    float3 mean = previous.rgb;
    float invalid = previous.a;
    uint invalid_now = 0;
    LOOP
    for (uint s = 0; s < g_frame.samples_per_frame; ++s)
    {
        uint sample_index = g_frame.sample_count + s;
        uint path_length;
        float3 radiance = render_sample(id.x, id.y, sample_index, path_length);
        // RENDERING_MODEL 11: a non-finite sample counts as zero radiance and is reported.
        if (!is_finite3(radiance))
        {
            radiance = splat3(0.0f);
            invalid += 1.0f;
            ++invalid_now;
        }
        mean += (radiance - mean) / float(sample_index + 1);
    }
    g_accumulation[id.xy] = float4(mean, invalid);
    if (invalid_now > 0)
    {
        uint ignored;
        g_counters.InterlockedAdd(0, invalid_now, ignored);
    }
}
