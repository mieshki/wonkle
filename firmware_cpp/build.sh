#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ "${1:-}" == "--clean" || "${1:-}" == "-c" ]]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

echo "Configuring..."
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Building..."
cmake --build build

echo "Build complete: build/firmware_cpp.elf"
