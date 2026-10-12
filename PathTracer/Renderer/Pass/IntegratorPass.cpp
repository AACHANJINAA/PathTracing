#include "../../Common/stdafx.h"
#include "IntegratorPass.h"
#include "../Accumulator.h"
#include "../ShaderCompiler.h"

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
    return true;
}

void INTEGRATOR_PASS::render(ID3D12GraphicsCommandList* list, UINT frame_slot,
    const FRAME_CONSTANTS& constants, const ACCUMULATOR& accumulator)
{
    std::memcpy(_mapped[frame_slot], &constants, sizeof(constants));
    list->SetComputeRootSignature(_root_signature.Get());
    list->SetPipelineState(_pipeline.Get());
    ID3D12DescriptorHeap* heaps[] = { accumulator.get_heap() };
    list->SetDescriptorHeaps(1, heaps);
    list->SetComputeRootConstantBufferView(root_frame_constants, _constant_buffers[frame_slot]->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(root_texture_table, accumulator.get_integrator_table());
    list->Dispatch((constants.width + group_size - 1) / group_size,
        (constants.height + group_size - 1) / group_size, 1);
}

void INTEGRATOR_PASS::release()
{
    for (UINT index = 0; index < frame_count; ++index)
    {
        if (_constant_buffers[index] && _mapped[index]) _constant_buffers[index]->Unmap(0, nullptr);
        _mapped[index] = nullptr;
        _constant_buffers[index].Reset();
    }
    _pipeline.Reset();
    _root_signature.Reset();
}
