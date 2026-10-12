#ifndef BSDF_HLSLI
#define BSDF_HLSLI

// BSDF interface (docs/RENDERING_MODEL.md section 6). All directions are in the local
// shading frame (z = shading normal, oriented toward wo for opaque materials).
//   bsdf_eval : f(wo, wi) without the cosine term; 0 for delta lobes
//   bsdf_pdf  : solid angle pdf of sampling wi; 0 for delta lobes
//   bsdf_sample: wi, weight = f * |cos(wi)| / pdf, pdf, isdelta

struct BSDF_SAMPLE
{
    float3 direction;
    float3 weight;
    float pdf;
    bool isdelta;
};

bool is_delta_material(MATERIAL material)
{
    return false;
}

float3 bsdf_eval(MATERIAL material, float3 wo, float3 wi)
{
    if (material.type == material_diffuse)
        return wo.z > 0.0f && wi.z > 0.0f ? material.albedo * inv_pi : splat3(0.0f);
    return splat3(0.0f);
}

float bsdf_pdf(MATERIAL material, float3 wo, float3 wi)
{
    if (material.type == material_diffuse)
        return wo.z > 0.0f ? cosine_hemisphere_pdf(wi.z) : 0.0f;
    return 0.0f;
}

bool bsdf_sample(MATERIAL material, float3 wo, float u1, float u2, float u3, OUT(BSDF_SAMPLE) sample)
{
    sample.direction = make_float3(0.0f, 0.0f, 1.0f);
    sample.weight = splat3(0.0f);
    sample.pdf = 0.0f;
    sample.isdelta = false;
    if (material.type == material_diffuse)
    {
        if (wo.z <= 0.0f) return false;
        sample.direction = sample_cosine_hemisphere(u1, u2);
        sample.pdf = cosine_hemisphere_pdf(sample.direction.z);
        if (sample.pdf <= 0.0f) return false;
        sample.weight = material.albedo; // (albedo / pi) * cos / (cos / pi)
        return true;
    }
    return false; // emissive surfaces absorb
}

#endif
