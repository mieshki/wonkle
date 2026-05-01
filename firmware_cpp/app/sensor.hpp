#pragma once
#include <cstdint>

namespace Sensor {
    static constexpr uint8_t ROW_LEN = 11;
    static constexpr uint8_t COL_LEN = 19;

    struct Cursor {
        float x;
        float y;
        bool valid;
    };

    void init();
    uint16_t read(uint8_t row, uint8_t col);
    void read_row(uint8_t row, uint16_t* out);
    void scan(uint16_t* out);
    Cursor find_center(const uint16_t* grid, uint16_t threshold = 2200);
}
