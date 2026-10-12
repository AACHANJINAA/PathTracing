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
};

// Requests the UI makes; the renderer applies them so it can wait for the GPU first.
struct VIEWER_REQUESTS
{
    bool isreset = false;
    bool isfullscreen_toggle = false;
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
