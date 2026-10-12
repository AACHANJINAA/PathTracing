#pragma once

#include "../../Common/stdafx.h"
#include "../Shader/Shared.hlsli"

class ACCUMULATOR;
class GPU_SCENE;

// Path tracing compute pass: root signature, PSO, per-frame constants and dispatch.
class INTEGRATOR_PASS
{
public:
    static constexpr UINT frame_count = 2;
    static constexpr UINT group_size = 8;

    bool initialize(ID3D12Device* device, std::string& error);
    void render(ID3D12GraphicsCommandList* list, UINT frame_slot, const FRAME_CONSTANTS& constants,
        const ACCUMULATOR& accumulator, const GPU_SCENE& scene);
    void release();
    // Non-finite samples since the last reset, read back from the frame that last used this slot.
    UINT get_invalid_samples() const { return _invalid_samples; }

private:
    enum ROOT_PARAMETER : UINT
    {
        root_frame_constants = 0,
        root_texture_table,
        root_primitives,
        root_materials,
        root_counters,
        root_parameter_count
    };

    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline;
    std::array<ComPtr<ID3D12Resource>, frame_count> _constant_buffers;
    std::array<UINT8*, frame_count> _mapped{};

    // u1 counter buffer: kept in COMMON between frames, explicit transitions inside render().
    ComPtr<ID3D12Resource> _counters;
    ComPtr<ID3D12Resource> _counter_zero;
    std::array<ComPtr<ID3D12Resource>, frame_count> _counter_readback;
    std::array<bool, frame_count> _is_readback_pending{};
    std::array<UINT, frame_count> _readback_generation{};
    UINT _generation = 0; // incremented on every accumulation reset; stale readbacks are ignored
    UINT _invalid_samples = 0;
};

static_assert(sizeof(FRAME_CONSTANTS) % 16 == 0, "FRAME_CONSTANTS rows must be 16 bytes");
