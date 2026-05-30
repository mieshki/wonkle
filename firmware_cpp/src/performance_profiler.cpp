#include "performance_profiler.hpp"
#include "sensor_grid.hpp"
#include "logger/rtt.hpp"

extern "C" {
#include "stm32f4xx_hal.h"
#include "core_cm4.h"
}

void PerformanceProfiler::init() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    tick_start_ = HAL_GetTick();
}

void PerformanceProfiler::begin(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) return;
    t_begin_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_telemetry_service() {
    if (!enabled_) return;
    t_telemetry_service_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_scan() {
    if (!enabled_) return;
    t_scan_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_centroid() {
    if (!enabled_) return;
    t_centroid_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_usb() {
    if (!enabled_) return;
    t_usb_ = DWT->CYCCNT;
    sum_telemetry_service_  += t_telemetry_service_ - t_begin_;
    sum_scan_     += t_scan_ - t_telemetry_service_;
    sum_centroid_ += t_centroid_ - t_scan_;
    sum_usb_      += t_usb_ - t_centroid_;
    sum_cycles_   += t_usb_ - t_begin_;
    iterations_++;
}

void PerformanceProfiler::record_scan_details(const SensorGrid& grid) {
    if (!enabled_) return;
    const auto& t = grid.getLastScanTiming();
    sum_mux_ += t.mux_cycles;
    sum_single_read_ += t.single_read_cycles;
    sum_tuning_overhead_ += t.tuning_overhead_cycles;
}

void PerformanceProfiler::report_if_due() {
    if (!enabled_) return;
    uint32_t elapsed = HAL_GetTick() - tick_start_;
    if (elapsed >= REPORT_INTERVAL_MS) {
        uint32_t hz = (iterations_ * 1000) / elapsed;
        uint64_t cycles_per_iter = sum_cycles_ / iterations_;
        uint32_t us_total = static_cast<uint32_t>((static_cast<uint64_t>(cycles_per_iter) * 1000000ULL) / SystemCoreClock);
        uint32_t us_telemetry_service = static_cast<uint32_t>((sum_telemetry_service_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_scan = static_cast<uint32_t>((sum_scan_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_centroid = static_cast<uint32_t>((sum_centroid_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_usb = static_cast<uint32_t>((sum_usb_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_mux = static_cast<uint32_t>((sum_mux_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_single_read = static_cast<uint32_t>((sum_single_read_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_tuning_overhead = static_cast<uint32_t>((sum_tuning_overhead_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        last_hz_ = hz;
        last_telemetry_service_us_ = us_telemetry_service;
        last_scan_us_ = us_scan;
        last_centroid_us_ = us_centroid;
        last_usb_us_ = us_usb;
        last_mux_us_ = us_mux;
        last_single_read_us_ = us_single_read;
        last_tuning_overhead_us_ = us_tuning_overhead;
        uint32_t pct_telemetry_service  = sum_cycles_ > 0 ? static_cast<uint32_t>((sum_telemetry_service_ * 1000ULL) / sum_cycles_) : 0;
        uint32_t pct_scan = sum_cycles_ > 0 ? static_cast<uint32_t>((sum_scan_ * 1000ULL) / sum_cycles_) : 0;
        uint32_t pct_centroid = sum_cycles_ > 0 ? static_cast<uint32_t>((sum_centroid_ * 1000ULL) / sum_cycles_) : 0;
        uint32_t pct_usb  = sum_cycles_ > 0 ? static_cast<uint32_t>((sum_usb_ * 1000ULL) / sum_cycles_) : 0;
        RTT::printf("%u Hz  %uus/frame  (telemetry: %u.%u%%  scan: %u.%u%%  centroid: %u.%u%%  usb: %u.%u%%)\n",
                    static_cast<unsigned>(hz),
                    static_cast<unsigned>(us_total),
                    static_cast<unsigned>(pct_telemetry_service / 10), static_cast<unsigned>(pct_telemetry_service % 10),
                    static_cast<unsigned>(pct_scan / 10), static_cast<unsigned>(pct_scan % 10),
                    static_cast<unsigned>(pct_centroid / 10), static_cast<unsigned>(pct_centroid % 10),
                    static_cast<unsigned>(pct_usb / 10), static_cast<unsigned>(pct_usb % 10));
        RTT::printf("         scan details: mux=%uus  read=%uus  oversample=%uus\n",
                    static_cast<unsigned>(us_mux),
                    static_cast<unsigned>(us_single_read),
                    static_cast<unsigned>(us_tuning_overhead));
        tick_start_ = HAL_GetTick();
        iterations_ = 0;
        sum_telemetry_service_ = 0;
        sum_scan_ = 0;
        sum_centroid_ = 0;
        sum_usb_ = 0;
        sum_cycles_ = 0;
        sum_mux_ = 0;
        sum_single_read_ = 0;
        sum_tuning_overhead_ = 0;
    }
}
