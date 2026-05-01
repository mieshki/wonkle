# Wonkle Firmware C++ Design Specification

**Date**: 2026-04-29  
**Target**: STM32F429IGTx (Wonkleboard Lite mk.1)  
**Language**: C++17  
**Scope**: Phase 1 (Hello World / RTT Logging)

---

## 1. Overview

This document specifies the architecture for `firmware_cpp`, a C++ reimplementation of the Wonkle tablet firmware. The goal is to establish a working C++ embedded development environment on the STM32F429, starting with a minimal "Hello World" that proves the toolchain, HAL, build system, and flashing pipeline.

The design is forward-compatible with later phases (sensor scanning, USB HID digitizer).

---

## 2. Goals

1. **Toolchain validation**: `arm-none-eabi-gcc` compiles C++ for `thumbv7em-none-eabihf`
2. **Runtime validation**: Startup code, linker script, and `SystemInit()` bring the chip up correctly
3. **Debug validation**: SEGGER RTT prints "Hello World" visible in the terminal via `probe-rs`
4. **Flash validation**: `probe-rs flash` deploys the ELF successfully
5. **Architecture foundation**: Directory structure and build system support incremental feature addition

---

## 3. Non-Goals

- No sensor scanning in Phase 1
- No USB HID in Phase 1
- No board-specific business logic beyond blinking an LED or RTT print

---

## 4. Architecture

### 4.1 Directory Structure

```
firmware_cpp/
├── CMakeLists.txt              # Top-level build
├── cmake/
│   └── arm-none-eabi.cmake     # Toolchain file
├── core/                       # CMSIS + STM32F4xx HAL (STM32Cube generated)
│   ├── startup/
│   │   ├── startup_stm32f429xx.s
│   │   └── system_stm32f4xx.c
│   ├── linker/
│   │   └── STM32F429IGTx_FLASH.ld
│   └── hal/                    # STM32CubeF4 HAL/LL sources
├── app/                        # Application C++ code
│   ├── main.cpp
│   ├── board.hpp               # Pin definitions, board constants
│   ├── board.cpp
│   ├── rtt.hpp                 # RTT logging wrapper
│   └── rtt.cpp
├── .cargo/
│   └── config.toml             # probe-rs runner configuration
└── README.md
```

### 4.2 Component Descriptions

#### `core/`
Contains all vendor-generated code. This directory is treated as a third-party dependency — application code does not modify it.

- **`startup_stm32f429xx.s`**: ARM Cortex-M4 startup assembly. Sets up the stack pointer, initializes `.data` and `.bss` sections, calls `SystemInit()`, then jumps to `main()`.
- **`system_stm32f4xx.c`**: CMSIS system file. Configures the Flash latency, voltage regulator, and PLL to achieve the desired system clock (84 MHz for Phase 1).
- **`STM32F429IGTx_FLASH.ld`**: Linker script defining memory regions:
  - `FLASH` (rx): `0x08000000`, 1 MB
  - `RAM` (rwx): `0x20000000`, 192 KB
  - `CCMRAM` (rwx): `0x10000000`, 64 KB
- **`hal/`**: STM32Cube HAL and LL drivers. Included as-is from STM32CubeF4.

#### `app/`
Contains all custom C++ application code.

- **`main.cpp`**: Entry point. Initializes HAL, configures system clock, sets up RTT, prints a message in a loop.
- **`board.hpp/cpp`**: Board-specific constants — pin mappings, LED GPIO port/pin, clock configurations.
- **`rtt.hpp/cpp`**: Thin C++ wrapper around SEGGER RTT. Provides a `printf`-style interface for logging.

#### `.cargo/config.toml`
Configures `cargo` (or `probe-rs run`) to use the correct runner. Even without Rust code, this file ensures `cargo run` can flash the C++ ELF via probe-rs.

```toml
[target.'cfg(all(target_arch = "arm", target_os = "none"))']
runner = "probe-rs run --chip STM32F429IGTx"

[build]
target = "thumbv7em-none-eabihf"
```

### 4.3 Build System

**CMake + Ninja** is the primary build system.

**Toolchain file (`cmake/arm-none-eabi.cmake`)**:
- Sets `CMAKE_SYSTEM_NAME` to `Generic`
- Sets `CMAKE_SYSTEM_PROCESSOR` to `arm`
- Finds `arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-objcopy`
- Sets flags:
  - `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
  - `-fno-exceptions -fno-rtti` (embedded C++)
  - `-ffunction-sections -fdata-sections`
  - `-Os` (optimize for size)
  - Linker flags: `--specs=nano.specs -lc -lm -lnosys -T${LINKER_SCRIPT} -Wl,--gc-sections`

**Top-level `CMakeLists.txt`**:
- Sets minimum CMake version (3.20)
- Includes toolchain file
- Defines the project
- Adds subdirectories for `core/` and `app/`
- Links the final ELF target
- Adds a custom target `flash` that runs `probe-rs flash`

### 4.4 Flashing and Debugging

**Flash command**:
```bash
probe-rs run --chip STM32F429IGTx build/firmware_cpp.elf
```

**RTT output**: SEGGER RTT runs over the ST-Link SWD connection. `probe-rs` captures RTT output and prints it to the host terminal. No UART or semihosting required.

---

## 5. Phase 1: Hello World Specification

### 5.1 Expected Behavior

After flashing:
1. The green LED on the board (or a debug GPIO) toggles at 1 Hz
2. RTT channel 0 prints: `Hello from Wonkle C++!` every second

### 5.2 Main Flow

```cpp
// main.cpp
int main() {
    HAL_Init();
    SystemClock_Config();  // 84 MHz from HSI
    Board::init();
    RTT::init();

    RTT::printf("Hello from Wonkle C++!\n");

    while (true) {
        Board::toggle_led();
        RTT::printf("Tick\n");
        HAL_Delay(1000);
    }
}
```

### 5.3 RTT Implementation

SEGGER RTT is a small, in-memory logging buffer accessed by the debugger. The implementation:
- Includes `SEGGER_RTT.h` and `SEGGER_RTT_Conf.h`
- Provides `RTT::init()` to set up the buffer
- Provides `RTT::printf(const char* fmt, ...)` as a thin wrapper

No dynamic memory allocation is used.

---

## 6. Phase 2+ Roadmap (Not in Scope)

### Phase 2: Sensor Scanning
- Port the 11×19 ADC matrix scanning logic
- Use HAL ADC in polling or DMA mode
- Implement centroid calculation in C++

### Phase 3: USB HID Digitizer
- Use STM32Cube USB Device middleware
- Define HID report descriptor for digitizer
- Implement `usbd_hid` equivalent in C

---

## 7. Dependencies

| Dependency | Version | Source | Purpose |
|-----------|---------|--------|---------|
| `arm-none-eabi-gcc` | 13.x+ | Ubuntu repos or Arm Developer | Compiler |
| `cmake` | 3.20+ | Ubuntu repos | Build system |
| `ninja-build` | latest | Ubuntu repos | Build generator |
| `STM32CubeF4` | latest | ST website | HAL/LL/startup |
| `SEGGER RTT` | latest | SEGGER or ST pack | Debug logging |
| `probe-rs` | 0.31.0 | Already installed | Flash/debug |

---

## 8. Risks and Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| STM32CubeMX-generated code is too heavy | Medium | Use LL drivers where possible; keep only required HAL modules |
| C++ exceptions/RTTI bloat binary | Low | Compiler flags `-fno-exceptions -fno-rtti` |
| RTT not visible in probe-rs | Low | Verify RTT block address in linker script; use `probe-rs run` not just `flash` |
| Linker script mismatch with Rust | Low | Match memory regions exactly to `memory.x` from Rust firmware |

---

## 9. Success Criteria

- [ ] `cmake -B build` completes without errors
- [ ] `cmake --build build` produces `firmware_cpp.elf`
- [ ] `probe-rs run --chip STM32F429IGTx build/firmware_cpp.elf` flashes successfully
- [ ] RTT output shows "Hello from Wonkle C++!" in the terminal
- [ ] LED blinks at 1 Hz

---

## 10. References

- [Rust firmware memory layout](https://github.com/wonkleio/wonkle/blob/main/firmware/memory.x)
- [STM32CubeF4 HAL User Manual](https://www.st.com/resource/en/user_manual/dm00105879-description-of-stm32f4-hal-and-ll-drivers-stmicroelectronics.pdf)
- [SEGGER RTT Documentation](https://wiki.segger.com/RTT)
- [probe-rs Getting Started](https://probe.rs/docs/getting-started/)
- [CMSIS-Core Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
