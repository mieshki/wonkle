#pragma once
#include <cstdint>

class PerformanceProfiler {
public:
    static constexpr uint32_t REPORT_INTERVAL_MS = 2000;

    void init();
    void begin(bool enabled);
    void mark_scan();
    void mark_centroid();
    void mark_usb();
    void report_if_due();

private:
    bool enabled_ = false;
    uint32_t t0_ = 0;
    uint32_t t1_ = 0;
    uint32_t t2_ = 0;
    uint32_t t3_ = 0;

    uint32_t tick_start_ = 0;
    uint32_t iterations_ = 0;
    uint64_t sum_scan_ = 0;
    uint64_t sum_centroid_ = 0;
    uint64_t sum_usb_ = 0;
    uint64_t sum_cycles_ = 0;
};
