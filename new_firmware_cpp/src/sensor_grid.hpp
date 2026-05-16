#pragma once
#include <cstdint>
#include "pins.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
}

class SensorGridBenchmark;

class SensorGrid {
    friend class SensorGridBenchmark;

public:
    void init();
    void scan_grid(uint16_t* out);

private:
    void initMuxGpio();
    void initAdcGpio();
    void initAdcPeripheral();
    void initDma();

    ADC_HandleTypeDef hadc2_;
    ADC_HandleTypeDef hadc3_;
    DMA_HandleTypeDef hdma2_;
    DMA_HandleTypeDef hdma3_;

    ADC_HandleTypeDef* getAdc(ADC_TypeDef* instance);
    void configureChannel(ADC_HandleTypeDef* hadc, uint8_t channel);
    void configureSequence(ADC_HandleTypeDef* hadc, const uint8_t* channels, uint8_t count);
    void selectRow(uint8_t rowIdx);
};
