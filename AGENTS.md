# Wonkle Project Knowledge Base

**Generated:** 2026-05-01T20:54:43Z
**Commit:** d8092db
**Branch:** cpp_firmware_only

## OVERVIEW

Wonkle is an open-source osu! gaming tablet with STM32F429IGTx MCU (Cortex-M4F), an 11×19 grid of 209 DRV5055A4 Hall effect sensors read via 19 CD74HC4067 analog muxes, operating as a USB HID digitizer (VID:PID 1209:02D7). This branch contains the C++ firmware implementation only — no Rust firmware.

## STRUCTURE

```
wonkle_fork/
├── firmware_cpp/         # C++17 firmware: CMake build, STM32Cube HAL, SEGGER RTT
│   ├── app/              # Application code (main, sensor, board, rtt)
│   ├── core/             # Git submodules: CMSIS5, stm32f4_cmsis, stm32f4_hal
│   ├── cmake/            # arm-none-eabi toolchain file
│   └── build/            # Ninja build output (gitignored)
├── pcb/                  # KiCad hardware design (PompyBoard)
├── tools/                # Python visualization (pyocd + pygame)
├── docs/specs/           # Design docs and implementation plans
└── .github/workflows/    # CI (devenv/Nix, Rust-only — currently broken)
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Build firmware | `firmware_cpp/build.sh` | cmake -B build -G Ninja + cmake --build build |
| Flash firmware | `firmware_cpp/flash.sh` | probe-rs run --chip STM32F429IGTx |
| Sensor scanning code | `firmware_cpp/app/sensor.cpp` | Direct register ADC, 11 rows × 19 cols |
| Pin definitions | `firmware_cpp/app/board.hpp`, `firmware_cpp/app/sensor.cpp` | Mux on PE3-PE6, ADC2 on PA1-PA7/PC1-PC5, ADC3 on PF3-PF10 |
| Clock config | `firmware_cpp/app/main.cpp` | SystemClock_Config(), 84MHz from HSI, PLLP_DIV4, FLASH_LATENCY_2 |
| Linker script | `firmware_cpp/core/linker/STM32F429IGTx_FLASH.ld` | FLASH 1MB@0x08000000, RAM 192KB@0x20000000, CCMRAM 64KB@0x10000000 |
| RTT debug output | `firmware_cpp/app/rtt.cpp` | SEGGER RTT over SWD, print via probe-rs |
| Visualize sensor grid | `tools/visualize_grid_with_cursor.py` | pyocd reads g_grid at 0x20000060, 5 centroid algorithms |
| PCB manufacturing | `pcb/README.md` | JLCPCB instructions, 1.0mm FR4 |

## CODE MAP

| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `main()` | function | `firmware_cpp/app/main.cpp:42` | Entry point: HAL init, clock config, sensor scan loop |
| `Sensor::scan()` | function | `firmware_cpp/app/sensor.cpp:153` | Full 11×19 grid scan via RTT output |
| `Sensor::read()` | function | `firmware_cpp/app/sensor.cpp:133` | Single sensor read: select_row → delay → read_adc_fast |
| `read_adc_fast()` | function | `firmware_cpp/app/sensor.cpp:126` | Direct register ADC (SQR3 + SWSTART + poll EOC) |
| `select_row()` | function | `firmware_cpp/app/sensor.cpp:119` | Mux select via PE3-PE6 GPIO |
| `configure_adc()` | function | `firmware_cpp/app/sensor.cpp:51` | ADC2/ADC3 init: 12-bit, software trigger, PCLK/4 prescaler |
| `SystemClock_Config()` | function | `firmware_cpp/app/main.cpp:12` | PLL: HSI×336/PLLP4=84MHz, APB1/2, FLASH_LATENCY_2 |
| `SysTick_Handler()` | function | `firmware_cpp/app/systick.cpp:4` | extern "C", calls HAL_IncTick() |
| `g_grid` | global | `firmware_cpp/app/main.cpp:39` | uint16_t[209] — full sensor grid output buffer |
| `COLS[]` | const array | `firmware_cpp/app/sensor.cpp:29` | Column→ADC channel mapping for 19 columns |
| `CHANNELS[][]` | const array | `firmware_cpp/app/sensor.cpp:10` | Row→mux select mapping for 11 rows |

## CONVENTIONS

- **C++ standard**: C++17 (`-fno-exceptions -fno-rtti`)
- **Header guards**: `#pragma once`
- **HAL includes**: `extern "C" { #include "stm32f4xx_hal.h" }` in C++ files
- **Namespaces**: `Board`, `Sensor`, `RTT` — one per subsystem
- **Naming**: PascalCase namespaces/functions, SCREAMING_SNAKE_CASE constants, `g_` prefix for globals
- **Build**: CMake 3.20+ with Ninja generator, `arm-none-eabi-gcc`/`g++`, newlib nano
- **Debug**: SEGGER RTT over SWD (no UART, no semihosting)
- **Flashing**: `probe-rs run` (not `probe-rs flash` — runtime RTT capture needed)
- **No formatter config**: No `.clang-format` or `.editorconfig` — style is manual

## ANTI-PATTERNS (THIS PROJECT)

- **NEVER use `%lu` or `%llu` with newlib nano printf** — corrupts stack on 32-bit ARM. Use `%u` with `static_cast<unsigned>(val)`.
- **NEVER run HSI at 168MHz** — 40% failure rate from HSI jitter amplified through ×336 PLL. Cap at 84MHz (PLLP_DIV4) or use HSE.
- **NEVER call HAL_Delay() before SysTick_Handler is active** — infinite loop in HAL.
- **ADC2 and ADC3 cannot run DMA simultaneously in independent mode** — must clear `ADC_CCR_MULTI` before enabling.
- **No dynamic memory allocation** — newlib nano sbrk may not be implemented. Stack-only allocations.
- **RTT buffer: 1 byte always lost** — SEGGER RTT ring buffer loses 1 byte to distinguish full vs empty.

## UNIQUE STYLES

- **Direct register ADC**: `read_adc_fast()` bypasses HAL (SQR3 + SWSTART + poll EOC) for speed
- **RTT printf wrapper**: `vsnprintf` into fixed 128-byte buf, then `SEGGER_RTT_WriteString` (avoiding SEGGER_RTT_vprintf limitation)
- **Grid output format**: `===GRID===` / `R{row}: {vals}` / `===END===` — designed for host-side parsing

## COMMANDS

```bash
# Build C++ firmware
cd firmware_cpp && ./build.sh

# Flash + monitor RTT
cd firmware_cpp && ./flash.sh

# Build then flash
cd firmware_cpp && cmake --build build --target flash

# Clean rebuild
cd firmware_cpp && ./build.sh --clean

# Visualize sensor grid (requires firmware running on device)
python tools/visualize_grid_with_cursor.py firmware_cpp/build/firmware_cpp.elf
```

## NOTES

- **No Rust firmware** on this branch. CI references `firmware/` directory but it doesn't exist here. Was on `dev` branch.
- **Git submodules** in `firmware_cpp/core/`: init with `git submodule update --init --recursive`
- **probe-rs udev rules** required. Errno 13 means rules not installed.
- **Board thickness conflict**: README says 1.0mm, layout metadata says 1.6mm. Needs resolution.
- **Sensor grid address**: `g_grid` at `0x20000060` in RAM — tools read this via pyocd.
- **SEGGER_RTT_Conf.h** controls RTT buffer size (default: 1024) — adjust if RTT output truncated.
