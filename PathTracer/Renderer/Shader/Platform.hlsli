#ifndef PLATFORM_HLSLI
#define PLATFORM_HLSLI

// HLSL side of the kernel portability macros. The C++ side is Common/HlslCompat.h.
// Kernel code uses OUT(T)/INOUT(T) instead of out/inout and avoids swizzles beyond .x/.y/.z,
// casts like (T)0, and vector comparisons, so the same text compiles as C++.

#ifndef __cplusplus
#define OUT(T) out T
#define INOUT(T) inout T
#define LOOP [loop]
#define UNROLL [unroll]
#endif

static const float pi = 3.14159265358979323846f;
static const float inv_pi = 0.31830988618379067154f;
static const float float_max = 3.402823466e+38f;

float3 make_float3(float x, float y, float z) { return float3(x, y, z); }
float3 splat3(float x) { return float3(x, x, x); }
float max_component(float3 v) { return max(v.x, max(v.y, v.z)); }
float luminance(float3 c) { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; }

// Bit test instead of isnan/isinf: FXC may fold those away under fast-math assumptions.
bool is_finite(float x) { return (asuint(x) & 0x7f800000u) != 0x7f800000u; }
bool is_finite3(float3 v) { return is_finite(v.x) && is_finite(v.y) && is_finite(v.z); }

#endif
