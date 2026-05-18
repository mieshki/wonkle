#pragma once

#include <cstdint>

namespace USB {

    constexpr uint8_t CDC_CLASS_ID = 1;

    void init();
    bool send_hid_report(const uint8_t* data, uint16_t len);
    bool cdc_send_frame(const uint8_t* data, uint16_t len);
    uint16_t cdc_rx_pop(uint8_t* buf, uint16_t maxlen);

}
