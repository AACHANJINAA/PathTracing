#include "../Common/stdafx.h"
#include "Accumulator.h"

bool ACCUMULATOR::initialize(ID3D12Device* device)
{
    CD3DX12_DESCRIPTOR_HEAP_DESC heap(3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&_heap)))) return false;
    _descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Slot 0 stays a valid null Texture2D SRV until an environment map is bound.
    D3D12_SHADER_RESOURCE_VIEW_DESC null_view{};
    null_view.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    null_view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    null_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    null_view.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(nullptr, &null_view, get_cpu_handle(0));
    return true;
}

bool ACCUMULATOR::resize(ID3D12Device* device, UINT width, UINT height)
{
    _texture.Reset();
    _width = (std::max)(width, 1u);
    _height = (std::max)(height, 1u);
    reset();

    const CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_DEFAULT);
    const auto description = CD3DX12_RESOURCE_DESC::texture_2d(format, _width, _height,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    // The texture lives in UNORDERED_ACCESS between frames; display transitions it temporarily.
    if (FAILED(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &description,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&_texture)))) return false;

    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered{};
    unordered.Format = format;
    unordered.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_texture.Get(), nullptr, &unordered, get_cpu_handle(1));

    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(_texture.Get(), &view, get_cpu_handle(2));
    return true;
}

void ACCUMULATOR::release()
{
    _texture.Reset();
    _heap.Reset();
}

D3D12_CPU_DESCRIPTOR_HANDLE ACCUMULATOR::get_cpu_handle(UINT slot) const
{
    auto handle = _heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(slot) * _descriptor_size;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ACCUMULATOR::get_gpu_handle(UINT slot) const
{
    auto handle = _heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(slot) * _descriptor_size;
    return handle;
}
