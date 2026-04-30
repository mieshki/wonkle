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
