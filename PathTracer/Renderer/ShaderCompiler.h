#pragma once

#include "../Common/stdafx.h"

// Runtime HLSL compilation with FXC (D3DCompileFromFile, shader model 5.1).
// Shaders are copied next to the executable under Shaders/ by the build.
class SHADER_COMPILER
{
public:
    static std::filesystem::path get_shader_directory();
    static bool compile(const wchar_t* file_name, const char* entry_point, const char* target,
        ComPtr<ID3DBlob>& shader, std::string& error);
};
