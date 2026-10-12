#pragma once

// C++ side of the HLSL vector types used by Renderer/Shader/Shared.hlsli.
// Layout matches HLSL: float3 is 12 bytes, no padding. Arithmetic lives in HlslCompat.h.

#include <cstdint>

typedef std::uint32_t uint;

struct float2
{
    float x = 0.0f, y = 0.0f;
    float2() = default;
    constexpr float2(float x_value, float y_value) : x(x_value), y(y_value) {}
};

struct float3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float3() = default;
    constexpr float3(float x_value, float y_value, float z_value) : x(x_value), y(y_value), z(z_value) {}
};

struct float4
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    float4() = default;
    constexpr float4(float x_value, float y_value, float z_value, float w_value)
        : x(x_value), y(y_value), z(z_value), w(w_value) {}
    constexpr float4(float3 v, float w_value) : x(v.x), y(v.y), z(v.z), w(w_value) {}
};

struct uint2
{
    uint x = 0, y = 0;
    uint2() = default;
    constexpr uint2(uint x_value, uint y_value) : x(x_value), y(y_value) {}
};

static_assert(sizeof(float3) == 12 && sizeof(float4) == 16 && sizeof(uint2) == 8);
