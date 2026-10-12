#pragma once

#include "Scene.h"

#include <filesystem>
#include <string>

// Scene JSON format: docs/ARCHITECTURE.md section 5.
// On failure the output scene is left untouched and error explains what is wrong.
class SCENE_LOADER
{
public:
    static constexpr size_t max_primitives = 4096;

    static bool load_file(const std::filesystem::path& path, SCENE& scene, std::string& error);
    static bool load_text(const std::string& text, SCENE& scene, std::string& error);
};
