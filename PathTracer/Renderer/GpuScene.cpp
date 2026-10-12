#include "../Common/stdafx.h"
#include "GpuScene.h"

namespace
{
    // Packs a vector into bytes; empty arrays still get one zeroed element so root SRVs stay valid.
    template <typename T>
    std::vector<UINT8> pack(const std::vector<T>& items)
    {
        std::vector<UINT8> bytes((std::max)(items.size(), size_t{ 1 }) * sizeof(T), 0);
        if (!items.empty()) std::memcpy(bytes.data(), items.data(), items.size() * sizeof(T));
        return bytes;
    }
}

bool GPU_SCENE::create_buffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heap, ComPtr<ID3D12Resource>& buffer)
{
    const CD3DX12_HEAP_PROPERTIES properties(heap);
    const auto description = CD3DX12_RESOURCE_DESC::buffer(size);
    const D3D12_RESOURCE_STATES state = heap == D3D12_HEAP_TYPE_UPLOAD
        ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
    return SUCCEEDED(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &description,
        state, nullptr, IID_PPV_ARGS(&buffer)));
}

bool GPU_SCENE::upload(ID3D12Device* device, ID3D12CommandQueue* queue, const SCENE& scene, std::string& error)
{
    const std::vector<UINT8> blobs[2] = { pack(scene.primitives), pack(scene.materials) };
    ComPtr<ID3D12Resource> targets[2];
    ComPtr<ID3D12Resource> staging[2];
    for (int index = 0; index < 2; ++index)
    {
        const UINT64 size = blobs[index].size();
        if (!create_buffer(device, size, D3D12_HEAP_TYPE_DEFAULT, targets[index]) ||
            !create_buffer(device, size, D3D12_HEAP_TYPE_UPLOAD, staging[index]))
        { error = "Could not create scene buffers"; return false; }
        void* mapped = nullptr;
        const D3D12_RANGE read{ 0, 0 };
        if (FAILED(staging[index]->Map(0, &read, &mapped))) { error = "Could not map scene upload buffer"; return false; }
        std::memcpy(mapped, blobs[index].data(), static_cast<size_t>(size));
        staging[index]->Unmap(0, nullptr);
    }

    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))) ||
        FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list))) ||
        FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
    { error = "Could not create scene upload commands"; return false; }
    for (int index = 0; index < 2; ++index)
        list->CopyBufferRegion(targets[index].Get(), 0, staging[index].Get(), 0, blobs[index].size());
    if (FAILED(list->Close())) { error = "Could not close scene upload commands"; return false; }
    ID3D12CommandList* lists[] = { list.Get() };
    queue->ExecuteCommandLists(1, lists);

    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event) { error = "CreateEvent failed"; return false; }
    const bool isdone = SUCCEEDED(queue->Signal(fence.Get(), 1)) &&
        SUCCEEDED(fence->SetEventOnCompletion(1, event)) && WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0;
    CloseHandle(event);
    if (!isdone) { error = "Scene upload did not complete"; return false; }

    _primitives = targets[0];
    _materials = targets[1];
    _primitive_count = static_cast<UINT>(scene.primitives.size());
    return true;
}

void GPU_SCENE::release()
{
    _primitives.Reset();
    _materials.Reset();
    _primitive_count = 0;
}
