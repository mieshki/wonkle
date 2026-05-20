#pragma once
#include <cstdint>

class SensorGrid;

class PerformanceProfiler {
public:
    static constexpr uint32_t REPORT_INTERVAL_MS = 2000;

    void init();
    void begin(bool enabled);
    void mark_scan();
    void mark_centroid();
    void mark_usb();
    void record_scan_details(const class SensorGrid& grid);
    void report_if_due();

    uint32_t get_last_hz() const { return last_hz_; }
    uint32_t get_last_scan_us() const { return last_scan_us_; }
    uint32_t get_last_centroid_us() const { return last_centroid_us_; }
    uint32_t get_last_usb_us() const { return last_usb_us_; }
    uint32_t get_last_mux_us() const { return last_mux_us_; }
    uint32_t get_last_single_read_us() const { return last_single_read_us_; }
    uint32_t get_last_tuning_overhead_us() const { return last_tuning_overhead_us_; }

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
    uint64_t sum_mux_ = 0;
    uint64_t sum_single_read_ = 0;
    uint64_t sum_tuning_overhead_ = 0;

    uint32_t last_hz_ = 0;
    uint32_t last_scan_us_ = 0;
    uint32_t last_centroid_us_ = 0;
    uint32_t last_usb_us_ = 0;
    uint32_t last_mux_us_ = 0;
    uint32_t last_single_read_us_ = 0;
    uint32_t last_tuning_overhead_us_ = 0;
};
