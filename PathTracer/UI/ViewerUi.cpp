#include "../Common/stdafx.h"
#include "ViewerUi.h"
#include "UiTheme.h"
#include "../ThirdParty/ImGui/imgui.h"
#include "../ThirdParty/ImGui/backends/imgui_impl_win32.h"
#include "../ThirdParty/ImGui/backends/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool VIEWER_UI::initialize(HWND window, ID3D12Device* device, ID3D12CommandQueue* queue, int frame_count)
{
    IMGUI_CHECKVERSION();
    _context = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.Fonts->AddFontDefault();
    apply_viewer_theme();

    CD3DX12_DESCRIPTOR_HEAP_DESC heap(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&_heap)))) { shutdown(); return false; }

    _is_win32_ready = ImGui_ImplWin32_Init(window);
    if (!_is_win32_ready) { shutdown(); return false; }
    ImGui_ImplDX12_InitInfo info{};
    info.Device = device;
    info.CommandQueue = queue;
    info.NumFramesInFlight = frame_count;
    info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    info.SrvDescriptorHeap = _heap.Get();
    // ImGui 1.91.9b allocates a single font descriptor.
    info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* data,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
    {
        *cpu = data->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        *gpu = data->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    };
    info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {};
    _is_dx12_ready = ImGui_ImplDX12_Init(&info);
    if (!_is_dx12_ready || !ImGui_ImplDX12_CreateDeviceObjects()) { shutdown(); return false; }
    return true;
}

void VIEWER_UI::begin_frame()
{
    if (_is_frame_open) ImGui::EndFrame();
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    _is_frame_open = true;
}

VIEWER_REQUESTS VIEWER_UI::draw(VIEWER_SETTINGS& settings, const VIEWER_STATS& stats)
{
    VIEWER_REQUESTS requests;
    if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) requests.isfullscreen_toggle = true;

    ImGui::SetNextWindowPos({ 10, 10 }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 300, 0 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("PATH TRACER", nullptr, ImGuiWindowFlags_NoSavedSettings))
    {
        const auto& io = ImGui::GetIO();
        ImGui::Text("%.0f FPS  |  %.2f ms", io.Framerate, io.Framerate > 0 ? 1000.0f / io.Framerate : 0.0f);
        ImGui::Text("%u x %u", stats.width, stats.height);
        ImGui::Text("Frame %u   Samples %u spp", stats.frame_index, stats.sample_count);

        ImGui::SeparatorText("ACCUMULATION");
        ImGui::Checkbox("Pause", &settings.ispaused);
        ImGui::SameLine();
        if (ImGui::Button("RESET")) requests.isreset = true;
        int target = static_cast<int>(settings.target_spp);
        if (ImGui::InputInt("Target spp", &target, 64, 1024)) settings.target_spp = static_cast<unsigned>((std::max)(target, 0));

        ImGui::SeparatorText("DISPLAY");
        ImGui::SliderFloat("Exposure", &settings.exposure, -8.0f, 8.0f, "%.2f EV");
        const char* tonemaps[] = { "None (clamp)", "Reinhard", "ACES fitted" };
        int tonemap = static_cast<int>(settings.tonemap);
        if (ImGui::Combo("Tonemap", &tonemap, tonemaps, 3)) settings.tonemap = static_cast<TONEMAP>(tonemap);
        ImGui::Checkbox("VSync", &settings.isvsync);
        ImGui::TextDisabled("F11: fullscreen");
    }
    ImGui::End();
    return requests;
}

void VIEWER_UI::render(ID3D12GraphicsCommandList* list)
{
    ImGui::Render();
    _is_frame_open = false;
    ID3D12DescriptorHeap* heaps[] = { _heap.Get() };
    list->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);
}

bool VIEWER_UI::process_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (!_context || !_is_win32_ready) return false;
    return ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam) != 0;
}

bool VIEWER_UI::wants_mouse() const { return _context && ImGui::GetIO().WantCaptureMouse; }
bool VIEWER_UI::wants_keyboard() const { return _context && ImGui::GetIO().WantCaptureKeyboard; }

void VIEWER_UI::shutdown()
{
    if (!_context) return;
    if (_is_frame_open) ImGui::EndFrame();
    if (_is_dx12_ready) ImGui_ImplDX12_Shutdown();
    if (_is_win32_ready) ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(_context);
    _context = nullptr;
    _is_dx12_ready = _is_win32_ready = _is_frame_open = false;
    _heap.Reset();
}
