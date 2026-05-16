#pragma once
#include <cstdint>
#include "pins.hpp"

class SensorGrid {
public:
    void init();

private:
    void initMuxGpio();
    void initAdcGpio();
    void initAdcPeripheral();

    ADC_HandleTypeDef hadc2_;
    ADC_HandleTypeDef hadc3_;
};
