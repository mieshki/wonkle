#pragma once
#include "../sensor_grid.hpp"
#include <cstdint>

struct BenchStats {
    uint16_t min, max, range;
    uint32_t duration_us;
    uint16_t p50, p90, p99;
    uint32_t variance;
};

class SensorGridBenchmark {
public:
    explicit SensorGridBenchmark(SensorGrid& grid);

    BenchStats benchmarkHalPerPin(uint8_t colIdx, uint16_t count);
    BenchStats benchmarkLLPerPin(uint8_t colIdx, uint16_t count);
    BenchStats benchmarkHalPerPinRow(uint8_t rowIdx, uint16_t count);
    BenchStats benchmarkLLPerPinRow(uint8_t rowIdx, uint16_t count);
    BenchStats benchmarkHalPerPinGrid(uint16_t count);
    BenchStats benchmarkLLPerPinGrid(uint16_t count);
    BenchStats benchmarkHalSeqDma(uint16_t count);
    BenchStats benchmarkLLSeqDma(uint16_t count);
    BenchStats benchmarkHalSeqDmaRow(uint8_t rowIdx, uint16_t count);
    BenchStats benchmarkLLSeqDmaRow(uint8_t rowIdx, uint16_t count);

    void run();

private:
    static constexpr uint8_t ADC2_SEQ_LEN = 11;
    static constexpr uint8_t ADC3_SEQ_LEN = 8;
    uint8_t adc2Channels_[ADC2_SEQ_LEN];
    uint8_t adc3Channels_[ADC3_SEQ_LEN];
    void buildSeqChannels();

private:
    SensorGrid& grid_;
};
