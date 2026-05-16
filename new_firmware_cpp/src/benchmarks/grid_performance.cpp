#include "grid_performance.hpp"
#include "logger/rtt.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
}

SensorGridBenchmark::SensorGridBenchmark(SensorGrid& grid) : grid_(grid) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    buildSeqChannels();
}

void SensorGridBenchmark::buildSeqChannels() {
    uint8_t i2 = 0, i3 = 0;
    for (uint8_t i = 0; i < Pins::COLS; ++i) {
        if (Pins::COL[i].adc == ADC2) adc2Channels_[i2++] = Pins::COL[i].ch;
        else adc3Channels_[i3++] = Pins::COL[i].ch;
    }
}

static BenchStats computeStats(const uint16_t* values, uint32_t totalReads, uint32_t duration_us) {
    uint16_t min = 0xFFFF, max = 0;
    uint32_t sum = 0;

    for (uint32_t i = 0; i < totalReads; ++i) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
        sum += values[i];
    }

    uint16_t avg = static_cast<uint16_t>(sum / totalReads);
    uint32_t sq_diff_sum = 0;

    for (uint32_t i = 0; i < totalReads; ++i) {
        int16_t diff = static_cast<int16_t>(values[i]) - static_cast<int16_t>(avg);
        if (diff < 0) diff = -diff;
        sq_diff_sum += static_cast<uint32_t>(diff) * diff;
    }

    uint16_t p50 = 0, p90 = 0, p99 = 0;
    if (totalReads <= 1000) {
        uint16_t sorted[1000];
        for (uint32_t i = 0; i < totalReads; ++i) sorted[i] = values[i];
        for (uint32_t i = 1; i < totalReads; ++i) {
            uint16_t key = sorted[i];
            int32_t j = i - 1;
            while (j >= 0 && sorted[j] > key) { sorted[j + 1] = sorted[j]; j--; }
            sorted[j + 1] = key;
        }
        p50 = sorted[totalReads / 2];
        p90 = sorted[totalReads * 90 / 100];
        p99 = sorted[totalReads * 99 / 100];
    }

    return {min, max, static_cast<uint16_t>(max - min), duration_us,
            p50, p90, p99, sq_diff_sum / totalReads};
}

static uint32_t sqrxPack(uint8_t sq1, uint8_t sq2, uint8_t sq3, uint8_t sq4, uint8_t sq5, uint8_t sq6) {
    return (sq1 & 0x1F) | ((sq2 & 0x1F) << 5) | ((sq3 & 0x1F) << 10)
         | ((sq4 & 0x1F) << 15) | ((sq5 & 0x1F) << 20) | ((sq6 & 0x1F) << 25);
}

static void initHalSingleMode(ADC_HandleTypeDef* hadc) {
    hadc->Lock = HAL_UNLOCKED;
    hadc->Init.ScanConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    HAL_ADC_Init(hadc);
}

static void initHalDualSingleMode(ADC_HandleTypeDef* hadc2, ADC_HandleTypeDef* hadc3) {
    initHalSingleMode(hadc2);
    initHalSingleMode(hadc3);
}

static void setupLLSequence(ADC_TypeDef* adc2, ADC_TypeDef* adc3,
                            const uint8_t* ch2, uint8_t len2,
                            const uint8_t* ch3, uint8_t len3) {
    adc2->SQR3 = sqrxPack(ch2[0], ch2[1], ch2[2], ch2[3], ch2[4], ch2[5]);
    adc2->SQR2 = sqrxPack(ch2[6], ch2[7], ch2[8], ch2[9], ch2[10], 0);
    adc2->SQR1 = (len2 - 1) << 20;

    adc3->SQR3 = sqrxPack(ch3[0], ch3[1], ch3[2], ch3[3], ch3[4], ch3[5]);
    adc3->SQR2 = sqrxPack(ch3[6], ch3[7], 0, 0, 0, 0);
    adc3->SQR1 = (len3 - 1) << 20;
}

static uint32_t elapsedUs(uint32_t t0) {
    uint32_t cycles = DWT->CYCCNT - t0;
    return static_cast<uint32_t>((static_cast<uint64_t>(cycles) * 1000000) / SystemCoreClock);
}

BenchStats SensorGridBenchmark::benchmarkHalPerPin(uint8_t colIdx, uint16_t count) {
    const auto& col = Pins::COL[colIdx];
    ADC_HandleTypeDef* hadc = grid_.getAdc(col.adc);
    initHalSingleMode(hadc);

    uint16_t values[1000];
    grid_.selectRow(0);
    grid_.configureChannel(hadc, col.ch);
    HAL_ADC_Start(hadc);

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t i = 0; i < count; ++i) {
        SET_BIT(hadc->Instance->CR2, ADC_CR2_SWSTART);
        HAL_ADC_PollForConversion(hadc, 10);
        values[i] = HAL_ADC_GetValue(hadc);
    }

    HAL_ADC_Stop(hadc);
    return computeStats(values, count, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkLLPerPin(uint8_t colIdx, uint16_t count) {
    const auto& col = Pins::COL[colIdx];
    ADC_HandleTypeDef* hadc = grid_.getAdc(col.adc);
    initHalSingleMode(hadc);

    uint16_t values[1000];
    grid_.selectRow(0);
    grid_.configureChannel(hadc, col.ch);
    SET_BIT(col.adc->CR2, ADC_CR2_ADON);

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t i = 0; i < count; ++i) {
        SET_BIT(col.adc->CR2, ADC_CR2_SWSTART);
        while (!(col.adc->SR & ADC_SR_EOC)) {}
        values[i] = static_cast<uint16_t>(col.adc->DR);
    }

    CLEAR_BIT(col.adc->CR2, ADC_CR2_ADON);
    return computeStats(values, count, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkHalPerPinRow(uint8_t rowIdx, uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;

    grid_.selectRow(rowIdx);
    ADC_HandleTypeDef* hadc2 = grid_.getAdc(ADC2);
    ADC_HandleTypeDef* hadc3 = grid_.getAdc(ADC3);
    initHalDualSingleMode(hadc2, hadc3);

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t i = 0; i < Pins::COLS; ++i) {
            const auto& col = Pins::COL[i];
            ADC_HandleTypeDef* hadc = grid_.getAdc(col.adc);
            grid_.configureChannel(hadc, col.ch);
            HAL_ADC_Start(hadc);
            HAL_ADC_PollForConversion(hadc, 10);
            if (totalReads < 1000) values[totalReads++] = HAL_ADC_GetValue(hadc);
            HAL_ADC_Stop(hadc);
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkLLPerPinRow(uint8_t rowIdx, uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;

    grid_.selectRow(rowIdx);
    initHalDualSingleMode(grid_.getAdc(ADC2), grid_.getAdc(ADC3));

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t i = 0; i < Pins::COLS; ++i) {
            const auto& col = Pins::COL[i];
            ADC_TypeDef* adc = col.adc;
            grid_.configureChannel(grid_.getAdc(adc), col.ch);
            SET_BIT(adc->CR2, ADC_CR2_ADON);
            SET_BIT(adc->CR2, ADC_CR2_SWSTART);
            while (!(adc->SR & ADC_SR_EOC)) {}
            if (totalReads < 1000) values[totalReads++] = static_cast<uint16_t>(adc->DR);
            CLEAR_BIT(adc->CR2, ADC_CR2_ADON);
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkHalPerPinGrid(uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;

    initHalDualSingleMode(grid_.getAdc(ADC2), grid_.getAdc(ADC3));

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t row = 0; row < Pins::ROWS; ++row) {
            grid_.selectRow(row);
            for (uint8_t i = 0; i < Pins::COLS; ++i) {
                const auto& col = Pins::COL[i];
                ADC_HandleTypeDef* hadc = grid_.getAdc(col.adc);
                grid_.configureChannel(hadc, col.ch);
                HAL_ADC_Start(hadc);
                HAL_ADC_PollForConversion(hadc, 10);
                if (totalReads < 1000) values[totalReads++] = HAL_ADC_GetValue(hadc);
                HAL_ADC_Stop(hadc);
            }
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkLLPerPinGrid(uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;

    initHalDualSingleMode(grid_.getAdc(ADC2), grid_.getAdc(ADC3));

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t row = 0; row < Pins::ROWS; ++row) {
            grid_.selectRow(row);
            for (uint8_t i = 0; i < Pins::COLS; ++i) {
                const auto& col = Pins::COL[i];
                ADC_TypeDef* adc = col.adc;
                grid_.configureChannel(grid_.getAdc(adc), col.ch);
                SET_BIT(adc->CR2, ADC_CR2_ADON);
                SET_BIT(adc->CR2, ADC_CR2_SWSTART);
                while (!(adc->SR & ADC_SR_EOC)) {}
                if (totalReads < 1000) values[totalReads++] = static_cast<uint16_t>(adc->DR);
                CLEAR_BIT(adc->CR2, ADC_CR2_ADON);
            }
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkHalSeqDma(uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;
    uint16_t dmaBuf2[ADC2_SEQ_LEN];
    uint16_t dmaBuf3[ADC3_SEQ_LEN];

    ADC_HandleTypeDef* hadc2 = grid_.getAdc(ADC2);
    ADC_HandleTypeDef* hadc3 = grid_.getAdc(ADC3);
    hadc2->Lock = HAL_UNLOCKED;
    hadc3->Lock = HAL_UNLOCKED;

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t row = 0; row < Pins::ROWS; ++row) {
            grid_.selectRow(row);
            grid_.configureSequence(hadc2, adc2Channels_, ADC2_SEQ_LEN);
            grid_.configureSequence(hadc3, adc3Channels_, ADC3_SEQ_LEN);

            HAL_ADC_Start_DMA(hadc2, (uint32_t*)dmaBuf2, ADC2_SEQ_LEN);
            HAL_ADC_Start_DMA(hadc3, (uint32_t*)dmaBuf3, ADC3_SEQ_LEN);

            HAL_DMA_PollForTransfer(&grid_.hdma2_, HAL_DMA_FULL_TRANSFER, 10);
            HAL_DMA_PollForTransfer(&grid_.hdma3_, HAL_DMA_FULL_TRANSFER, 10);

            for (uint8_t i = 0; i < ADC2_SEQ_LEN && totalReads < 1000; ++i)
                values[totalReads++] = dmaBuf2[i];
            for (uint8_t i = 0; i < ADC3_SEQ_LEN && totalReads < 1000; ++i)
                values[totalReads++] = dmaBuf3[i];

            HAL_ADC_Stop_DMA(hadc2);
            HAL_ADC_Stop_DMA(hadc3);
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkLLSeqDma(uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;
    uint16_t dmaBuf2[ADC2_SEQ_LEN];
    uint16_t dmaBuf3[ADC3_SEQ_LEN];

    ADC_TypeDef* adc2 = ADC2;
    ADC_TypeDef* adc3 = ADC3;
    DMA_Stream_TypeDef* dma2 = DMA2_Stream2;
    DMA_Stream_TypeDef* dma3 = DMA2_Stream0;

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        for (uint8_t row = 0; row < Pins::ROWS; ++row) {
            grid_.selectRow(row);

            setupLLSequence(adc2, adc3, adc2Channels_, ADC2_SEQ_LEN, adc3Channels_, ADC3_SEQ_LEN);

            dma2->M0AR = (uint32_t)dmaBuf2;
            dma2->NDTR = ADC2_SEQ_LEN;
            dma2->CR |= DMA_SxCR_EN;

            dma3->M0AR = (uint32_t)dmaBuf3;
            dma3->NDTR = ADC3_SEQ_LEN;
            dma3->CR |= DMA_SxCR_EN;

            SET_BIT(adc2->CR2, ADC_CR2_DMA);
            SET_BIT(adc3->CR2, ADC_CR2_DMA);
            SET_BIT(adc2->CR2, ADC_CR2_ADON);
            SET_BIT(adc3->CR2, ADC_CR2_ADON);
            SET_BIT(adc2->CR2, ADC_CR2_SWSTART);
            SET_BIT(adc3->CR2, ADC_CR2_SWSTART);

            while (!(DMA2->LISR & DMA_LISR_TCIF2)) {}
            DMA2->LIFCR = DMA_LIFCR_CTCIF2;
            while (!(DMA2->LISR & DMA_LISR_TCIF0)) {}
            DMA2->LIFCR = DMA_LIFCR_CTCIF0;

            for (uint8_t i = 0; i < ADC2_SEQ_LEN && totalReads < 1000; ++i)
                values[totalReads++] = dmaBuf2[i];
            for (uint8_t i = 0; i < ADC3_SEQ_LEN && totalReads < 1000; ++i)
                values[totalReads++] = dmaBuf3[i];

            CLEAR_BIT(dma2->CR, DMA_SxCR_EN);
            CLEAR_BIT(dma3->CR, DMA_SxCR_EN);
            CLEAR_BIT(adc2->CR2, ADC_CR2_DMA);
            CLEAR_BIT(adc3->CR2, ADC_CR2_DMA);
            CLEAR_BIT(adc2->CR2, ADC_CR2_ADON);
            CLEAR_BIT(adc3->CR2, ADC_CR2_ADON);
        }
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkHalSeqDmaRow(uint8_t rowIdx, uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;
    uint16_t dmaBuf2[ADC2_SEQ_LEN];
    uint16_t dmaBuf3[ADC3_SEQ_LEN];

    ADC_HandleTypeDef* hadc2 = grid_.getAdc(ADC2);
    ADC_HandleTypeDef* hadc3 = grid_.getAdc(ADC3);
    hadc2->Lock = HAL_UNLOCKED;
    hadc3->Lock = HAL_UNLOCKED;

    grid_.selectRow(rowIdx);

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        grid_.configureSequence(hadc2, adc2Channels_, ADC2_SEQ_LEN);
        grid_.configureSequence(hadc3, adc3Channels_, ADC3_SEQ_LEN);

        HAL_ADC_Start_DMA(hadc2, (uint32_t*)dmaBuf2, ADC2_SEQ_LEN);
        HAL_ADC_Start_DMA(hadc3, (uint32_t*)dmaBuf3, ADC3_SEQ_LEN);

        HAL_DMA_PollForTransfer(&grid_.hdma2_, HAL_DMA_FULL_TRANSFER, 10);
        HAL_DMA_PollForTransfer(&grid_.hdma3_, HAL_DMA_FULL_TRANSFER, 10);

        for (uint8_t i = 0; i < ADC2_SEQ_LEN && totalReads < 1000; ++i)
            values[totalReads++] = dmaBuf2[i];
        for (uint8_t i = 0; i < ADC3_SEQ_LEN && totalReads < 1000; ++i)
            values[totalReads++] = dmaBuf3[i];

        HAL_ADC_Stop_DMA(hadc2);
        HAL_ADC_Stop_DMA(hadc3);
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

BenchStats SensorGridBenchmark::benchmarkLLSeqDmaRow(uint8_t rowIdx, uint16_t count) {
    uint16_t values[1000];
    uint32_t totalReads = 0;
    uint16_t dmaBuf2[ADC2_SEQ_LEN];
    uint16_t dmaBuf3[ADC3_SEQ_LEN];

    ADC_TypeDef* adc2 = ADC2;
    ADC_TypeDef* adc3 = ADC3;
    DMA_Stream_TypeDef* dma2 = DMA2_Stream2;
    DMA_Stream_TypeDef* dma3 = DMA2_Stream0;

    grid_.selectRow(rowIdx);

    uint32_t t0 = DWT->CYCCNT;
    for (uint16_t c = 0; c < count; ++c) {
        setupLLSequence(adc2, adc3, adc2Channels_, ADC2_SEQ_LEN, adc3Channels_, ADC3_SEQ_LEN);

        dma2->M0AR = (uint32_t)dmaBuf2;
        dma2->NDTR = ADC2_SEQ_LEN;
        dma2->CR |= DMA_SxCR_EN;

        dma3->M0AR = (uint32_t)dmaBuf3;
        dma3->NDTR = ADC3_SEQ_LEN;
        dma3->CR |= DMA_SxCR_EN;

        SET_BIT(adc2->CR2, ADC_CR2_DMA);
        SET_BIT(adc3->CR2, ADC_CR2_DMA);
        SET_BIT(adc2->CR2, ADC_CR2_ADON);
        SET_BIT(adc3->CR2, ADC_CR2_ADON);
        SET_BIT(adc2->CR2, ADC_CR2_SWSTART);
        SET_BIT(adc3->CR2, ADC_CR2_SWSTART);

        while (!(DMA2->LISR & DMA_LISR_TCIF2)) {}
        DMA2->LIFCR = DMA_LIFCR_CTCIF2;
        while (!(DMA2->LISR & DMA_LISR_TCIF0)) {}
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;

        for (uint8_t i = 0; i < ADC2_SEQ_LEN && totalReads < 1000; ++i)
            values[totalReads++] = dmaBuf2[i];
        for (uint8_t i = 0; i < ADC3_SEQ_LEN && totalReads < 1000; ++i)
            values[totalReads++] = dmaBuf3[i];

        CLEAR_BIT(dma2->CR, DMA_SxCR_EN);
        CLEAR_BIT(dma3->CR, DMA_SxCR_EN);
        CLEAR_BIT(adc2->CR2, ADC_CR2_DMA);
        CLEAR_BIT(adc3->CR2, ADC_CR2_DMA);
        CLEAR_BIT(adc2->CR2, ADC_CR2_ADON);
        CLEAR_BIT(adc3->CR2, ADC_CR2_ADON);
    }

    return computeStats(values, totalReads, elapsedUs(t0));
}

struct BenchResult {
    const char* label;
    BenchStats stats;
    uint32_t ops;
};

static void sortResults(BenchResult* results, uint8_t count) {
    for (int8_t i = 1; i < count; ++i) {
        BenchResult key = results[i];
        uint32_t keyTime = key.stats.duration_us;
        int8_t j = i - 1;
        while (j >= 0 && results[j].stats.duration_us > keyTime) {
            results[j + 1] = results[j];
            j--;
        }
        results[j + 1] = key;
    }
}

static void printResults(const BenchResult* results, uint8_t count, const char* unit) {
    for (uint8_t i = 0; i < count; ++i) {
        uint32_t per_op = results[i].ops > 0
            ? (results[i].stats.duration_us * (unit[0] == 'n' ? 1000u : 1u)) / results[i].ops
            : results[i].stats.duration_us * (unit[0] == 'n' ? 1000u : 1u);
        uint32_t per_op_hz = (results[i].ops > 0 && results[i].stats.duration_us > 0)
            ? (results[i].ops * 1000000u / results[i].stats.duration_us) : 0;
        RTT::printf("  %-14s | min=%5u max=%5u range=%4u p50=%5u p90=%5u p99=%5u var=%4u | %7u%s %8uHz\n",
                    results[i].label,
                    static_cast<unsigned>(results[i].stats.min), static_cast<unsigned>(results[i].stats.max),
                    static_cast<unsigned>(results[i].stats.range),
                    static_cast<unsigned>(results[i].stats.p50), static_cast<unsigned>(results[i].stats.p90),
                    static_cast<unsigned>(results[i].stats.p99), static_cast<unsigned>(results[i].stats.variance),
                    per_op, unit, per_op_hz);
    }
}

void SensorGridBenchmark::run() {
    constexpr uint16_t SENSOR_ITERS = 1000;
    constexpr uint16_t ROW_ITERS = 50;
    constexpr uint16_t GRID_ITERS = 4;

    BenchResult sensorResults[2] = {
        { "HAL per_pin", benchmarkHalPerPin(0, SENSOR_ITERS), SENSOR_ITERS },
        { "LL per_pin",  benchmarkLLPerPin(0, SENSOR_ITERS), SENSOR_ITERS },
    };
    RTT::printf("1 sensor\n");
    sortResults(sensorResults, 2);
    printResults(sensorResults, 2, "ns");

    BenchResult rowResults[4] = {
        { "HAL per_pin", benchmarkHalPerPinRow(0, ROW_ITERS), ROW_ITERS },
        { "LL per_pin",  benchmarkLLPerPinRow(0, ROW_ITERS), ROW_ITERS },
        { "HAL seq dma", benchmarkHalSeqDmaRow(0, ROW_ITERS), ROW_ITERS },
        { "LL seq dma",  benchmarkLLSeqDmaRow(0, ROW_ITERS), ROW_ITERS },
    };
    RTT::printf("1 row\n");
    sortResults(rowResults, 4);
    printResults(rowResults, 4, "us");

    BenchResult gridResults[4] = {
        { "HAL per_pin", benchmarkHalPerPinGrid(GRID_ITERS), GRID_ITERS },
        { "LL per_pin",  benchmarkLLPerPinGrid(GRID_ITERS), GRID_ITERS },
        { "HAL seq dma", benchmarkHalSeqDma(GRID_ITERS), GRID_ITERS },
        { "LL seq dma",  benchmarkLLSeqDma(GRID_ITERS), GRID_ITERS },
    };
    RTT::printf("full grid\n");
    sortResults(gridResults, 4);
    printResults(gridResults, 4, "us");

    RTT::printf("=========================\n");
}
