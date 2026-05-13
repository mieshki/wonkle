cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
project(new_firmware CXX C ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Core sources (submodules + vendored) ────────────────────────────
set(CORE_SOURCES
      core/logger/SEGGER_RTT.c
      core/usb_device/Src/usbd_core.c
      core/usb_device/Src/usbd_ctlreq.c
      core/usb_device/Src/usbd_hid.c
      core/usb_device/Src/usbd_ioreq.c
      core/stm32f4_cmsis/Source/Templates/gcc/startup_stm32f429xx.s
    core/stm32f4_cmsis/Source/Templates/system_stm32f4xx.c
    core/stm32f4_hal/Src/stm32f4xx_hal.c
    core/stm32f4_hal/Src/stm32f4xx_hal_gpio.c
    core/stm32f4_hal/Src/stm32f4xx_hal_rcc.c
    core/stm32f4_hal/Src/stm32f4xx_hal_cortex.c
    core/stm32f4_hal/Src/stm32f4xx_hal_dma.c
    core/stm32f4_hal/Src/stm32f4xx_hal_adc.c
    core/stm32f4_hal/Src/stm32f4xx_hal_adc_ex.c
    core/stm32f4_hal/Src/stm32f4xx_hal_pcd.c
    core/stm32f4_hal/Src/stm32f4xx_hal_pcd_ex.c
    core/stm32f4_hal/Src/stm32f4xx_ll_usb.c
)

# ── Application sources ─────────────────────────────────────────────
set(SRC_SOURCES
     src/main.cpp
     src/systick.cpp
     src/logger/rtt.cpp
 )

add_executable(${PROJECT_NAME}.elf ${CORE_SOURCES} ${SRC_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
      core
      core/logger
      core/usb_device/Inc
      core/cmsis5/CMSIS/Core/Include
      core/stm32f4_cmsis/Include
      core/stm32f4_hal/Inc
      src
      src/logger
 )

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F429xx
    USE_HAL_DRIVER
)

# ── Flash target ────────────────────────────────────────────────────
add_custom_target(flash
    COMMAND probe-rs run --chip STM32F429IGTx $<TARGET_FILE:${PROJECT_NAME}.elf>
    DEPENDS ${PROJECT_NAME}.elf
)

# ── Post-build: binary + size ───────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)