#pragma once

// Compiles the HLSL kernel (Renderer/Shader/*.hlsli) as C++ so the CPU tools in Tests/
// run exactly the same intersection, sampling and BSDF code as the GPU.
// Only the HLSL subset used by the kernel is provided. Not used by the Windows app.

#include "HlslTypes.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#define OUT(T) T&
#define INOUT(T) T&
#define LOOP
#define UNROLL

// ---- float2 ----
inline float2 operator+(float2 a, float2 b) { return { a.x + b.x, a.y + b.y }; }
inline float2 operator-(float2 a, float2 b) { return { a.x - b.x, a.y - b.y }; }
inline float2 operator*(float2 a, float s) { return { a.x * s, a.y * s }; }
inline float2 operator*(float s, float2 a) { return { a.x * s, a.y * s }; }

// ---- float3 ----
inline float3 operator+(float3 a, float3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline float3 operator-(float3 a, float3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline float3 operator*(float3 a, float3 b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline float3 operator/(float3 a, float3 b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline float3 operator+(float3 a, float s) { return { a.x + s, a.y + s, a.z + s }; }
inline float3 operator-(float3 a, float s) { return { a.x - s, a.y - s, a.z - s }; }
inline float3 operator*(float3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline float3 operator/(float3 a, float s) { return { a.x / s, a.y / s, a.z / s }; }
inline float3 operator+(float s, float3 a) { return a + s; }
inline float3 operator-(float s, float3 a) { return { s - a.x, s - a.y, s - a.z }; }
inline float3 operator*(float s, float3 a) { return a * s; }
inline float3 operator/(float s, float3 a) { return { s / a.x, s / a.y, s / a.z }; }
inline float3 operator-(float3 a) { return { -a.x, -a.y, -a.z }; }
inline float3& operator+=(float3& a, float3 b) { a = a + b; return a; }
inline float3& operator-=(float3& a, float3 b) { a = a - b; return a; }
inline float3& operator*=(float3& a, float3 b) { a = a * b; return a; }
inline float3& operator*=(float3& a, float s) { a = a * s; return a; }
inline float3& operator/=(float3& a, float s) { a = a / s; return a; }

// ---- scalar intrinsics (HLSL names) ----
inline float saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
inline float clamp(float x, float a, float b) { return std::clamp(x, a, b); }
inline uint clamp(uint x, uint a, uint b) { return std::clamp(x, a, b); }
inline float min(float a, float b) { return std::min(a, b); }
inline float max(float a, float b) { return std::max(a, b); }
inline uint min(uint a, uint b) { return std::min(a, b); }
inline uint max(uint a, uint b) { return std::max(a, b); }
inline int min(int a, int b) { return std::min(a, b); }
inline int max(int a, int b) { return std::max(a, b); }
inline float rsqrt(float x) { return 1.0f / std::sqrt(x); }
inline float rcp(float x) { return 1.0f / x; }
inline float frac(float x) { return x - std::floor(x); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float sign(float x) { return x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f); }
inline uint asuint(float x) { uint u; std::memcpy(&u, &x, 4); return u; }
inline float asfloat(uint u) { float x; std::memcpy(&x, &u, 4); return x; }
using std::abs; using std::sqrt; using std::exp; using std::exp2; using std::log; using std::pow;
using std::sin; using std::cos; using std::tan; using std::acos; using std::asin; using std::atan;
using std::atan2; using std::floor; using std::ceil;

// ---- vector intrinsics ----
inline float dot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float3 cross(float3 a, float3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline float length(float3 a) { return std::sqrt(dot(a, a)); }
inline float3 normalize(float3 a) { return a * (1.0f / length(a)); }
inline float3 abs(float3 a) { return { std::fabs(a.x), std::fabs(a.y), std::fabs(a.z) }; }
inline float3 min(float3 a, float3 b) { return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
inline float3 max(float3 a, float3 b) { return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
inline float3 saturate(float3 a) { return { saturate(a.x), saturate(a.y), saturate(a.z) }; }
inline float3 lerp(float3 a, float3 b, float t) { return a + (b - a) * t; }
inline float3 sqrt(float3 a) { return { std::sqrt(a.x), std::sqrt(a.y), std::sqrt(a.z) }; }
inline float3 exp(float3 a) { return { std::exp(a.x), std::exp(a.y), std::exp(a.z) }; }
inline float3 pow(float3 a, float e) { return { std::pow(a.x, e), std::pow(a.y, e), std::pow(a.z, e) }; }
