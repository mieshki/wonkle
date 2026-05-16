#include "sensor_grid.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
}

void SensorGrid::init() {
    initMuxGpio();
    initAdcGpio();
    initAdcPeripheral();
}

void SensorGrid::initMuxGpio() {
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef cfg = {};
    cfg.Pin = (1U << Pins::MUX.s0) | (1U << Pins::MUX.s1)
            | (1U << Pins::MUX.s2) | (1U << Pins::MUX.s3);
    cfg.Mode = GPIO_MODE_OUTPUT_PP;
    cfg.Pull = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Pins::MUX.port, &cfg);
}

void SensorGrid::initAdcGpio() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef cfg = {};
    cfg.Mode = GPIO_MODE_ANALOG;
    cfg.Pull = GPIO_NOPULL;

    uint16_t pinsA = 0, pinsC = 0, pinsF = 0;
    for (uint8_t i = 0; i < Pins::COLS; ++i) {
        if (Pins::COL[i].port == GPIOA)
            pinsA |= Pins::COL[i].pin;
        else if (Pins::COL[i].port == GPIOC)
            pinsC |= Pins::COL[i].pin;
        else if (Pins::COL[i].port == GPIOF)
            pinsF |= Pins::COL[i].pin;
    }

    if (pinsA) { cfg.Pin = pinsA; HAL_GPIO_Init(GPIOA, &cfg); }
    if (pinsC) { cfg.Pin = pinsC; HAL_GPIO_Init(GPIOC, &cfg); }
    if (pinsF) { cfg.Pin = pinsF; HAL_GPIO_Init(GPIOF, &cfg); }
}

void SensorGrid::initAdcPeripheral() {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC2_CLK_ENABLE();
    __HAL_RCC_ADC3_CLK_ENABLE();

    hadc2_.Instance = ADC2;
    hadc2_.Init.Resolution = ADC_RESOLUTION_12B;
    hadc2_.Init.ScanConvMode = DISABLE;
    hadc2_.Init.ContinuousConvMode = DISABLE;
    hadc2_.Init.DiscontinuousConvMode = DISABLE;
    hadc2_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc2_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2_.Init.NbrOfConversion = 1;
    hadc2_.Init.DMAContinuousRequests = DISABLE;
    hadc2_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc2_);
    HAL_ADC_Start(&hadc2_);

    hadc3_.Instance = ADC3;
    hadc3_.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3_.Init.ScanConvMode = DISABLE;
    hadc3_.Init.ContinuousConvMode = DISABLE;
    hadc3_.Init.DiscontinuousConvMode = DISABLE;
    hadc3_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3_.Init.NbrOfConversion = 1;
    hadc3_.Init.DMAContinuousRequests = DISABLE;
    hadc3_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc3_);
    HAL_ADC_Start(&hadc3_);
}
