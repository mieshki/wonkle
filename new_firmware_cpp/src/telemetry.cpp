#include "telemetry.hpp"
#include "usb/usb.hpp"

void Telemetry::feed_grid(const uint16_t *grid, float cx, float cy, bool cvalid) {
    if (++divider_ < 11) return;
    divider_ = 0;
    if (USB::is_subscribed(USB::SUB_GRID)) {
        USB::send_cdc_grid(grid, cx, cy, cvalid);
    }
}
