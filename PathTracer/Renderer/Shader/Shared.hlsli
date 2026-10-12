#ifndef SHARED_HLSLI
#define SHARED_HLSLI

// Structures and constants shared by HLSL, the Windows app and the CPU kernel (Tests/).
// C++ includes this after Common/HlslTypes.h.
// FRAME_CONSTANTS is a constant buffer: keep every row 16 bytes (float3 + 4-byte scalar).
// PRIMITIVE / MATERIAL are structured buffer elements: tightly packed, 4-byte aligned.

static const uint primitive_sphere = 0;
static const uint primitive_quad = 1;
static const uint primitive_box = 2;

static const uint material_diffuse = 0;
static const uint material_emissive = 1;

static const uint view_lit = 0;
static const uint view_normal = 1;
static const uint view_albedo = 2;
static const uint view_depth = 3;
static const uint view_path_length = 4;
static const uint view_nan = 5;          // lit accumulation + non-finite samples shown in magenta

static const uint sampling_bsdf = 0;     // BSDF sampling only (emitters hit by chance)

struct PRIMITIVE
{
    uint type;
    uint material;
    float area;          // surface area (used by light sampling)
    float radius;        // sphere
    float3 position;     // sphere center / quad corner / box center
    float3 edge_u;       // quad edge u / box local x axis (unit)
    float3 edge_v;       // quad edge v / box local y axis (unit)
    float3 normal;       // quad unit normal = normalize(cross(u, v)) / box local z axis (unit)
    float3 half_size;    // box half extents along its local axes
};

struct MATERIAL
{
    uint type;
    float roughness;
    float ior;
    float padding;
    float3 albedo;       // linear RGB reflectance
    float3 emission;     // linear RGB radiance (emissive only)
};

struct FRAME_CONSTANTS
{
    float3 camera_position;
    float tan_half_fov_y;
    float3 camera_right;
    float aspect;
    float3 camera_up;
    uint width;
    float3 camera_forward;
    uint height;

    uint frame_index;
    uint sample_count;          // samples already accumulated before this dispatch
    uint primitive_count;
    uint debug_view;

    float depth_range;
    uint seed;
    uint max_depth;             // maximum number of path segments (1 = emitters only)
    uint samples_per_frame;

    uint russian_roulette_depth;
    uint sampling_mode;
    uint padding0;
    uint padding1;
};

#endif
