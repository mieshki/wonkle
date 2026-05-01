# firmware_cpp — C++17 Firmware

## OVERVIEW

C++17 bare-metal firmware for STM32F429IGTx (Cortex-M4F). Uses STM32Cube HAL via git submodules, SEGGER RTT for debug output, CMake+Ninja build, probe-rs flash. 84MHz system clock from HSI oscillator.

## STRUCTURE

```
firmware_cpp/
├── CMakeLists.txt              # Top-level build: app + core sources, flash target
├── build.sh / flash.sh         # Convenience scripts
├── cmake/
│   └── arm-none-eabi.cmake     # Cross-compiler: cortex-m4, newlib nano, -fno-exceptions
├── app/                        # Application code (< 300 lines each)
│   ├── main.cpp                # Entry: HAL_Init, SystemClock_Config, sensor scan loop
│   ├── sensor.cpp / sensor.hpp # 11×19 Hall sensor grid via ADC2+ADC3 polling
│   ├── board.cpp / board.hpp   # LED GPIO init on PE2
│   ├── rtt.cpp / rtt.hpp       # SEGGER RTT printf wrapper (vsnprintf + WriteString)
│   ├── systick.cpp             # extern "C" SysTick_Handler → HAL_IncTick
│   ├── stm32f4xx_hal_conf.h    # HAL module selection
│   └── SEGGER_RTT.{c,h,Conf.h} # SEGGER debug logging driver
├── core/                       # Git submodules — DO NOT MODIFY
│   ├── cmsis5/                 # ARM CMSIS_5 (ARM-software/CMSIS_5)
│   ├── stm32f4_cmsis/          # ST device header + startup (cmsis_device_f4)
│   ├── stm32f4_hal/            # ST HAL drivers (stm32f4xx_hal_driver)
│   └── linker/
│       └── STM32F429IGTx_FLASH.ld  # FLASH 1MB, RAM 192KB, CCMRAM 64KB
└── build/                      # Ninja output (gitignored)
```

## WHERE TO LOOK

| Task | File | Notes |
|------|------|-------|
| Change clock speed | `app/main.cpp:12` SystemClock_Config() | PLL: HSI/16×336/PLLP4=84MHz |
| Add sensor HAL module | `CMakeLists.txt:16` + `app/stm32f4xx_hal_conf.h` | Enable in both |
| Modify sensor scan | `app/sensor.cpp:153` scan() | Calls read_row() per row |
| Change row→mux mapping | `app/sensor.cpp:10` CHANNELS[][] | 11 rows → 4-bit mux select |
| Change column→ADC mapping | `app/sensor.cpp:29` COLS[] | 19 cols → ADC2/ADC3 channels |
| Adjust sampling speed | `app/sensor.cpp:65` set_sampling_time() | 84 cycles default |
| Change RTT buffer size | `app/SEGGER_RTT_Conf.h` | BUFFER_SIZE_UP default 1024 |
| Add new app source file | `CMakeLists.txt:20` APP_SOURCES | Add .cpp path here |

## BUILD & FLASH

```bash
# Build
./build.sh                                # cmake -B build -G Ninja && cmake --build build

# Flash + RTT output
./flash.sh                                # probe-rs run --chip STM32F429IGTx

# CMake direct
cmake --build build --target flash        # Build + flash in one step

# Git submodules (first time)
git submodule update --init --recursive
```

## KEY DESIGN DECISIONS

- **Direct register ADC** (`read_adc_fast`) instead of HAL ADC for speed: writes SQR3 → SWSTART → polls EOC → reads DR
- **VSprintf into fixed buffer** (128 bytes) then `SEGGER_RTT_WriteString` instead of `SEGGER_RTT_vprintf` — avoids newlib nano format limitation
- **Software-triggered single conversion** — no DMA, no continuous mode, no scan mode. One channel per conversion.
- **ADC12_COMMON cleared** (`ADC->CCR &= ~(ADC_CCR_MULTI_Msk)`) — ADC2/ADC3 independent, avoids dual-mode DMA conflict
- **Clock: 84MHz from HSI** — reliable. 168MHz (PLLP_DIV2) crashes 40% of the time from HSI jitter. HSE needed for 168MHz.

## ANTI-PATTERNS

- **NEVER`%lu`/`%llu` in printf** — newlib nano vsnprintf corrupts stack. Use `%u` with `static_cast<unsigned>()`.
- **NEVER 168MHz from HSI** — 40% hard fault rate. Use 84MHz or HSE.
- **NEVER call HAL_Delay() before SysTick_Handler** — infinite hang. SysTick_Handler must be `extern "C"`.
- **NEVER modify `core/`** — all vendor files. Only edit `app/` and `CMakeLists.txt`.
- **NEVER dynamic allocation** — no heap. Stack-only. Variable-length arrays OK.
- **NEVER use `probe-rs flash`** — use `probe-rs run` to capture RTT output.
