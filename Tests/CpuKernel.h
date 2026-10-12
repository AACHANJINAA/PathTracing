#pragma once

// Compiles the GPU kernel (PathTracer/Renderer/Shader/*.hlsli) as C++.
// Include this from exactly one translation unit (KernelTool.cpp): the kernel functions are
// ordinary (non-inline) definitions, exactly as in HLSL.

#include "../PathTracer/Common/HlslCompat.h"
#include "../PathTracer/Renderer/Shader/Platform.hlsli"
#include "../PathTracer/Renderer/Shader/Shared.hlsli"
#include "../PathTracer/Scene/Scene.h"

#include <vector>

// ---- resource bindings (HLSL: ConstantBuffer / StructuredBuffer) ----
FRAME_CONSTANTS g_frame{};
std::vector<PRIMITIVE> g_primitive_buffer;
std::vector<MATERIAL> g_material_buffer;

PRIMITIVE get_primitive(uint index) { return g_primitive_buffer[index]; }
MATERIAL get_material(uint index) { return g_material_buffer[index]; }

#include "../PathTracer/Renderer/Shader/Intersection.hlsli"
#include "../PathTracer/Renderer/Shader/Camera.hlsli"

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
}
