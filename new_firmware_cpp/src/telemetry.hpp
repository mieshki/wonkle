#pragma once
#include <cstdint>

class Telemetry {
public:
    void feed_grid(const uint16_t *grid, float cx, float cy, bool cvalid);

private:
    uint8_t divider_ = 0;
};
