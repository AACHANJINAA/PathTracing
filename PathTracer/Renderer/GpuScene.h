#pragma once

#include "../Common/stdafx.h"
#include "../Scene/Scene.h"

// Uploads SCENE primitives/materials into DEFAULT-heap structured buffers (t0, t1).
// Buffers are created in COMMON and rely on implicit promotion (copy) and decay, then
// implicit promotion to NON_PIXEL_SHADER_RESOURCE when the integrator reads them.
// The caller must make sure the GPU no longer uses the previous buffers (wait_for_gpu).
class GPU_SCENE
{
public:
    bool upload(ID3D12Device* device, ID3D12CommandQueue* queue, const SCENE& scene, std::string& error);
    void release();

    D3D12_GPU_VIRTUAL_ADDRESS get_primitives() const { return _primitives->GetGPUVirtualAddress(); }
    D3D12_GPU_VIRTUAL_ADDRESS get_materials() const { return _materials->GetGPUVirtualAddress(); }
    UINT get_primitive_count() const { return _primitive_count; }

private:
    bool create_buffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heap, ComPtr<ID3D12Resource>& buffer);

    ComPtr<ID3D12Resource> _primitives;
    ComPtr<ID3D12Resource> _materials;
    UINT _primitive_count = 0;
};
