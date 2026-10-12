#pragma once

// Compiles the GPU kernel (PathTracer/Renderer/Shader/*.hlsli) as C++.
// Include this from exactly one translation unit (KernelTool.cpp): the kernel functions are
// ordinary (non-inline) definitions, exactly as in HLSL.

#include "../PathTracer/Common/HlslCompat.h"
#include "../PathTracer/Renderer/Shader/Platform.hlsli"
#include "../PathTracer/Renderer/Shader/Shared.hlsli"
#include "../PathTracer/Scene/Scene.h"

#include <thread>
#include <vector>

// ---- resource bindings (HLSL: ConstantBuffer / StructuredBuffer) ----
FRAME_CONSTANTS g_frame{};
std::vector<PRIMITIVE> g_primitive_buffer;
std::vector<MATERIAL> g_material_buffer;

PRIMITIVE get_primitive(uint index) { return g_primitive_buffer[index]; }
MATERIAL get_material(uint index) { return g_material_buffer[index]; }

#include "../PathTracer/Renderer/Shader/Intersection.hlsli"
#include "../PathTracer/Renderer/Shader/Camera.hlsli"
#include "../PathTracer/Renderer/Shader/Random.hlsli"
#include "../PathTracer/Renderer/Shader/Sampling.hlsli"
#include "../PathTracer/Renderer/Shader/Bsdf.hlsli"
#include "../PathTracer/Renderer/Shader/PathTrace.hlsli"

// ---- host helpers ----
inline void cpu_bind_scene(const SCENE& scene, uint width, uint height)
{
    g_primitive_buffer = scene.primitives;
    g_material_buffer = scene.materials;
    const CAMERA_BASIS basis = make_camera_basis(scene.camera);
    g_frame = FRAME_CONSTANTS{};
    g_frame.camera_position = basis.position;
    g_frame.camera_right = basis.right;
    g_frame.camera_up = basis.up;
    g_frame.camera_forward = basis.forward;
    g_frame.tan_half_fov_y = basis.tan_half_fov_y;
    g_frame.width = width;
    g_frame.height = height;
    g_frame.aspect = float(width) / float(height);
    g_frame.primitive_count = static_cast<uint>(scene.primitives.size());
    g_frame.depth_range = scene.bounding_radius() * 4.0f;
    g_frame.max_depth = 8;
    g_frame.russian_roulette_depth = 3;
    g_frame.samples_per_frame = 1;
    g_frame.sampling_mode = sampling_bsdf;
}

struct CPU_IMAGE
{
    uint width = 0, height = 0;
    std::vector<float3> pixels;   // running mean, same as the GPU accumulation texture
    uint invalid_samples = 0;     // non-finite samples replaced by zero
};

// Same accumulation as Integrator.hlsl (running mean, non-finite -> 0), multi-threaded by rows.
inline CPU_IMAGE cpu_render(uint spp)
{
    CPU_IMAGE image;
    image.width = g_frame.width;
    image.height = g_frame.height;
    image.pixels.assign(size_t(image.width) * image.height, float3{});
    const unsigned thread_count = std::max(1u, std::thread::hardware_concurrency());
    std::vector<uint> invalid(thread_count, 0);
    std::vector<std::thread> threads;
    for (unsigned worker = 0; worker < thread_count; ++worker)
        threads.emplace_back([&, worker]()
        {
            for (uint y = worker; y < image.height; y += thread_count)
                for (uint x = 0; x < image.width; ++x)
                {
                    float3 mean{};
                    for (uint s = 0; s < spp; ++s)
                    {
                        uint path_length;
                        float3 radiance = render_sample(x, y, s, path_length);
                        if (!is_finite3(radiance)) { radiance = splat3(0.0f); ++invalid[worker]; }
                        mean += (radiance - mean) / float(s + 1);
                    }
                    image.pixels[size_t(y) * image.width + x] = mean;
                }
        });
    for (auto& thread : threads) thread.join();
    for (uint count : invalid) image.invalid_samples += count;
    return image;
}
