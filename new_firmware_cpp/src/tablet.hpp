#pragma once
#include <cstdint>
#include "sensor_grid.hpp"
#include "telemetry.hpp"

class Tablet {
public:
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
    void update_cursor(const Cursor& cursor);

    SensorGrid sensor_grid_;
    Telemetry telemetry_;
    uint16_t grid_[209];
    uint16_t threshold_ = 2200;

    uint32_t tick_start_ = 0;
    uint32_t iterations_ = 0;
    uint64_t sum_scan_ = 0;
    uint64_t sum_centroid_ = 0;
    uint64_t sum_usb_ = 0;
    uint64_t sum_cycles_ = 0;
};
