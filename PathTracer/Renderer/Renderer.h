#pragma once

#include "../Common/stdafx.h"
#include "Accumulator.h"
#include "CameraController.h"
#include "GpuScene.h"
#include "ViewerSettings.h"
#include "Pass/IntegratorPass.h"
#include "Pass/DisplayPass.h"
#include "../UI/ViewerUi.h"
#include "../Scene/Scene.h"

// Window, device, swap chain and frame orchestration (docs/ARCHITECTURE.md section 3).
class RENDERER
{
public:
    RENDERER() = default;
    ~RENDERER();
    RENDERER(const RENDERER&) = delete;
    RENDERER& operator=(const RENDERER&) = delete;

    bool initialize(HINSTANCE instance, int show_command);
    int run();

private:
    static constexpr UINT frame_count = 2;
    static constexpr wchar_t window_class_name[] = L"PathTracerWindow";
    static LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    bool fail(const char* stage, const std::string& detail = {});
    bool create_window(int show_command);
    bool create_device();
    bool create_command_objects();
    bool create_swap_chain();
    bool create_render_targets();
    bool resize_swap_chain(UINT width, UINT height);
    void toggle_fullscreen();
    void render_frame();
    void discover_scenes();
    bool load_scene(int index, bool isinitial);
    void apply_requests(const VIEWER_REQUESTS& requests);
    bool is_estimate_changed(const VIEWER_SETTINGS& previous) const;
    bool wait_for_gpu();
    bool move_to_next_frame();
    void shutdown();

    HINSTANCE _instance = nullptr;
    HWND _window = nullptr;
    ComPtr<IDXGIFactory4> _factory;
    ComPtr<ID3D12Device> _device;
    ComPtr<ID3D12CommandQueue> _command_queue;
    ComPtr<IDXGISwapChain3> _swap_chain;
    ComPtr<ID3D12DescriptorHeap> _render_target_heap;
    std::array<ComPtr<ID3D12Resource>, frame_count> _render_targets;
    UINT _render_target_descriptor_size = 0;
    std::array<ComPtr<ID3D12CommandAllocator>, frame_count> _allocators;
    std::array<UINT64, frame_count> _frame_fences{};
    ComPtr<ID3D12GraphicsCommandList> _command_list;
    ComPtr<ID3D12Fence> _fence;
    UINT64 _fence_value = 0;
    HANDLE _fence_event = nullptr;
    UINT _frame_index = 0;

    ACCUMULATOR _accumulator;
    INTEGRATOR_PASS _integrator_pass;
    DISPLAY_PASS _display_pass;
    VIEWER_UI _ui;
    VIEWER_SETTINGS _settings;
    VIEWER_SETTINGS _previous_settings;

    SCENE _scene;
    GPU_SCENE _gpu_scene;
    CAMERA_CONTROLLER _camera;
    std::vector<std::filesystem::path> _scene_files;
    std::vector<std::string> _scene_names;
    int _scene_index = -1;
    UINT64 _last_tick = 0;

    bool _is_failed = false;
    bool _is_fullscreen = false;
    RECT _windowed_rect{};
    LONG_PTR _windowed_style = WS_OVERLAPPEDWINDOW;
};
