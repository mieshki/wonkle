#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Submodule check ───────────────────────────────────────────────────
MISSING=0
for submodule in core/cmsis5 core/stm32f4_cmsis core/stm32f4_hal; do
    if [ ! -d "$submodule" ] || [ -z "$(ls -A "$submodule" 2>/dev/null)" ]; then
        echo "ERROR: Submodule $submodule is missing or empty."
        MISSING=1
    fi
done

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "Please fetch the required submodules first:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

BUILD_TYPE="Debug"
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        --debug)   BUILD_TYPE="Debug" ;;
        --release) BUILD_TYPE="Release" ;;
        --clean|-c) CLEAN=1 ;;
        *) echo "Unknown option: $arg"; echo "Usage: $0 [--debug|--release] [--clean|-c]"; exit 1 ;;
    esac
done

if [[ "$CLEAN" -eq 1 ]]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

echo "Configuring (${BUILD_TYPE})..."
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Building..."
cmake --build build

echo "Build complete: build/new_firmware.elf"
