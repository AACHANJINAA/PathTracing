#include "../Common/stdafx.h"
#include "CameraController.h"
#include "../ThirdParty/ImGui/imgui.h"

namespace
{
    constexpr float pitch_limit = 1.55f; // ~89 degrees
    constexpr float mouse_sensitivity = 0.004f;
}

void CAMERA_CONTROLLER::reset(const SCENE_CAMERA& camera, float scene_radius)
{
    _position = camera.position;
    float3 f{ camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z };
    const float length = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
    f = { f.x / length, f.y / length, f.z / length };
    _yaw = std::atan2(f.x, -f.z);
    _pitch = std::asin((std::clamp)(f.y, -1.0f, 1.0f));
    _fov_y = camera.fov_y;
    _speed = (std::max)(scene_radius * 0.5f, 0.1f);
}

float3 CAMERA_CONTROLLER::forward() const
{
    return { std::cos(_pitch) * std::sin(_yaw), std::sin(_pitch), -std::cos(_pitch) * std::cos(_yaw) };
}

bool CAMERA_CONTROLLER::update(float delta_time, bool ismouse_free, bool iskeyboard_free)
{
    const auto& io = ImGui::GetIO();
    bool ischanged = false;
    if (ismouse_free && ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
    {
        _yaw += io.MouseDelta.x * mouse_sensitivity;
        _pitch = (std::clamp)(_pitch - io.MouseDelta.y * mouse_sensitivity, -pitch_limit, pitch_limit);
        ischanged = true;
    }
    if (iskeyboard_free)
    {
        const float3 f = forward();
        const float3 right{ -f.z, 0.0f, f.x }; // forward x up, normalized in the horizontal plane
        const float right_length = (std::max)(std::sqrt(right.x * right.x + right.z * right.z), 1e-6f);
        float3 move{};
        auto add = [&](float3 v, float s) { move = { move.x + v.x * s, move.y + v.y * s, move.z + v.z * s }; };
        if (ImGui::IsKeyDown(ImGuiKey_W)) add(f, 1.0f);
        if (ImGui::IsKeyDown(ImGuiKey_S)) add(f, -1.0f);
        if (ImGui::IsKeyDown(ImGuiKey_D)) add(right, 1.0f / right_length);
        if (ImGui::IsKeyDown(ImGuiKey_A)) add(right, -1.0f / right_length);
        if (ImGui::IsKeyDown(ImGuiKey_E)) add({ 0.0f, 1.0f, 0.0f }, 1.0f);
        if (ImGui::IsKeyDown(ImGuiKey_Q)) add({ 0.0f, 1.0f, 0.0f }, -1.0f);
        if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f)
        {
            const float step = _speed * delta_time * (io.KeyShift ? 4.0f : 1.0f);
            _position = { _position.x + move.x * step, _position.y + move.y * step, _position.z + move.z * step };
            ischanged = true;
        }
    }
    return ischanged;
}

SCENE_CAMERA CAMERA_CONTROLLER::get_camera() const
{
    const float3 f = forward();
    SCENE_CAMERA camera;
    camera.position = _position;
    camera.target = { _position.x + f.x, _position.y + f.y, _position.z + f.z };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fov_y = _fov_y;
    return camera;
}

CAMERA_BASIS CAMERA_CONTROLLER::get_basis() const
{
    return make_camera_basis(get_camera());
}
