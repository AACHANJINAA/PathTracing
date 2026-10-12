#pragma once

#include "../Common/stdafx.h"

// Owns the R32G32B32A32_FLOAT accumulation texture (running mean of radiance)
// and the shader-visible descriptor heap used by the integrator and display passes.
//
// Descriptor slots:
//   0: environment SRV (t3, null until an environment map exists)
//   1: accumulation UAV (u0)   -> integrator table = slots 0..1
//   2: accumulation SRV (t0)   -> display table    = slot 2
class ACCUMULATOR
{
public:
    static constexpr DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;

    bool initialize(ID3D12Device* device);
    bool resize(ID3D12Device* device, UINT width, UINT height);
    void release();

    void reset() { _sample_count = 0; _frame_index = 0; }
    void advance(UINT samples) { _sample_count += samples; ++_frame_index; }

    UINT get_width() const { return _width; }
    UINT get_height() const { return _height; }
    UINT get_sample_count() const { return _sample_count; }
    UINT get_frame_index() const { return _frame_index; }
    ID3D12Resource* get_texture() const { return _texture.Get(); }
    ID3D12DescriptorHeap* get_heap() const { return _heap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(UINT slot) const;
    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(UINT slot) const;
    D3D12_GPU_DESCRIPTOR_HANDLE get_integrator_table() const { return get_gpu_handle(0); }
    D3D12_GPU_DESCRIPTOR_HANDLE get_display_table() const { return get_gpu_handle(2); }

private:
    ComPtr<ID3D12DescriptorHeap> _heap;
    ComPtr<ID3D12Resource> _texture;
    UINT _descriptor_size = 0;
    UINT _width = 0;
    UINT _height = 0;
    UINT _sample_count = 0;
    UINT _frame_index = 0;
};
