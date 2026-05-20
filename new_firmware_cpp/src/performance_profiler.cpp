#include "performance_profiler.hpp"
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
    t0_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_scan() {
    if (!enabled_) return;
    t1_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_centroid() {
    if (!enabled_) return;
    t2_ = DWT->CYCCNT;
}

void PerformanceProfiler::mark_usb() {
    if (!enabled_) return;
    t3_ = DWT->CYCCNT;
    sum_scan_ += t1_ - t0_;
    sum_centroid_ += t2_ - t1_;
    sum_usb_ += t3_ - t2_;
    sum_cycles_ += t3_ - t0_;
    iterations_++;
}

void PerformanceProfiler::report_if_due() {
    if (!enabled_) return;
    uint32_t elapsed = HAL_GetTick() - tick_start_;
    if (elapsed >= REPORT_INTERVAL_MS) {
        uint32_t hz = (iterations_ * 1000) / elapsed;
        uint64_t cycles_per_iter = sum_cycles_ / iterations_;
        uint32_t us_total = static_cast<uint32_t>((static_cast<uint64_t>(cycles_per_iter) * 1000000ULL) / SystemCoreClock);
        uint32_t us_scan = static_cast<uint32_t>((sum_scan_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_centroid = static_cast<uint32_t>((sum_centroid_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        uint32_t us_usb = static_cast<uint32_t>((sum_usb_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
        last_hz_ = hz;
        last_scan_us_ = us_total;
        last_centroid_us_ = us_centroid;
        last_usb_us_ = us_usb;
        RTT::printf("%u Hz | scan=%uus centroid=%uus usb=%uus total=%uus\n",
                    static_cast<unsigned>(hz),
                    static_cast<unsigned>(us_scan),
                    static_cast<unsigned>(us_centroid),
                    static_cast<unsigned>(us_usb),
                    static_cast<unsigned>(us_total));
        tick_start_ = HAL_GetTick();
        iterations_ = 0;
        sum_scan_ = 0;
        sum_centroid_ = 0;
        sum_usb_ = 0;
        sum_cycles_ = 0;
    }
}
