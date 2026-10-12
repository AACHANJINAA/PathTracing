#pragma once

#include "../Common/stdafx.h"
#include "../Renderer/ViewerSettings.h"

struct ImGuiContext;

struct VIEWER_STATS
{
    UINT width = 0;
    UINT height = 0;
    UINT frame_index = 0;
    UINT sample_count = 0;
    const std::vector<std::string>* scene_names = nullptr;
    int scene_index = -1;
    UINT primitive_count = 0;
    float fov_y = 45.0f;
};

// Requests the UI makes; the renderer applies them so it can wait for the GPU first.
struct VIEWER_REQUESTS
{
    bool isreset = false;
    bool isfullscreen_toggle = false;
    int load_scene_index = -1;   // >= 0: load this entry of scene_names
    bool isreload_scene = false;
    bool isreset_camera = false;
    float fov_y = -1.0f;         // > 0: new vertical field of view
};

class VIEWER_UI
{
public:
    bool initialize(HWND window, ID3D12Device* device, ID3D12CommandQueue* queue, int frame_count);
    void begin_frame();
    VIEWER_REQUESTS draw(VIEWER_SETTINGS& settings, const VIEWER_STATS& stats);
    void render(ID3D12GraphicsCommandList* list);
    void shutdown();
    bool process_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    bool wants_mouse() const;
    bool wants_keyboard() const;

private:
    ImGuiContext* _context = nullptr;
    ComPtr<ID3D12DescriptorHeap> _heap;
    bool _is_win32_ready = false;
    bool _is_dx12_ready = false;
    bool _is_frame_open = false;
};
