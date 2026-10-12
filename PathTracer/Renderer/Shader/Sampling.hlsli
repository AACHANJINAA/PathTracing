#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

// Shading frames and direction sampling. Local frame: z = normal.

struct SHADING_FRAME
{
    float3 tangent;
    float3 bitangent;
    float3 normal;
};

// Duff et al. 2017, "Building an Orthonormal Basis, Revisited" (branchless, no singularity).
SHADING_FRAME make_frame(float3 n)
{
    float s = n.z >= 0.0f ? 1.0f : -1.0f;
    float a = -1.0f / (s + n.z);
    float b = n.x * n.y * a;
    SHADING_FRAME frame;
    frame.tangent = make_float3(1.0f + s * n.x * n.x * a, s * b, -s * n.x);
    frame.bitangent = make_float3(b, s + n.y * n.y * a, -n.y);
    frame.normal = n;
    return frame;
}

float3 to_local(SHADING_FRAME frame, float3 v)
{
    return make_float3(dot(v, frame.tangent), dot(v, frame.bitangent), dot(v, frame.normal));
}

float3 to_world(SHADING_FRAME frame, float3 v)
{
    return v.x * frame.tangent + v.y * frame.bitangent + v.z * frame.normal;
}

// Cosine-weighted hemisphere (Malley): pdf = cos(theta) / pi.
float3 sample_cosine_hemisphere(float u1, float u2)
{
    float r = sqrt(u1);
    float phi = 2.0f * pi * u2;
    return make_float3(r * cos(phi), r * sin(phi), sqrt(max(0.0f, 1.0f - u1)));
}

float cosine_hemisphere_pdf(float cos_theta)
{
    return cos_theta > 0.0f ? cos_theta * inv_pi : 0.0f;
}

// Uniform direction on the unit sphere: pdf = 1 / (4 pi).
float3 sample_uniform_sphere(float u1, float u2)
{
    float z = 1.0f - 2.0f * u1;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2.0f * pi * u2;
    return make_float3(r * cos(phi), r * sin(phi), z);
}

#endif
