#ifndef PATH_TRACE_HLSLI
#define PATH_TRACE_HLSLI

// Unidirectional path tracer (docs/RENDERING_MODEL.md section 8).
// Requires: Intersection, Camera, Random, Sampling, Bsdf and g_frame.
//
// Path length convention: a ray is the bounce-th segment of the path (camera ray = 1).
// A contribution that ends on segment n is allowed when n <= g_frame.max_depth, so
// max_depth = 1 shows emitters only and max_depth = 2 is direct lighting.

// Self-intersection offset (RENDERING_MODEL 3): push along the geometric normal to the
// side the new ray leaves from.
float3 offset_ray_origin(float3 position, float3 geometric_normal, float3 direction)
{
    float scale = max(1.0f, max(abs(position.x), max(abs(position.y), abs(position.z))));
    float epsilon = 1e-4f * scale;
    return position + (dot(direction, geometric_normal) > 0.0f ? epsilon : -epsilon) * geometric_normal;
}

float3 emitted_radiance(MATERIAL material, HIT hit)
{
    // Emitters radiate from their front face only (RENDERING_MODEL 5).
    return material.type == material_emissive && hit.isfront_face ? material.emission : splat3(0.0f);
}

float3 trace_path(RAY ray, INOUT(RNG) rng, OUT(uint) path_length)
{
    float3 radiance = splat3(0.0f);
    float3 throughput = make_float3(1.0f, 1.0f, 1.0f);
    path_length = 0;

    LOOP
    for (uint bounce = 1; bounce <= g_frame.max_depth; ++bounce)
    {
        HIT hit;
        if (!intersect_scene(ray, 0.0f, float_max, hit)) break;
        path_length = bounce;
        MATERIAL material = get_material(hit.material);

        // BSDF sampling only: every emitter hit counts in full.
        radiance += throughput * emitted_radiance(material, hit);

        if (bounce == g_frame.max_depth) break;

        // Opaque shading normal faces the side the ray came from.
        float3 wo_world = -1.0f * ray.direction;
        float3 shading_normal = hit.isfront_face ? hit.normal : -1.0f * hit.normal;
        SHADING_FRAME frame = make_frame(shading_normal);
        float3 wo = to_local(frame, wo_world);

        BSDF_SAMPLE sample;
        float u1 = next_float(rng);
        float u2 = next_float(rng);
        float u3 = next_float(rng);
        if (!bsdf_sample(material, wo, u1, u2, u3, sample)) break;
        throughput *= sample.weight;

        // Russian roulette (RENDERING_MODEL 8): unbiased early termination.
        if (bounce >= g_frame.russian_roulette_depth)
        {
            float survival = min(max_component(throughput), 0.95f);
            if (next_float(rng) >= survival) break;
            throughput /= survival;
        }

        float3 wi_world = to_world(frame, sample.direction);
        ray = make_ray(offset_ray_origin(hit.position, hit.normal, wi_world), wi_world);
    }
    return radiance;
}

// One pixel sample: jittered camera ray + path. sample_index is the global accumulation index.
float3 render_sample(uint pixel_x, uint pixel_y, uint sample_index, OUT(uint) path_length)
{
    RNG rng = make_rng(pixel_x, pixel_y, sample_index, g_frame.seed);
    float jitter_x = next_float(rng);
    float jitter_y = next_float(rng);
    RAY ray = generate_camera_ray(float(pixel_x) + jitter_x, float(pixel_y) + jitter_y);
    return trace_path(ray, rng, path_length);
}

#endif
