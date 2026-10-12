#pragma once

#include "../../Common/stdafx.h"
#include "../Shader/Shared.hlsli"

class ACCUMULATOR;

// Path tracing compute pass: root signature, PSO, per-frame constants and dispatch.
class INTEGRATOR_PASS
{
public:
    static constexpr UINT frame_count = 2;
    static constexpr UINT group_size = 8;

    bool initialize(ID3D12Device* device, std::string& error);
    void render(ID3D12GraphicsCommandList* list, UINT frame_slot, const FRAME_CONSTANTS& constants,
        const ACCUMULATOR& accumulator);
    void release();

private:
    enum ROOT_PARAMETER : UINT
    {
        root_frame_constants = 0,
        root_texture_table,
        root_parameter_count
    };

    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline;
    std::array<ComPtr<ID3D12Resource>, frame_count> _constant_buffers;
    std::array<UINT8*, frame_count> _mapped{};
};

static_assert(sizeof(FRAME_CONSTANTS) % 16 == 0, "FRAME_CONSTANTS rows must be 16 bytes");
