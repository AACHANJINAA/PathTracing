#pragma once

// CPU-side scene: primitives and materials in the exact GPU layout (Shared.hlsli),
// plus the camera. Portable C++ (no Windows headers) so the CPU kernel tools can use it.

#include "../Common/HlslTypes.h"
#include "../Renderer/Shader/Shared.hlsli"

#include <string>
#include <vector>

struct SCENE_CAMERA
{
    float3 position{ 0.0f, 0.0f, 5.0f };
    float3 target{ 0.0f, 0.0f, 0.0f };
    float3 up{ 0.0f, 1.0f, 0.0f };
    float fov_y = 45.0f; // degrees, vertical
};

struct CAMERA_BASIS
{
    float3 position;
    float3 right;
    float3 up;
    float3 forward;
    float tan_half_fov_y = 0.0f;
};

struct SCENE
{
    std::string name;
    SCENE_CAMERA camera;
    std::vector<std::string> material_names;
    std::vector<MATERIAL> materials;
    std::vector<PRIMITIVE> primitives;

    // Radius of a sphere around the origin that contains every primitive (for UI ranges).
    float bounding_radius() const;
};

// Right-handed, +Y up: forward = target - position, right = forward x up.
CAMERA_BASIS make_camera_basis(const SCENE_CAMERA& camera);

PRIMITIVE make_sphere(float3 center, float radius, uint material);
PRIMITIVE make_quad(float3 corner, float3 edge_u, float3 edge_v, uint material);
// Box from local min/max corners, rotated by rotation_y degrees about its own center (+Y axis).
PRIMITIVE make_box(float3 minimum, float3 maximum, float rotation_y, uint material);
