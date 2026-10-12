#include "Platform.hlsli"
#include "Shared.hlsli"

// M1: one camera ray per pixel, debug views (normal / albedo / depth).

ConstantBuffer<FRAME_CONSTANTS> g_frame : register(b0);
StructuredBuffer<PRIMITIVE> g_primitives : register(t0);
StructuredBuffer<MATERIAL> g_materials : register(t1);
RWTexture2D<float4> g_accumulation : register(u0);

PRIMITIVE get_primitive(uint index) { return g_primitives[index]; }
MATERIAL get_material(uint index) { return g_materials[index]; }

#include "Intersection.hlsli"
#include "Camera.hlsli"

float3 shade_debug(RAY ray)
{
    HIT hit;
    if (!intersect_scene(ray, 0.0f, float_max, hit)) return splat3(0.0f);
    MATERIAL material = get_material(hit.material);
    if (g_frame.debug_view == view_normal) return hit.normal * 0.5f + 0.5f;
    if (g_frame.debug_view == view_albedo)
        return material.type == material_emissive ? saturate(material.emission) : material.albedo;
    if (g_frame.debug_view == view_depth) return splat3(saturate(hit.t / max(g_frame.depth_range, 1e-3f)));
    // Lit placeholder until M2: albedo with a headlight.
    return material.type == material_emissive ? saturate(material.emission)
        : material.albedo * abs(dot(hit.normal, ray.direction));
}

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_frame.width || id.y >= g_frame.height) return;
    RAY ray = generate_camera_ray(float(id.x) + 0.5f, float(id.y) + 0.5f);
    g_accumulation[id.xy] = float4(shade_debug(ray), 0.0f);
}
