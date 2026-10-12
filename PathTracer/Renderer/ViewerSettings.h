#pragma once

// Settings edited by the UI. Fields that change the estimate reset accumulation
// (docs/RENDERING_MODEL.md section 10); display-only fields do not.
enum class TONEMAP : int { none = 0, reinhard = 1, aces = 2 };

struct VIEWER_SETTINGS
{
    // Display only
    float exposure = 0.0f;
    TONEMAP tonemap = TONEMAP::aces;
    bool isvsync = false;

    // Accumulation control (no reset)
    bool ispaused = false;
    unsigned target_spp = 0; // 0 = unlimited

    // View (debug views are recomputed every frame, never accumulated)
    unsigned debug_view = 0; // view_* in Shared.hlsli
    float depth_range = 0.0f; // 0 = automatic from the scene bounds

    // Integrator (changing any of these resets accumulation)
    unsigned max_depth = 8;
    unsigned russian_roulette_depth = 3;
    unsigned seed = 0;
    unsigned samples_per_frame = 1; // not an estimate change: more samples of the same estimator
};
