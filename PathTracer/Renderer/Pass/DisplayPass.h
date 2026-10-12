#pragma once

#include "../../Common/stdafx.h"

class ACCUMULATOR;
struct VIEWER_SETTINGS;

// Full-screen triangle: accumulation texture -> exposure -> tone mapping -> sRGB back buffer.
class DISPLAY_PASS
{
public:
    static constexpr DXGI_FORMAT target_format = DXGI_FORMAT_R8G8B8A8_UNORM;

    bool initialize(ID3D12Device* device, std::string& error);
    void render(ID3D12GraphicsCommandList* list, const ACCUMULATOR& accumulator, const VIEWER_SETTINGS& settings);
    void release();

private:
    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline;
};
