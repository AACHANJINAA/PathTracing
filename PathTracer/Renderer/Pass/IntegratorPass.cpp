#include "../../Common/stdafx.h"
#include "IntegratorPass.h"
#include "../Accumulator.h"
#include "../ShaderCompiler.h"
#include "../GpuScene.h"

namespace
{
    constexpr UINT64 constant_buffer_size = (sizeof(FRAME_CONSTANTS) + 255) & ~UINT64(255);
}

bool INTEGRATOR_PASS::initialize(ID3D12Device* device, std::string& error)
{
    // Table: t3 environment (slot 0), u0 accumulation (slot 1).
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 1;
    ranges[0].BaseShaderRegister = 3;
    ranges[0].OffsetInDescriptorsFromTableStart = 0;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 0;
    ranges[1].OffsetInDescriptorsFromTableStart = 1;

    D3D12_ROOT_PARAMETER parameters[root_parameter_count]{};
    parameters[root_frame_constants].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[root_frame_constants].Descriptor.ShaderRegister = 0;
    parameters[root_texture_table].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[root_texture_table].DescriptorTable.NumDescriptorRanges = 2;
    parameters[root_texture_table].DescriptorTable.pDescriptorRanges = ranges;
    parameters[root_primitives].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[root_primitives].Descriptor.ShaderRegister = 0;
    parameters[root_materials].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[root_materials].Descriptor.ShaderRegister = 1;
    parameters[root_counters].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    parameters[root_counters].Descriptor.ShaderRegister = 1;

    const CD3DX12_ROOT_SIGNATURE_DESC root(root_parameter_count, parameters, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> serialized, messages;
    if (FAILED(D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &messages)))
    {
        error = messages ? static_cast<const char*>(messages->GetBufferPointer()) : "Integrator root signature";
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
        IID_PPV_ARGS(&_root_signature)))) { error = "CreateRootSignature (integrator)"; return false; }

    ComPtr<ID3DBlob> shader;
    if (!SHADER_COMPILER::compile(L"Integrator.hlsl", "cs_main", "cs_5_1", shader, error)) return false;
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = _root_signature.Get();
    description.CS = { shader->GetBufferPointer(), shader->GetBufferSize() };
    if (FAILED(device->CreateComputePipelineState(&description, IID_PPV_ARGS(&_pipeline))))
    { error = "CreateComputePipelineState (integrator)"; return false; }

    const CD3DX12_HEAP_PROPERTIES upload(D3D12_HEAP_TYPE_UPLOAD);
    const auto buffer = CD3DX12_RESOURCE_DESC::buffer(constant_buffer_size);
    for (UINT index = 0; index < frame_count; ++index)
    {
        if (FAILED(device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constant_buffers[index]))))
        { error = "Frame constant buffer"; return false; }
        const D3D12_RANGE read{ 0, 0 };
        if (FAILED(_constant_buffers[index]->Map(0, &read, reinterpret_cast<void**>(&_mapped[index]))))
        { error = "Map frame constants"; return false; }
    }

    const CD3DX12_HEAP_PROPERTIES default_heap(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_HEAP_PROPERTIES readback_heap(D3D12_HEAP_TYPE_READBACK);
    auto counter_description = CD3DX12_RESOURCE_DESC::buffer(sizeof(UINT));
    counter_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    const auto small = CD3DX12_RESOURCE_DESC::buffer(sizeof(UINT));
    if (FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &counter_description,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_counters))) ||
        FAILED(device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &small,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_counter_zero))))
    { error = "Counter buffers"; return false; }
    void* zero = nullptr;
    if (FAILED(_counter_zero->Map(0, nullptr, &zero))) { error = "Map counter zero"; return false; }
    std::memset(zero, 0, sizeof(UINT));
    _counter_zero->Unmap(0, nullptr);
    for (auto& readback : _counter_readback)
        if (FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &small,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback))))
        { error = "Counter readback"; return false; }
    return true;
}

void INTEGRATOR_PASS::render(ID3D12GraphicsCommandList* list, UINT frame_slot,
    const FRAME_CONSTANTS& constants, const ACCUMULATOR& accumulator, const GPU_SCENE& scene)
{
    // The previous use of this slot has completed (frame fence), so its readback is valid.
    if (constants.sample_count == 0) { ++_generation; _invalid_samples = 0; }
    if (_is_readback_pending[frame_slot] && _readback_generation[frame_slot] == _generation)
    {
        void* data = nullptr;
        const D3D12_RANGE read{ 0, sizeof(UINT) };
        if (SUCCEEDED(_counter_readback[frame_slot]->Map(0, &read, &data)))
        {
            std::memcpy(&_invalid_samples, data, sizeof(UINT));
            const D3D12_RANGE written{ 0, 0 };
            _counter_readback[frame_slot]->Unmap(0, &written);
        }
    }
    _is_readback_pending[frame_slot] = false;

    std::memcpy(_mapped[frame_slot], &constants, sizeof(constants));
    if (constants.sample_count == 0)
    {
        const auto to_copy = CD3DX12_RESOURCE_BARRIER::transition(_counters.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        list->ResourceBarrier(1, &to_copy);
        list->CopyBufferRegion(_counters.Get(), 0, _counter_zero.Get(), 0, sizeof(UINT));
        const auto to_uav = CD3DX12_RESOURCE_BARRIER::transition(_counters.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        list->ResourceBarrier(1, &to_uav);
    }
    else
    {
        const auto to_uav = CD3DX12_RESOURCE_BARRIER::transition(_counters.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        list->ResourceBarrier(1, &to_uav);
    }
    list->SetComputeRootSignature(_root_signature.Get());
    list->SetPipelineState(_pipeline.Get());
    ID3D12DescriptorHeap* heaps[] = { accumulator.get_heap() };
    list->SetDescriptorHeaps(1, heaps);
    list->SetComputeRootConstantBufferView(root_frame_constants, _constant_buffers[frame_slot]->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(root_texture_table, accumulator.get_integrator_table());
    list->SetComputeRootShaderResourceView(root_primitives, scene.get_primitives());
    list->SetComputeRootShaderResourceView(root_materials, scene.get_materials());
    list->SetComputeRootUnorderedAccessView(root_counters, _counters->GetGPUVirtualAddress());
    list->Dispatch((constants.width + group_size - 1) / group_size,
        (constants.height + group_size - 1) / group_size, 1);

    const auto to_source = CD3DX12_RESOURCE_BARRIER::transition(_counters.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(1, &to_source);
    list->CopyBufferRegion(_counter_readback[frame_slot].Get(), 0, _counters.Get(), 0, sizeof(UINT));
    const auto to_common = CD3DX12_RESOURCE_BARRIER::transition(_counters.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(1, &to_common);
    _is_readback_pending[frame_slot] = true;
    _readback_generation[frame_slot] = _generation;
}

void INTEGRATOR_PASS::release()
{
    for (UINT index = 0; index < frame_count; ++index)
    {
        if (_constant_buffers[index] && _mapped[index]) _constant_buffers[index]->Unmap(0, nullptr);
        _mapped[index] = nullptr;
        _constant_buffers[index].Reset();
    }
    _counters.Reset();
    _counter_zero.Reset();
    for (auto& readback : _counter_readback) readback.Reset();
    _is_readback_pending = {};
    _pipeline.Reset();
    _root_signature.Reset();
}
