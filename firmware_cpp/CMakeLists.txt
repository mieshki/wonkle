cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
project(firmware_cpp CXX C ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES
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
    core/usb_device/Src/usbd_core.c
    core/usb_device/Src/usbd_ctlreq.c
    core/usb_device/Src/usbd_ioreq.c
    core/usb_device/Src/usbd_hid.c
)

set(APP_SOURCES
    app/main.cpp
    app/board.cpp
    app/rtt.cpp
    app/systick.cpp
    app/sensor.cpp
    app/SEGGER_RTT.c
    app/usbd_conf.c
    app/usbd_desc.c
    app/usb.cpp
)

add_executable(${PROJECT_NAME}.elf ${CORE_SOURCES} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    core/cmsis5/CMSIS/Core/Include
    core/stm32f4_cmsis/Include
    core/stm32f4_hal/Inc
    core/usb_device/Inc
    app
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F429xx
    USE_HAL_DRIVER
)

add_custom_target(flash
    COMMAND probe-rs run --chip STM32F429IGTx $<TARGET_FILE:${PROJECT_NAME}.elf>
    DEPENDS ${PROJECT_NAME}.elf
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)
