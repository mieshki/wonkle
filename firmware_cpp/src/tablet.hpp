#pragma once
#include <cstdint>
#include <array>
#include "sensor_grid.hpp"
#include "telemetry.hpp"
#include "performance_profiler.hpp"

class Tablet {
public:
    static constexpr uint8_t    HID_REPORT_SIZE   = 8;
    static constexpr uint8_t    HID_IN_RANGE      = 0x02;
    static constexpr uint32_t   USB_ENUM_DELAY_MS = 3000;
    static constexpr float      GRID_TO_HID_SCALE = 10000.0f;
    static constexpr float      GRID_COLS_MAX     = 18.0f;
    static constexpr float      GRID_ROWS_MAX     = 10.0f;

    struct __attribute__((packed)) HidReport {
        uint8_t buttons;
        uint8_t flags;
        int16_t x;
        int16_t y;
        uint16_t _pad;
    };
    static_assert(sizeof(HidReport) == 8, "HidReport padding");

    struct Cursor {
        float x;
        float y;
        bool valid;
    };

    void init();
    void tick(bool measure = true);

    void set_threshold(uint16_t t) { threshold_ = t; }
    SensorGrid& get_sensor_grid() { return sensor_grid_; }

private:
    Cursor find_centroid(const uint16_t* grid);
    void update_cursor(Cursor& cursor);

    SensorGrid sensor_grid_;
    Telemetry telemetry_;
    PerformanceProfiler profiler_;
    std::array<uint16_t, 209> grid_;
    uint16_t threshold_ = 2200;

    bool ema_initialized_ = false;
    float smoothed_x_ = 0.0f;
    float smoothed_y_ = 0.0f;
};
