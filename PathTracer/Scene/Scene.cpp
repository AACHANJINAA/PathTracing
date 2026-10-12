#include "Scene.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;

    float3 add(float3 a, float3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    float3 sub(float3 a, float3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    float3 scale(float3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
    float dot3(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    float3 cross3(float3 a, float3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    float length3(float3 a) { return std::sqrt(dot3(a, a)); }
    float3 normalize3(float3 a) { const float l = length3(a); return l > 0.0f ? scale(a, 1.0f / l) : a; }
}

float SCENE::bounding_radius() const
{
    float radius = 1.0f;
    for (const auto& primitive : primitives)
    {
        float extent = 0.0f;
        if (primitive.type == primitive_sphere) extent = length3(primitive.position) + primitive.radius;
        else if (primitive.type == primitive_quad)
            extent = length3(primitive.position) + length3(primitive.edge_u) + length3(primitive.edge_v);
        else extent = length3(primitive.position) + length3(primitive.half_size);
        radius = std::max(radius, extent);
    }
    return radius;
}

CAMERA_BASIS make_camera_basis(const SCENE_CAMERA& camera)
{
    CAMERA_BASIS basis;
    basis.position = camera.position;
    basis.forward = normalize3(sub(camera.target, camera.position));
    basis.right = normalize3(cross3(basis.forward, camera.up));
    basis.up = cross3(basis.right, basis.forward);
    basis.tan_half_fov_y = std::tan(camera.fov_y * 0.5f * pi / 180.0f);
    return basis;
}

PRIMITIVE make_sphere(float3 center, float radius, uint material)
{
    PRIMITIVE primitive{};
    primitive.type = primitive_sphere;
    primitive.material = material;
    primitive.position = center;
    primitive.radius = radius;
    primitive.area = 4.0f * pi * radius * radius;
    return primitive;
}

PRIMITIVE make_quad(float3 corner, float3 edge_u, float3 edge_v, uint material)
{
    PRIMITIVE primitive{};
    primitive.type = primitive_quad;
    primitive.material = material;
    primitive.position = corner;
    primitive.edge_u = edge_u;
    primitive.edge_v = edge_v;
    const float3 n = cross3(edge_u, edge_v);
    primitive.area = length3(n);
    primitive.normal = normalize3(n);
    return primitive;
}

PRIMITIVE make_box(float3 minimum, float3 maximum, float rotation_y, uint material)
{
    PRIMITIVE primitive{};
    primitive.type = primitive_box;
    primitive.material = material;
    primitive.position = scale(add(minimum, maximum), 0.5f);
    primitive.half_size = scale(sub(maximum, minimum), 0.5f);
    const float angle = rotation_y * pi / 180.0f;
    const float c = std::cos(angle), s = std::sin(angle);
    // Right-handed rotation about +Y: x -> (c, 0, -s), z -> (s, 0, c).
    primitive.edge_u = { c, 0.0f, -s };
    primitive.edge_v = { 0.0f, 1.0f, 0.0f };
    primitive.normal = { s, 0.0f, c };
    const float3 h = primitive.half_size;
    primitive.area = 8.0f * (h.x * h.y + h.y * h.z + h.z * h.x);
    return primitive;
}
