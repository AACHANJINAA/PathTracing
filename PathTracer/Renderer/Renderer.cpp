#include "../Common/stdafx.h"
#include "Renderer.h"
#include "ShaderCompiler.h"
#include "../Scene/SceneLoader.h"

namespace
{
    UINT client_width(HWND window)
    {
        RECT rect{};
        GetClientRect(window, &rect);
        return static_cast<UINT>((std::max)(rect.right - rect.left, 0L));
    }

    UINT client_height(HWND window)
    {
        RECT rect{};
        GetClientRect(window, &rect);
        return static_cast<UINT>((std::max)(rect.bottom - rect.top, 0L));
    }

    std::filesystem::path executable_directory()
    {
        return SHADER_COMPILER::get_shader_directory().parent_path();
    }
}

RENDERER::~RENDERER()
{
    shutdown();
}

bool RENDERER::fail(const char* stage, const std::string& detail)
{
    std::string message = std::string("Initialization failed: ") + stage;
    if (!detail.empty()) message += "\n\n" + detail;
    OutputDebugStringA((message + "\n").c_str());
    std::fprintf(stderr, "%s\n", message.c_str());
    MessageBoxA(_window, message.c_str(), "PathTracer", MB_OK | MB_ICONERROR);
    _is_failed = true;
    return false;
}

bool RENDERER::initialize(HINSTANCE instance, int show_command)
{
    _instance = instance;
    if (!create_window(show_command)) return fail("create_window");
    if (!create_device()) return fail("create_device");
    if (!create_command_objects()) return fail("create_command_objects");
    if (!create_swap_chain()) return fail("create_swap_chain");
    if (!create_render_targets()) return fail("create_render_targets");
    if (FAILED(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)))) return fail("CreateFence");
    _fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!_fence_event) return fail("CreateEvent");

    std::string error;
    if (!_accumulator.initialize(_device.Get())) return fail("accumulator.initialize");
    if (!_accumulator.resize(_device.Get(), client_width(_window), client_height(_window)))
        return fail("accumulator.resize");
    if (!_integrator_pass.initialize(_device.Get(), error)) return fail("integrator_pass.initialize", error);
    if (!_display_pass.initialize(_device.Get(), error)) return fail("display_pass.initialize", error);
    if (!_ui.initialize(_window, _device.Get(), _command_queue.Get(), frame_count)) return fail("viewer_ui.initialize");

    discover_scenes();
    int initial = 0;
    for (int index = 0; index < static_cast<int>(_scene_names.size()); ++index)
        if (_scene_names[index] == "cornell_box") initial = index;
    if (_scene_files.empty()) return fail("scene", "No scene files in " + (executable_directory() / "Scenes").string());
    if (!load_scene(initial, true)) return false;
    _previous_settings = _settings;
    return true;
}

int RENDERER::run()
{
    MSG message{};
    while (true)
    {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) return static_cast<int>(message.wParam);
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (IsIconic(_window))
        {
            WaitMessage();
            continue;
        }
        render_frame();
        if (_is_failed) return EXIT_FAILURE;
    }
}

bool RENDERER::create_window(int show_command)
{
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.hInstance = _instance;
    window_class.lpfnWndProc = window_procedure;
    window_class.lpszClassName = window_class_name;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    RECT rect{ 0, 0, 1280, 720 };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    _window = CreateWindowExW(0, window_class_name, L"PathTracer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, _instance, this);
    if (!_window) return false;
    ShowWindow(_window, show_command);
    UpdateWindow(_window);
    return true;
}

bool RENDERER::create_device()
{
    UINT factory_flags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
    {
        debug->EnableDebugLayer();
        factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    if (FAILED(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&_factory)))) return false;
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT index = 0; _factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index)
    {
        DXGI_ADAPTER_DESC1 description{};
        adapter->GetDesc1(&description);
        if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)))) return true;
    }
    return SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
}

bool RENDERER::create_command_objects()
{
    CD3DX12_COMMAND_QUEUE_DESC queue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (FAILED(_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&_command_queue)))) return false;
    for (auto& allocator : _allocators)
        if (FAILED(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)))) return false;
    if (FAILED(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _allocators[0].Get(), nullptr,
        IID_PPV_ARGS(&_command_list)))) return false;
    return SUCCEEDED(_command_list->Close());
}

bool RENDERER::create_swap_chain()
{
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.BufferCount = frame_count;
    description.Format = DISPLAY_PASS::target_format;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    description.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> swap_chain;
    if (FAILED(_factory->CreateSwapChainForHwnd(_command_queue.Get(), _window, &description, nullptr, nullptr, &swap_chain)) ||
        FAILED(_factory->MakeWindowAssociation(_window, DXGI_MWA_NO_ALT_ENTER)) ||
        FAILED(swap_chain.As(&_swap_chain))) return false;
    _frame_index = _swap_chain->GetCurrentBackBufferIndex();
    return true;
}

bool RENDERER::create_render_targets()
{
    if (!_render_target_heap)
    {
        CD3DX12_DESCRIPTOR_HEAP_DESC heap(frame_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        if (FAILED(_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&_render_target_heap)))) return false;
        _render_target_descriptor_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    auto handle = _render_target_heap->GetCPUDescriptorHandleForHeapStart();
    for (UINT index = 0; index < frame_count; ++index)
    {
        if (FAILED(_swap_chain->GetBuffer(index, IID_PPV_ARGS(&_render_targets[index])))) return false;
        _device->CreateRenderTargetView(_render_targets[index].Get(), nullptr, handle);
        handle.ptr += _render_target_descriptor_size;
    }
    return true;
}

bool RENDERER::resize_swap_chain(UINT width, UINT height)
{
    if (!wait_for_gpu()) return false;
    for (auto& target : _render_targets) target.Reset();
    if (FAILED(_swap_chain->ResizeBuffers(frame_count, width, height, DISPLAY_PASS::target_format, 0))) return false;
    _frame_index = _swap_chain->GetCurrentBackBufferIndex();
    return create_render_targets() && _accumulator.resize(_device.Get(), width, height);
}

void RENDERER::toggle_fullscreen()
{
    if (!_is_fullscreen)
    {
        GetWindowRect(_window, &_windowed_rect);
        _windowed_style = GetWindowLongPtrW(_window, GWL_STYLE);
        MONITORINFO monitor{ sizeof(MONITORINFO) };
        GetMonitorInfoW(MonitorFromWindow(_window, MONITOR_DEFAULTTONEAREST), &monitor);
        SetWindowLongPtrW(_window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(_window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
            monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        SetWindowLongPtrW(_window, GWL_STYLE, _windowed_style);
        SetWindowPos(_window, HWND_TOP, _windowed_rect.left, _windowed_rect.top,
            _windowed_rect.right - _windowed_rect.left, _windowed_rect.bottom - _windowed_rect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    _is_fullscreen = !_is_fullscreen;
}

void RENDERER::discover_scenes()
{
    _scene_files.clear();
    _scene_names.clear();
    std::error_code code;
    const auto directory = executable_directory() / "Scenes";
    for (const auto& entry : std::filesystem::directory_iterator(directory, code))
        if (entry.is_regular_file() && entry.path().extension() == ".json") _scene_files.push_back(entry.path());
    std::sort(_scene_files.begin(), _scene_files.end());
    for (const auto& path : _scene_files) _scene_names.push_back(path.stem().string());
}

bool RENDERER::load_scene(int index, bool isinitial)
{
    if (index < 0 || index >= static_cast<int>(_scene_files.size())) return false;
    SCENE loaded;
    std::string error;
    if (!SCENE_LOADER::load_file(_scene_files[index], loaded, error))
    {
        // V-SCENE: report and keep the current scene.
        if (isinitial) return fail("scene load", error);
        MessageBoxA(_window, error.c_str(), "Scene load failed", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!wait_for_gpu()) return fail("wait_for_gpu (scene)");
    if (!_gpu_scene.upload(_device.Get(), _command_queue.Get(), loaded, error))
    {
        if (isinitial) return fail("scene upload", error);
        MessageBoxA(_window, error.c_str(), "Scene upload failed", MB_OK | MB_ICONWARNING);
        return false;
    }
    _scene = std::move(loaded);
    _scene_index = index;
    _camera.reset(_scene.camera, _scene.bounding_radius());
    _accumulator.reset();
    SetWindowTextA(_window, ("PathTracer - " + _scene.name).c_str());
    return true;
}

bool RENDERER::is_estimate_changed(const VIEWER_SETTINGS& previous) const
{
    return previous.debug_view != _settings.debug_view || previous.depth_range != _settings.depth_range;
}

void RENDERER::apply_requests(const VIEWER_REQUESTS& requests)
{
    if (requests.isfullscreen_toggle) toggle_fullscreen(); // new size is picked up next frame
    if (requests.load_scene_index >= 0) load_scene(requests.load_scene_index, false);
    if (requests.isreload_scene)
    {
        const auto current = _scene_index >= 0 ? _scene_files[_scene_index] : std::filesystem::path{};
        discover_scenes();
        const auto found = std::find(_scene_files.begin(), _scene_files.end(), current);
        load_scene(found != _scene_files.end() ? static_cast<int>(found - _scene_files.begin()) : 0, false);
    }
    if (requests.isreset_camera) { _camera.reset(_scene.camera, _scene.bounding_radius()); _accumulator.reset(); }
    if (requests.fov_y > 0.0f) { _camera.set_fov(requests.fov_y); _accumulator.reset(); }
    if (requests.isreset) _accumulator.reset();
}

void RENDERER::render_frame()
{
    const UINT width = client_width(_window);
    const UINT height = client_height(_window);
    if (width == 0 || height == 0) return;
    const auto back_buffer = _render_targets[0]->GetDesc();
    if (back_buffer.Width != width || back_buffer.Height != height)
    {
        if (!resize_swap_chain(width, height)) { fail("resize_swap_chain"); return; }
    }

    const UINT64 tick = GetTickCount64();
    const float delta_time = _last_tick ? (std::min)((tick - _last_tick) / 1000.0f, 0.1f) : 0.0f;
    _last_tick = tick;

    _ui.begin_frame();
    VIEWER_STATS stats{ width, height, _accumulator.get_frame_index(), _accumulator.get_sample_count() };
    stats.scene_names = &_scene_names;
    stats.scene_index = _scene_index;
    stats.primitive_count = _gpu_scene.get_primitive_count();
    stats.fov_y = _camera.get_fov();
    const VIEWER_REQUESTS requests = _ui.draw(_settings, stats);
    apply_requests(requests);
    if (_is_failed) return;
    if (_camera.update(delta_time, !_ui.wants_mouse(), !_ui.wants_keyboard())) _accumulator.reset();
    if (is_estimate_changed(_previous_settings)) _accumulator.reset();
    _previous_settings = _settings;
    // M1: no accumulation yet; every frame is a fresh single-sample image.
    _accumulator.reset();

    // Always render the first sample after a reset so the display never shows stale data.
    const UINT samples = 1;
    const bool isbelow_target = _settings.target_spp == 0 || _accumulator.get_sample_count() < _settings.target_spp;
    const bool isaccumulating = isbelow_target && (!_settings.ispaused || _accumulator.get_sample_count() == 0);

    const CAMERA_BASIS camera = _camera.get_basis();
    FRAME_CONSTANTS constants{};
    constants.camera_position = camera.position;
    constants.camera_right = camera.right;
    constants.camera_up = camera.up;
    constants.camera_forward = camera.forward;
    constants.tan_half_fov_y = camera.tan_half_fov_y;
    constants.width = _accumulator.get_width();
    constants.height = _accumulator.get_height();
    constants.aspect = static_cast<float>(constants.width) / static_cast<float>(constants.height);
    constants.frame_index = _accumulator.get_frame_index();
    constants.sample_count = _accumulator.get_sample_count();
    constants.primitive_count = _gpu_scene.get_primitive_count();
    constants.debug_view = _settings.debug_view;
    constants.depth_range = _settings.depth_range > 0.0f ? _settings.depth_range : _scene.bounding_radius() * 4.0f;

    auto& allocator = _allocators[_frame_index];
    if (FAILED(allocator->Reset()) || FAILED(_command_list->Reset(allocator.Get(), nullptr)))
    { fail("command list reset"); return; }

    if (isaccumulating)
    {
        _integrator_pass.render(_command_list.Get(), _frame_index, constants, _accumulator, _gpu_scene);
        _accumulator.advance(samples);
    }

    ID3D12Resource* accumulation = _accumulator.get_texture();
    ID3D12Resource* target = _render_targets[_frame_index].Get();
    const D3D12_RESOURCE_BARRIER before[] = {
        CD3DX12_RESOURCE_BARRIER::transition(accumulation, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };
    _command_list->ResourceBarrier(2, before);

    auto rtv = _render_target_heap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(_frame_index) * _render_target_descriptor_size;
    _command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    const D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    const D3D12_RECT scissor{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    _command_list->RSSetViewports(1, &viewport);
    _command_list->RSSetScissorRects(1, &scissor);
    _display_pass.render(_command_list.Get(), _accumulator, _settings);
    _ui.render(_command_list.Get());

    const D3D12_RESOURCE_BARRIER after[] = {
        CD3DX12_RESOURCE_BARRIER::transition(accumulation, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)
    };
    _command_list->ResourceBarrier(2, after);
    if (FAILED(_command_list->Close())) { fail("command list close"); return; }
    ID3D12CommandList* lists[] = { _command_list.Get() };
    _command_queue->ExecuteCommandLists(1, lists);
    if (FAILED(_swap_chain->Present(_settings.isvsync ? 1 : 0, 0))) { fail("Present"); return; }
    if (!move_to_next_frame()) fail("move_to_next_frame");
}

bool RENDERER::move_to_next_frame()
{
    const UINT64 signal = ++_fence_value;
    if (FAILED(_command_queue->Signal(_fence.Get(), signal))) return false;
    _frame_fences[_frame_index] = signal;
    _frame_index = _swap_chain->GetCurrentBackBufferIndex();
    if (_fence->GetCompletedValue() < _frame_fences[_frame_index])
    {
        if (FAILED(_fence->SetEventOnCompletion(_frame_fences[_frame_index], _fence_event)) ||
            WaitForSingleObject(_fence_event, INFINITE) != WAIT_OBJECT_0) return false;
    }
    return SUCCEEDED(_device->GetDeviceRemovedReason());
}

bool RENDERER::wait_for_gpu()
{
    if (!_command_queue || !_fence || !_fence_event) return true;
    const UINT64 signal = ++_fence_value;
    if (FAILED(_command_queue->Signal(_fence.Get(), signal)) ||
        FAILED(_fence->SetEventOnCompletion(signal, _fence_event))) return false;
    return WaitForSingleObject(_fence_event, INFINITE) == WAIT_OBJECT_0;
}

void RENDERER::shutdown()
{
    wait_for_gpu();
    _ui.shutdown();
    _display_pass.release();
    _integrator_pass.release();
    _gpu_scene.release();
    _accumulator.release();
    if (_fence_event) { CloseHandle(_fence_event); _fence_event = nullptr; }
    _fence.Reset();
    _command_list.Reset();
    for (auto& allocator : _allocators) allocator.Reset();
    for (auto& target : _render_targets) target.Reset();
    _render_target_heap.Reset();
    _swap_chain.Reset();
    _command_queue.Reset();
    _device.Reset();
    _factory.Reset();
    if (_window) { DestroyWindow(_window); _window = nullptr; }
    if (_instance) { UnregisterClassW(window_class_name, _instance); _instance = nullptr; }
}

LRESULT CALLBACK RENDERER::window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(window, message, wparam, lparam);
    }
    auto* renderer = reinterpret_cast<RENDERER*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (renderer && renderer->_ui.process_message(window, message, wparam, lparam)) return 1;
    switch (message)
    {
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize = { 640, 400 };
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}
