#include "../../Common/stdafx.h"
#include "DisplayPass.h"
#include "../Accumulator.h"
#include "../ShaderCompiler.h"
#include "../ViewerSettings.h"

namespace
{
    struct DISPLAY_CONSTANTS
    {
        float exposure;
        uint tonemap;
        uint flags;
        uint padding;
    };
    static_assert(sizeof(DISPLAY_CONSTANTS) == 16);
}

bool DISPLAY_PASS::initialize(ID3D12Device* device, std::string& error)
{
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER parameters[2]{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[0].Constants.ShaderRegister = 0;
    parameters[0].Constants.Num32BitValues = sizeof(DISPLAY_CONSTANTS) / 4;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    parameters[1].DescriptorTable.pDescriptorRanges = &range;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    const CD3DX12_ROOT_SIGNATURE_DESC root(2, parameters, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> serialized, messages;
    if (FAILED(D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &messages)))
    {
        error = messages ? static_cast<const char*>(messages->GetBufferPointer()) : "Display root signature";
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
        IID_PPV_ARGS(&_root_signature)))) { error = "CreateRootSignature (display)"; return false; }

    ComPtr<ID3DBlob> vertex, pixel;
    if (!SHADER_COMPILER::compile(L"Display.hlsl", "vs_main", "vs_5_1", vertex, error) ||
        !SHADER_COMPILER::compile(L"Display.hlsl", "ps_main", "ps_5_1", pixel, error)) return false;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC description{};
    description.pRootSignature = _root_signature.Get();
    description.VS = { vertex->GetBufferPointer(), vertex->GetBufferSize() };
    description.PS = { pixel->GetBufferPointer(), pixel->GetBufferSize() };
    description.RasterizerState = CD3DX12_RASTERIZER_DESC();
    description.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    description.BlendState = CD3DX12_BLEND_DESC();
    description.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC();
    description.DepthStencilState.DepthEnable = FALSE;
    description.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    description.SampleMask = UINT_MAX;
    description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = target_format;
    description.SampleDesc.Count = 1;
    if (FAILED(device->CreateGraphicsPipelineState(&description, IID_PPV_ARGS(&_pipeline))))
    { error = "CreateGraphicsPipelineState (display)"; return false; }
    return true;
}

void DISPLAY_PASS::render(ID3D12GraphicsCommandList* list, const ACCUMULATOR& accumulator,
    const VIEWER_SETTINGS& settings)
{
    DISPLAY_CONSTANTS constants{};
    constants.exposure = settings.exposure;
    constants.tonemap = static_cast<uint>(settings.tonemap);
    constants.flags = settings.debug_view == 5 ? 1u : 0u; // view_nan: highlight non-finite samples
    list->SetGraphicsRootSignature(_root_signature.Get());
    list->SetPipelineState(_pipeline.Get());
    ID3D12DescriptorHeap* heaps[] = { accumulator.get_heap() };
    list->SetDescriptorHeaps(1, heaps);
    list->SetGraphicsRoot32BitConstants(0, sizeof(constants) / 4, &constants, 0);
    list->SetGraphicsRootDescriptorTable(1, accumulator.get_display_table());
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->DrawInstanced(3, 1, 0, 0);
}

void DISPLAY_PASS::release()
{
    _pipeline.Reset();
    _root_signature.Reset();
}
