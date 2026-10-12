#ifndef INTERSECTION_HLSLI
#define INTERSECTION_HLSLI

// Ray-primitive intersection (docs/RENDERING_MODEL.md section 3).
// Requires: get_primitive(uint), g_frame.primitive_count.

struct RAY
{
    float3 origin;
    float3 direction;    // normalized
};

struct HIT
{
    float t;
    float3 position;
    float3 normal;       // geometric normal, outward (closed shapes) or cross(u, v) (quad front)
    uint primitive;
    uint material;
    bool isfront_face;   // dot(ray.direction, normal) < 0
};

RAY make_ray(float3 origin, float3 direction)
{
    RAY ray;
    ray.origin = origin;
    ray.direction = direction;
    return ray;
}

// Sphere: numerically stable quadratic (Ray Tracing Gems, ch. 7).
bool intersect_sphere(RAY ray, PRIMITIVE sphere, float tmin, float tmax, OUT(float) t, OUT(float3) normal)
{
    float3 f = ray.origin - sphere.position;
    float b = -dot(f, ray.direction);
    float3 l = f + b * ray.direction;
    float r2 = sphere.radius * sphere.radius;
    float discriminant = r2 - dot(l, l);
    t = 0.0f;
    normal = splat3(0.0f);
    if (discriminant < 0.0f) return false;
    float c = dot(f, f) - r2;
    float q = b + (b >= 0.0f ? 1.0f : -1.0f) * sqrt(discriminant);
    float t0 = q != 0.0f ? c / q : 0.0f;
    float t1 = q;
    if (t0 > t1) { float swap = t0; t0 = t1; t1 = swap; }
    if (t0 >= tmin && t0 <= tmax) t = t0;
    else if (t1 >= tmin && t1 <= tmax) t = t1;
    else return false;
    normal = normalize(ray.origin + t * ray.direction - sphere.position);
    return true;
}

// Quad (parallelogram): corner + alpha * u + beta * v, alpha, beta in [0, 1].
bool intersect_quad(RAY ray, PRIMITIVE quad, float tmin, float tmax, OUT(float) t, OUT(float3) normal)
{
    normal = quad.normal;
    t = 0.0f;
    float denominator = dot(quad.normal, ray.direction);
    if (abs(denominator) < 1e-9f) return false;
    t = dot(quad.position - ray.origin, quad.normal) / denominator;
    if (t < tmin || t > tmax) return false;
    float3 local = ray.origin + t * ray.direction - quad.position;
    float3 n = cross(quad.edge_u, quad.edge_v);
    float3 w = n / dot(n, n);
    float alpha = dot(w, cross(local, quad.edge_v));
    float beta = dot(w, cross(quad.edge_u, local));
    return alpha >= 0.0f && alpha <= 1.0f && beta >= 0.0f && beta <= 1.0f;
}

// Oriented box: slab test in the box's local frame (axes = edge_u, edge_v, normal).
bool intersect_box(RAY ray, PRIMITIVE box, float tmin, float tmax, OUT(float) t, OUT(float3) normal)
{
    float3 d = ray.origin - box.position;
    float3 o = make_float3(dot(d, box.edge_u), dot(d, box.edge_v), dot(d, box.normal));
    float3 dir = make_float3(dot(ray.direction, box.edge_u), dot(ray.direction, box.edge_v),
        dot(ray.direction, box.normal));
    float3 inverse = make_float3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    float3 t_low = (-1.0f * box.half_size - o) * inverse;
    float3 t_high = (box.half_size - o) * inverse;
    float3 t_near3 = min(t_low, t_high);
    float3 t_far3 = max(t_low, t_high);
    float t_near = max(t_near3.x, max(t_near3.y, t_near3.z));
    float t_far = min(t_far3.x, min(t_far3.y, t_far3.z));
    t = 0.0f;
    normal = splat3(0.0f);
    if (t_near > t_far) return false;
    if (t_near >= tmin && t_near <= tmax) t = t_near;
    else if (t_far >= tmin && t_far <= tmax) t = t_far;
    else return false;
    // The face is the axis where the local hit point is closest to the half extent.
    float3 p = (o + t * dir) / box.half_size;
    float3 a = abs(p);
    float3 local_normal;
    if (a.x >= a.y && a.x >= a.z) local_normal = make_float3(p.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    else if (a.y >= a.z) local_normal = make_float3(0.0f, p.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
    else local_normal = make_float3(0.0f, 0.0f, p.z >= 0.0f ? 1.0f : -1.0f);
    normal = local_normal.x * box.edge_u + local_normal.y * box.edge_v + local_normal.z * box.normal;
    return true;
}

bool intersect_primitive(RAY ray, PRIMITIVE primitive, float tmin, float tmax, OUT(float) t, OUT(float3) normal)
{
    if (primitive.type == primitive_sphere) return intersect_sphere(ray, primitive, tmin, tmax, t, normal);
    if (primitive.type == primitive_quad) return intersect_quad(ray, primitive, tmin, tmax, t, normal);
    return intersect_box(ray, primitive, tmin, tmax, t, normal);
}

// Closest hit over all primitives (linear scan; acceleration structure is out of scope).
bool intersect_scene(RAY ray, float tmin, float tmax, OUT(HIT) hit)
{
    hit.t = tmax;
    hit.position = splat3(0.0f);
    hit.normal = splat3(0.0f);
    hit.primitive = 0xffffffffu;
    hit.material = 0;
    hit.isfront_face = false;
    bool isfound = false;
    LOOP
    for (uint index = 0; index < g_frame.primitive_count; ++index)
    {
        PRIMITIVE primitive = get_primitive(index);
        float t;
        float3 normal;
        if (intersect_primitive(ray, primitive, tmin, hit.t, t, normal))
        {
            isfound = true;
            hit.t = t;
            hit.normal = normal;
            hit.primitive = index;
            hit.material = primitive.material;
        }
    }
    if (isfound)
    {
        hit.position = ray.origin + hit.t * ray.direction;
        hit.isfront_face = dot(ray.direction, hit.normal) < 0.0f;
    }
    return isfound;
}

#endif
