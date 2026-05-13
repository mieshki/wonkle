#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ ! -f "build/new_firmware.elf" ]]; then
    echo "Firmware not built yet. Building first..."
    ./compile.sh
fi

echo "Flashing..."
probe-rs run --chip STM32F429IGTx build/new_firmware.elf