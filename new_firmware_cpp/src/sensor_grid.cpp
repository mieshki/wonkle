#include "sensor_grid.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
}

void SensorGrid::init() {
    initMuxGpio();
    initAdcGpio();
    initAdcPeripheral();
    initDma();
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

void SensorGrid::initDma() {
    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma2_.Instance = DMA2_Stream2;
    hdma2_.Init.Channel = DMA_CHANNEL_1;
    hdma2_.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma2_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma2_.Init.MemInc = DMA_MINC_ENABLE;
    hdma2_.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma2_.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma2_.Init.Mode = DMA_NORMAL;
    hdma2_.Init.Priority = DMA_PRIORITY_HIGH;
    hdma2_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma2_);

    hdma3_.Instance = DMA2_Stream0;
    hdma3_.Init.Channel = DMA_CHANNEL_2;
    hdma3_.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma3_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma3_.Init.MemInc = DMA_MINC_ENABLE;
    hdma3_.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma3_.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma3_.Init.Mode = DMA_NORMAL;
    hdma3_.Init.Priority = DMA_PRIORITY_HIGH;
    hdma3_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma3_);

    __HAL_LINKDMA(&hadc2_, DMA_Handle, hdma2_);
    __HAL_LINKDMA(&hadc3_, DMA_Handle, hdma3_);
}

ADC_HandleTypeDef* SensorGrid::getAdc(ADC_TypeDef* instance) {
    return (instance == ADC2) ? &hadc2_ : &hadc3_;
}

void SensorGrid::configureChannel(ADC_HandleTypeDef* hadc, uint8_t channel) {
    ADC_ChannelConfTypeDef sConfig = {};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES; //ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(hadc, &sConfig);
}

void SensorGrid::configureSequence(ADC_HandleTypeDef* hadc, const uint8_t* channels, uint8_t count) {
    for (uint8_t i = 0; i < count; ++i) {
        ADC_ChannelConfTypeDef sConfig = {};
        sConfig.Channel = channels[i];
        sConfig.Rank = i + 1;
        sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
        HAL_ADC_ConfigChannel(hadc, &sConfig);
    }
    hadc->Init.NbrOfConversion = count;
    hadc->Init.ScanConvMode = ENABLE;
    HAL_ADC_Init(hadc);
}

void SensorGrid::selectRow(uint8_t rowIdx) {
    uint8_t ch = Pins::ROW_TO_MUX_CHANNEL[rowIdx];
    uint32_t bits = ((ch >> 0) & 1) << Pins::MUX.s0
                  | ((ch >> 1) & 1) << Pins::MUX.s1
                  | ((ch >> 2) & 1) << Pins::MUX.s2
                  | ((ch >> 3) & 1) << Pins::MUX.s3;
    uint32_t mask = (1U << Pins::MUX.s0) | (1U << Pins::MUX.s1)
                  | (1U << Pins::MUX.s2) | (1U << Pins::MUX.s3);
    Pins::MUX.port->ODR = (Pins::MUX.port->ODR & ~mask) | bits;
}
