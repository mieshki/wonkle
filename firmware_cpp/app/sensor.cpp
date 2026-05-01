#include "sensor.hpp"
#include "stm32f4xx_hal.h"

static ADC_HandleTypeDef hadc2;
static ADC_HandleTypeDef hadc3;

static constexpr uint8_t ROW_LEN = 11;
static constexpr uint8_t COL_LEN = 19;

static const bool CHANNELS[ROW_LEN][4] = {
    {false, true, false, true},
    {true, false, false, true},
    {false, false, false, true},
    {false, false, false, false},
    {true, false, false, false},
    {false, true, false, false},
    {true, true, false, false},
    {false, false, true, false},
    {true, false, true, false},
    {false, true, true, false},
    {true, true, true, false},
};

struct ColConfig {
    ADC_TypeDef* adc;
    uint32_t channel;
};

static const ColConfig COLS[COL_LEN] = {
    {ADC3, ADC_CHANNEL_9},
    {ADC3, ADC_CHANNEL_14},
    {ADC3, ADC_CHANNEL_15},
    {ADC3, ADC_CHANNEL_4},
    {ADC3, ADC_CHANNEL_5},
    {ADC3, ADC_CHANNEL_6},
    {ADC3, ADC_CHANNEL_7},
    {ADC3, ADC_CHANNEL_8},
    {ADC2, ADC_CHANNEL_11},
    {ADC2, ADC_CHANNEL_12},
    {ADC2, ADC_CHANNEL_13},
    {ADC2, ADC_CHANNEL_1},
    {ADC2, ADC_CHANNEL_2},
    {ADC2, ADC_CHANNEL_3},
    {ADC2, ADC_CHANNEL_4},
    {ADC2, ADC_CHANNEL_6},
    {ADC2, ADC_CHANNEL_7},
    {ADC2, ADC_CHANNEL_14},
    {ADC2, ADC_CHANNEL_15},
};

static void configure_adc(ADC_HandleTypeDef* hadc) {
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DMAContinuousRequests = DISABLE;
    HAL_ADC_Init(hadc);
}

static void set_sampling_time(ADC_TypeDef* adc, uint32_t channel) {
    uint32_t tmp = channel;
    if (tmp > ADC_CHANNEL_9) {
        tmp -= 10;
        MODIFY_REG(adc->SMPR1, ADC_SMPR1_SMP10 << (3 * tmp), ADC_SAMPLETIME_84CYCLES << (3 * tmp));
    } else {
        MODIFY_REG(adc->SMPR2, ADC_SMPR2_SMP0 << (3 * tmp), ADC_SAMPLETIME_84CYCLES << (3 * tmp));
    }
}

void Sensor::init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC2_CLK_ENABLE();
    __HAL_RCC_ADC3_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6
             | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio);

    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4
             | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);

    ADC->CCR &= ~(ADC_CCR_MULTI_Msk);

    hadc2.Instance = ADC2;
    configure_adc(&hadc2);
    hadc3.Instance = ADC3;
    configure_adc(&hadc3);

    for (uint8_t c = 0; c < COL_LEN; c++) {
        set_sampling_time(COLS[c].adc, COLS[c].channel);
    }

    SET_BIT(ADC2->CR2, ADC_CR2_ADON);
    SET_BIT(ADC3->CR2, ADC_CR2_ADON);
}

static void select_row(uint8_t row) {
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, CHANNELS[row][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, CHANNELS[row][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, CHANNELS[row][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, CHANNELS[row][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint16_t read_adc_fast(ADC_TypeDef* adc, uint32_t channel) {
    MODIFY_REG(adc->SQR3, ADC_SQR3_SQ1, channel);
    SET_BIT(adc->CR2, ADC_CR2_SWSTART);
    while (!(adc->SR & ADC_SR_EOC)) {}
    return (uint16_t)adc->DR;
}

uint16_t Sensor::read(uint8_t row, uint8_t col) {
    select_row(row);

    for (volatile uint32_t i = 0; i < 2000; i++) { __NOP(); }

    const ColConfig& cfg = COLS[col];
    return read_adc_fast(cfg.adc, cfg.channel);
}

void Sensor::read_row(uint8_t row, uint16_t* out) {
    select_row(row);

    for (volatile uint32_t i = 0; i < 2000; i++) { __NOP(); }

    for (uint8_t c = 0; c < COL_LEN; c++) {
        const ColConfig& cfg = COLS[c];
        out[c] = read_adc_fast(cfg.adc, cfg.channel);
    }
}

void Sensor::scan(uint16_t* out) {
    for (uint8_t r = 0; r < ROW_LEN; r++) {
        read_row(r, &out[r * COL_LEN]);
    }
}
