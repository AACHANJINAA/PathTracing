#pragma once

#include "../Common/stdafx.h"
#include "../Scene/Scene.h"

// Free-fly camera driven by ImGui input state (docs/RENDERER_SPEC.md 3.4).
// Right mouse drag rotates, W/A/S/D move, Q/E move down/up, Shift is faster.
class CAMERA_CONTROLLER
{
public:
    void reset(const SCENE_CAMERA& camera, float scene_radius);
    // Returns true when the camera moved (accumulation must reset).
    bool update(float delta_time, bool ismouse_free, bool iskeyboard_free);
    CAMERA_BASIS get_basis() const;
    SCENE_CAMERA get_camera() const;
    void set_fov(float fov_y) { _fov_y = fov_y; }
    float get_fov() const { return _fov_y; }

private:
    float3 forward() const;

    float3 _position{};
    float _yaw = 0.0f;    // radians, 0 looks down -Z, positive turns toward +X
    float _pitch = 0.0f;  // radians, positive looks up
    float _fov_y = 45.0f;
    float _speed = 1.0f;
};
