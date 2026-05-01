# Wonkle Firmware C++ Implementation Plan

**Date**: 2026-04-29  
**Phase**: 1 (Hello World / RTT Logging)  
**Target**: STM32F429IGTx  

---

## Overview

This plan details the concrete steps to implement the `firmware_cpp` Phase 1 Hello World. Each step is designed to be completed in order, with verification at key milestones.

---

## Prerequisites

- [ ] `arm-none-eabi-gcc` installed (`arm-none-eabi-gcc --version`)
- [ ] `cmake` >= 3.20 installed (`cmake --version`)
- [ ] `ninja-build` installed (`ninja --version`)
- [ ] `probe-rs` working with ST-Link V2 (`probe-rs list` shows device)
- [ ] STM32F429 reference manual and datasheet available

---

## Step 1: Create Directory Structure

**Goal**: Establish the project layout.

```bash
cd /path/to/wonkle
mkdir -p firmware_cpp/{cmake,core/{startup,linker,hal},app,.cargo}
touch firmware_cpp/CMakeLists.txt
```

**Files created**:
- `firmware_cpp/CMakeLists.txt`
- `firmware_cpp/cmake/arm-none-eabi.cmake`
- `firmware_cpp/.cargo/config.toml`

---

## Step 2: Obtain STM32CubeF4 Startup Code

**Goal**: Get vendor-generated startup, system, and linker files.

**Option A: STM32CubeMX (Recommended)**
1. Download STM32CubeMX from ST website
2. Create new project for STM32F429IGTx
3. In "Project Manager":
   - Toolchain: `CMake`
   - Code Generator: Generate peripheral initialization as pair of `.c/.h` files
4. Generate code
5. Copy generated files into `firmware_cpp/core/`

**Option B: Manual Download**
1. Download STM32CubeF4 firmware package from ST
2. Extract and copy:
   - `Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f429xx.s` → `core/startup/`
   - `Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c` → `core/startup/`
   - `Drivers/CMSIS/Device/ST/STM32F4xx/Include/` → `core/hal/include/`
   - `Drivers/STM32F4xx_HAL_Driver/` → `core/hal/`
   - Linker script from `Projects/STM32F429I-Discovery/Templates/SW4STM32/` → `core/linker/`

**Verification**: `ls firmware_cpp/core/startup/` shows `startup_stm32f429xx.s` and `system_stm32f4xx.c`

---

## Step 3: Create CMake Toolchain File

**Goal**: Configure CMake to use ARM cross-compiler.

**File**: `firmware_cpp/cmake/arm-none-eabi.cmake`

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_C_FLAGS} -T${CMAKE_SOURCE_DIR}/core/linker/STM32F429IGTx_FLASH.ld --specs=nano.specs -lc -lm -lnosys -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**Verification**: File exists and contains the above content.

---

## Step 4: Create Top-Level CMakeLists.txt

**Goal**: Define the build targets.

**File**: `firmware_cpp/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
project(firmware_cpp C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HAL sources (add as needed)
set(HAL_SOURCES
    core/startup/startup_stm32f429xx.s
    core/startup/system_stm32f4xx.c
    # Add HAL .c files here as needed
)

# Application sources
set(APP_SOURCES
    app/main.cpp
    app/board.cpp
    app/rtt.cpp
)

add_executable(${PROJECT_NAME}.elf ${HAL_SOURCES} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    core/hal/include
    core/startup
    app
)

# Custom flash target
add_custom_target(flash
    COMMAND probe-rs run --chip STM32F429IGTx ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
    DEPENDS ${PROJECT_NAME}.elf
)
```

**Verification**: `cmake -B build` runs without errors.

---

## Step 5: Configure probe-rs Runner

**Goal**: Allow `cargo run` or direct `probe-rs run` to flash the ELF.

**File**: `firmware_cpp/.cargo/config.toml`

```toml
[target.'cfg(all(target_arch = "arm", target_os = "none"))']
runner = "probe-rs run --chip STM32F429IGTx"

[build]
target = "thumbv7em-none-eabihf"
```

**Verification**: File exists.

---

## Step 6: Write Board Configuration

**Goal**: Define board-specific constants (pins, clocks).

**File**: `firmware_cpp/app/board.hpp`

```cpp
#pragma once
#include <cstdint>

namespace Board {
    // LED on PE2 (verify with schematic)
    constexpr uint32_t LED_PORT = 0; // GPIOE base
    constexpr uint16_t LED_PIN = 2;

    void init();
    void toggle_led();
}
```

**File**: `firmware_cpp/app/board.cpp`

```cpp
#include "board.hpp"
#include "stm32f4xx_hal.h"

namespace Board {
    void init() {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        GPIO_InitTypeDef gpio = {};
        gpio.Pin = GPIO_PIN_2;
        gpio.Mode = GPIO_MODE_OUTPUT_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOE, &gpio);
    }

    void toggle_led() {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_2);
    }
}
```

**Note**: Verify LED pin with Wonkle schematic. Adjust if different.

---

## Step 7: Write RTT Logging Wrapper

**Goal**: Provide printf-style logging over SEGGER RTT.

**File**: `firmware_cpp/app/rtt.hpp`

```cpp
#pragma once
#include <cstdarg>
#include <cstdio>

namespace RTT {
    void init();
    void printf(const char* fmt, ...);
}
```

**File**: `firmware_cpp/app/rtt.cpp`

```cpp
#include "rtt.hpp"
#include "SEGGER_RTT.h"

namespace RTT {
    void init() {
        SEGGER_RTT_Init();
    }

    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        SEGGER_RTT_vprintf(0, fmt, &args);
        va_end(args);
    }
}
```

**Note**: `SEGGER_RTT.h` and `SEGGER_RTT.c` must be added to the build. These come from the SEGGER J-Link package or ST firmware pack.

---

## Step 8: Write Main Application

**Goal**: Hello World — blink LED and print over RTT.

**File**: `firmware_cpp/app/main.cpp`

```cpp
#include "board.hpp"
#include "rtt.hpp"
#include "stm32f4xx_hal.h"

extern "C" void SystemClock_Config(void);

int main() {
    HAL_Init();
    SystemClock_Config();
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

---

## Step 9: Configure System Clock

**Goal**: Ensure `SystemClock_Config()` sets up 84 MHz (or desired) clock.

**File**: `core/startup/system_stm32f4xx.c` (generated by STM32CubeMX)

**Verification**: Check that `SystemCoreClock` is set to `84000000` (84 MHz). The generated file from STM32CubeMX should already be correct for the selected chip.

---

## Step 10: Build

**Goal**: Compile and link the firmware.

```bash
cd /path/to/wonkle/firmware_cpp
cmake -B build -G Ninja
cmake --build build
```

**Verification**:
- `build/firmware_cpp.elf` exists
- `arm-none-eabi-size build/firmware_cpp.elf` shows reasonable sizes (< 64KB for Phase 1)

---

## Step 11: Flash and Verify

**Goal**: Deploy firmware and observe RTT output.

```bash
probe-rs run --chip STM32F429IGTx build/firmware_cpp.elf
```

**Expected output**:
```
Hello from Wonkle C++!
Tick
Tick
Tick
...
```

**Physical verification**: On-board LED blinks at 1 Hz.

---

## Step 12: Add mise.toml (Optional but Recommended)

**Goal**: Make the C++ toolchain manageable via mise.

**File**: `firmware_cpp/mise.toml`

```toml
[tools]
# Add arm-none-eabi-gcc via nix or asdf if available
"nix:gcc-arm-embedded" = "latest"
"nix:cmake" = "latest"
"nix:ninja" = "latest"

[plugins]
nix = "https://github.com/jbadeau/mise-nix"
```

**Note**: This is optional. If `arm-none-eabi-gcc` is already system-installed, skip this step.

---

## Success Criteria

- [ ] `cmake -B build` succeeds
- [ ] `cmake --build build` produces `firmware_cpp.elf`
- [ ] `probe-rs run` flashes without errors
- [ ] RTT shows "Hello from Wonkle C++!"
- [ ] LED blinks at 1 Hz

---

## Next Steps (Phase 2)

1. Add HAL ADC initialization
2. Port sensor scanning logic (11×19 matrix + centroid)
3. Add USB Device middleware
4. Implement HID digitizer descriptor

---

## Files Summary

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Build definition |
| `cmake/arm-none-eabi.cmake` | Cross-compilation settings |
| `.cargo/config.toml` | probe-rs runner config |
| `core/startup/startup_stm32f429xx.s` | Assembly startup |
| `core/startup/system_stm32f4xx.c` | Clock init |
| `core/linker/STM32F429IGTx_FLASH.ld` | Memory layout |
| `app/main.cpp` | Application entry |
| `app/board.hpp/cpp` | Board config |
| `app/rtt.hpp/cpp` | RTT logging |
