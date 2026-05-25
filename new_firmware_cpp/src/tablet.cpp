#include "tablet.hpp"
#include "system_init.hpp"
#include "pins.hpp"
#include "usb/usb.hpp"
#include "logger/rtt.hpp"

void Tablet::init() {
    PlatformInit();

    RTT::printf("Waiting for 3 seconds...\n");
    HAL_Delay(USB_ENUM_DELAY_MS);

    RTT::printf("Initializing...\n");
    USB::init();
    sensor_grid_.init();
    telemetry_.init(sensor_grid_, profiler_);
    profiler_.init();
    RTT::printf("Init done\n");
    RTT::printf("Config: mux_settling=%u, adc_sampling=%s, adc_re_reads=%u\n",
                static_cast<unsigned>(sensor_grid_.getMuxSettling()),
                adcSamplingLabel(sensor_grid_.getAdcSampling()),
                static_cast<unsigned>(sensor_grid_.getAdcReReads()));
}

void Tablet::tick(bool measure) {
    profiler_.begin(measure);

    telemetry_.service();
    profiler_.mark_telemetry_service();

    sensor_grid_.scan_grid(grid_.data());
    profiler_.mark_scan();
    profiler_.record_scan_details(sensor_grid_);

    auto cursor = find_centroid(grid_.data());
    profiler_.mark_centroid();

    update_cursor(cursor);

    profiler_.mark_usb();
    profiler_.report_if_due();

    telemetry_.feed_grid(grid_.data(), cursor.x, cursor.y, cursor.valid);
}

void Tablet::update_cursor(const Cursor& cursor) {
    HidReport report = {0};
    if (cursor.valid) {
        uint16_t x_usb = static_cast<uint16_t>(cursor.x * GRID_TO_HID_SCALE / GRID_COLS_MAX);
        uint16_t y_usb = static_cast<uint16_t>(cursor.y * GRID_TO_HID_SCALE / GRID_ROWS_MAX);
        report.flags = HID_IN_RANGE;
        report.x = static_cast<int16_t>(x_usb);
        report.y = static_cast<int16_t>(y_usb);
    }
    USB::send_hid_report(reinterpret_cast<const uint8_t*>(&report), sizeof(report));
}

Tablet::Cursor Tablet::find_centroid(const uint16_t* grid) {
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float total = 0.0f;
    uint8_t clustered_count = 0;

    for (uint8_t r = 0; r < Pins::ROWS; r++) {
        for (uint8_t c = 0; c < Pins::COLS; c++) {
            uint16_t raw = grid[r * Pins::COLS + c];
            if (raw > threshold_) {
                bool has_neighbor =
                    (r > 0     && grid[(r - 1) * Pins::COLS + c] > threshold_) ||
                    (r < Pins::ROWS - 1 && grid[(r + 1) * Pins::COLS + c] > threshold_) ||
                    (c > 0     && grid[r * Pins::COLS + (c - 1)] > threshold_) ||
                    (c < Pins::COLS - 1  && grid[r * Pins::COLS + (c + 1)] > threshold_);

                if (has_neighbor) {
                    clustered_count++;
                    float val = static_cast<float>(raw - threshold_);
                    sum_x += val * static_cast<float>(c);
                    sum_y += val * static_cast<float>(r);
                    total += val;
                }
            }
        }
    }

    if (clustered_count >= 3 && total > 0.0f) {
        return { sum_x / total, sum_y / total, true };
    }
    return { 0.0f, 0.0f, false };
}
