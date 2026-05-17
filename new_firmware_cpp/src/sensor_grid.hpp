#pragma once
#include <cstdint>
#include "pins.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
}

enum class AdcSampling : uint8_t {
    Cycles3, Cycles15, Cycles28, Cycles56, Cycles84, Cycles112, Cycles144, Cycles480
};

constexpr uint8_t ADC_SAMPLING_COUNT = 8;

constexpr uint32_t adcSamplingToHal(AdcSampling s) {
    constexpr uint32_t map[ADC_SAMPLING_COUNT] = {
        ADC_SAMPLETIME_3CYCLES,
        ADC_SAMPLETIME_15CYCLES,
        ADC_SAMPLETIME_28CYCLES,
        ADC_SAMPLETIME_56CYCLES,
        ADC_SAMPLETIME_84CYCLES,
        ADC_SAMPLETIME_112CYCLES,
        ADC_SAMPLETIME_144CYCLES,
        ADC_SAMPLETIME_480CYCLES,
    };
    return map[static_cast<uint8_t>(s)];
}

constexpr const char* adcSamplingLabel(AdcSampling s) {
    constexpr const char* labels[ADC_SAMPLING_COUNT] = {
        "3 cyc", "15 cyc", "28 cyc", "56 cyc",
        "84 cyc", "112 cyc", "144 cyc", "480 cyc",
    };
    return labels[static_cast<uint8_t>(s)];
}

class SensorGridBenchmark;

class SensorGrid {
    friend class SensorGridBenchmark;

public:
    void init();
    void scan_grid(uint16_t* out);

    uint32_t getMuxSettling() const { return muxSettling_; }
    void setMuxSettling(uint32_t cycles);

    AdcSampling getAdcSampling() const { return adcSampling_; }
    void setAdcSampling(AdcSampling s);

    uint8_t getAdcDummyReads() const { return adcDummyReads_; }
    void setAdcDummyReads(uint8_t count);

    bool getOversampleEnabled() const { return oversampleEnabled_; }
    void setOversampleEnabled(bool enabled);

private:
    void initMuxGpio();
    void initAdcGpio();
    void initAdcPeripheral();
    void initDma();
    void applyAdcSampling(AdcSampling s);

    ADC_HandleTypeDef hadc2_;
    ADC_HandleTypeDef hadc3_;
    DMA_HandleTypeDef hdma2_;
    DMA_HandleTypeDef hdma3_;

    uint32_t    muxSettling_     = 4000;
    AdcSampling adcSampling_     = AdcSampling::Cycles3;
    uint8_t     adcDummyReads_   = 3;
    bool        oversampleEnabled_ = true;

    ADC_HandleTypeDef* getAdc(ADC_TypeDef* instance);
    void configureChannel(ADC_HandleTypeDef* hadc, uint8_t channel);
    void configureSequence(ADC_HandleTypeDef* hadc, const uint8_t* channels, uint8_t count);
    void selectRow(uint8_t rowIdx);
};
