#pragma once
#include <cstdint>

extern "C" {
#include "stm32f4xx_hal.h"
}

namespace Pins {

constexpr uint8_t ROWS = 11;
constexpr uint8_t COLS = 19;

struct MuxConfig {
    GPIO_TypeDef* port;
    uint8_t s0;  // bit 0 (LSB)
    uint8_t s1;  // bit 1
    uint8_t s2;  // bit 2
    uint8_t s3;  // bit 3 (MSB)
};
constexpr MuxConfig MUX = {GPIOE, 3, 4, 5, 6};

// Mapping: logical row index (0-10) → physical mux channel (I0-I15)
constexpr uint8_t ROW_TO_MUX_CHANNEL[ROWS] = {10, 9, 8, 0, 1, 2, 3, 4, 5, 6, 7};

struct Column { GPIO_TypeDef* port; uint16_t pin; ADC_TypeDef* adc; uint8_t ch; };

inline GPIO_TypeDef* const BTN1_PORT = GPIOE;
constexpr uint16_t           BTN1_PIN  = GPIO_PIN_0;
inline GPIO_TypeDef* const BTN2_PORT = GPIOE;
constexpr uint16_t           BTN2_PIN  = GPIO_PIN_1;

constexpr Column COL[COLS] = {
    {GPIOF, GPIO_PIN_3,  ADC3,  9},
    {GPIOF, GPIO_PIN_4,  ADC3, 14},
    {GPIOF, GPIO_PIN_5,  ADC3, 15},
    {GPIOF, GPIO_PIN_6,  ADC3,  4},
    {GPIOF, GPIO_PIN_7,  ADC3,  5},
    {GPIOF, GPIO_PIN_8,  ADC3,  6},
    {GPIOF, GPIO_PIN_9,  ADC3,  7},
    {GPIOF, GPIO_PIN_10, ADC3,  8},
    {GPIOC, GPIO_PIN_1,  ADC2, 11},
    {GPIOC, GPIO_PIN_2,  ADC2, 12},
    {GPIOC, GPIO_PIN_3,  ADC2, 13},
    {GPIOA, GPIO_PIN_1,  ADC2,  1},
    {GPIOA, GPIO_PIN_2,  ADC2,  2},
    {GPIOA, GPIO_PIN_3,  ADC2,  3},
    {GPIOA, GPIO_PIN_4,  ADC2,  4},
    {GPIOA, GPIO_PIN_6,  ADC2,  6},
    {GPIOA, GPIO_PIN_7,  ADC2,  7},
    {GPIOC, GPIO_PIN_4,  ADC2, 14},
    {GPIOC, GPIO_PIN_5,  ADC2, 15},
};

}
