#ifndef SHARED_HLSLI
#define SHARED_HLSLI

// Structures shared by HLSL and C++ (C++ includes this through Common/HlslTypes.h).
// FRAME_CONSTANTS is a constant buffer: keep every row 16 bytes (float3 + 4-byte scalar).

struct FRAME_CONSTANTS
{
    uint width;
    uint height;
    uint frame_index;
    uint sample_count;
};

#endif
