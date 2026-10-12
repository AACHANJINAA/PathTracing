#!/usr/bin/env bash
# Builds Tests/KernelTool with g++ or clang++ (Linux/macOS/WSL). On Windows use Tests/KernelTool.vcxproj.
set -euo pipefail
cd "$(dirname "$0")/.."
CXX="${CXX:-g++}"
OUT="${OUT:-build}"
mkdir -p "$OUT"
"$CXX" -std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -pthread \
    Tests/KernelTool.cpp \
    PathTracer/Scene/Json.cpp PathTracer/Scene/Scene.cpp PathTracer/Scene/SceneLoader.cpp \
    -o "$OUT/KernelTool"
echo "built $OUT/KernelTool"
