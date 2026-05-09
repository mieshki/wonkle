#pragma once

#include <cstdint>

namespace USB {
    void init();
    void poll();
    bool send_report(uint16_t x, uint16_t y, bool in_range, bool tip);
}
