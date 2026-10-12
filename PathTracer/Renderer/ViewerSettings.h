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
};
