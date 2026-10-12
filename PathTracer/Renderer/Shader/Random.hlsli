#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

// PCG hash / PCG-RXS-M-XS 32 (Jarzynski & Olano 2020). docs/RENDERING_MODEL.md section 9.
// Same seed, pixel and sample index always give the same sequence on GPU and CPU.

uint pcg_hash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

struct RNG
{
    uint state;
};

RNG make_rng(uint pixel_x, uint pixel_y, uint sample_index, uint seed)
{
    RNG rng;
    rng.state = pcg_hash(pixel_x + pcg_hash(pixel_y + pcg_hash(sample_index + pcg_hash(seed))));
    return rng;
}

// Uniform float in [0, 1): top 24 bits so 1.0 is never returned.
float next_float(INOUT(RNG) rng)
{
    rng.state = rng.state * 747796405u + 2891336453u;
    uint word = ((rng.state >> ((rng.state >> 28u) + 4u)) ^ rng.state) * 277803737u;
    word = (word >> 22u) ^ word;
    return float(word >> 8u) * (1.0f / 16777216.0f);
}

#endif
