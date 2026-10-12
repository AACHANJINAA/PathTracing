#include "../Common/stdafx.h"
#include "ShaderCompiler.h"

std::filesystem::path SHADER_COMPILER::get_shader_directory()
{
    wchar_t module_path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::filesystem::path(module_path).parent_path() / L"Shaders";
}

bool SHADER_COMPILER::compile(const wchar_t* file_name, const char* entry_point, const char* target,
    ComPtr<ID3DBlob>& shader, std::string& error)
{
    const auto path = get_shader_directory() / file_name;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    ComPtr<ID3DBlob> messages;
    const HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry_point, target, flags, 0, &shader, &messages);
    if (messages)
    {
        const char* text = static_cast<const char*>(messages->GetBufferPointer());
        OutputDebugStringA(text);
        std::fputs(text, stderr);
        if (FAILED(result)) error = text;
    }
    if (FAILED(result) && error.empty())
        error = "Could not compile " + path.string() + " (" + entry_point + ")";
    return SUCCEEDED(result);
}
