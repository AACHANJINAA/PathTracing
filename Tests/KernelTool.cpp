// KernelTool: runs the GPU path tracing kernel on the CPU.
//
//   KernelTool test [--quick]           unit and statistical tests (docs/VERIFY.md V-IDs)
//
// Build: Tests/build.sh (g++/clang) or Tests/KernelTool.vcxproj (MSVC).

#include "CpuKernel.h"
#include "../PathTracer/Scene/SceneLoader.h"

#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace
{
    std::filesystem::path g_repository_root;

    std::filesystem::path find_repository_root(const char* argv0)
    {
        for (auto path : { std::filesystem::current_path(), std::filesystem::absolute(argv0).parent_path() })
        {
            for (int depth = 0; depth < 6 && !path.empty(); ++depth, path = path.parent_path())
                if (std::filesystem::exists(path / "Scenes" / "cornell_box.json")) return path;
        }
        return std::filesystem::current_path();
    }

    std::filesystem::path scene_path(const char* name) { return g_repository_root / "Scenes" / name; }
}

#include "KernelTests.inl"

int main(int argc, char** argv)
{
    g_repository_root = find_repository_root(argv[0]);
    const std::string command = argc > 1 ? argv[1] : "test";
    std::vector<std::string> arguments(argv + (argc > 1 ? 2 : 1), argv + argc);
    if (command == "test") return run_tests(arguments);
    std::fprintf(stderr, "usage: KernelTool test [--quick]\n");
    return 2;
}
