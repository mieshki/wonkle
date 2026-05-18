#include "telemetry.hpp"
#include "usb/usb.hpp"
#include "logger/rtt.hpp"
#include "pins.hpp"

extern "C" {
#include "crc16.h"
}

void Telemetry::init(SensorGrid& grid) {
    grid_ = &grid;
}

void Telemetry::service() {
    parse_rx();
}

static uint16_t frame_crc(const uint8_t* data, uint16_t len) {
    return crc16_ccitt(data, len);
}

void Telemetry::feed_grid(const uint16_t *grid, float cx, float cy, bool cvalid) {
    if (++grid_tick_counter_ < GRID_SEND_EVERY_N_TICKS) return;
    grid_tick_counter_ = 0;

    if (!is_subscribed(SUB_GRID)) return;

    static CdcGridFrame frame;
    frame.header.sync_lo  = SYNC_LO;
    frame.header.sync_hi  = SYNC_HI;
    frame.header.version  = PROTOCOL_VERSION;
    frame.header.msg_type = MSG_GRID;
    frame.header.seq      = frame_counter_;

    for (uint8_t i = 0; i < Pins::ROWS * Pins::COLS; i++) {
        frame.payload.values[i] = grid[i];
    }
    frame.payload.cursor_x     = static_cast<int16_t>(cx * 100.0f);
    frame.payload.cursor_y     = static_cast<int16_t>(cy * 100.0f);
    frame.payload.cursor_valid = cvalid ? 1 : 0;

    uint8_t *payload = reinterpret_cast<uint8_t*>(&frame);
    uint16_t payload_len = static_cast<uint16_t>(offsetof(CdcGridFrame, crc));
    frame.crc = frame_crc(payload, payload_len);

    if (frame_counter_ < 3) {
        RTT::printf("GRID#%u subs=0x%02x\n",
                    static_cast<unsigned>(frame_counter_),
                    static_cast<unsigned>(subscriptions_));
    }
    USB::cdc_send_frame(reinterpret_cast<const uint8_t*>(&frame), sizeof(frame));
    frame_counter_++;
}

void Telemetry::send_config() {
    static CdcConfigResponse resp;
    resp.sync_lo  = SYNC_LO;
    resp.sync_hi  = SYNC_HI;
    resp.version  = PROTOCOL_VERSION;
    resp.msg_type = 0x20;
    resp.seq      = 0;
    resp.mux_settling    = grid_->getMuxSettling();
    resp.adc_sampling    = static_cast<uint8_t>(grid_->getAdcSampling());
    resp.adc_dummy_reads  = grid_->getAdcDummyReads();
    resp.adc_oversample   = grid_->getOversampleEnabled() ? 1 : 0;

    uint8_t *payload = reinterpret_cast<uint8_t*>(&resp);
    uint16_t payload_len = static_cast<uint16_t>(offsetof(CdcConfigResponse, crc));
    resp.crc = frame_crc(payload, payload_len);

    USB::cdc_send_frame(reinterpret_cast<const uint8_t*>(&resp), sizeof(resp));
}

void Telemetry::parse_rx() {
    uint8_t buf[64];
    uint16_t n = USB::cdc_rx_pop(buf, sizeof(buf));
    if (n > 0) {
        on_command(buf[0], &buf[1], n - 1);
    }
}

void Telemetry::on_command(uint8_t cmd, const uint8_t *data, uint8_t len) {
    switch (cmd) {
    case CMD_SUBSCRIBE:
        if (len >= 1) {
            subscriptions_ |= data[0];
        } else {
            subscriptions_ |= SUB_GRID;
        }
        RTT::printf("CDC: subscribe 0x%02x (now=0x%02x)\n",
                    static_cast<unsigned>(len >= 1 ? data[0] : SUB_GRID),
                    static_cast<unsigned>(subscriptions_));
        break;
    case CMD_UNSUBSCRIBE:
        subscriptions_ = 0;
        RTT::printf("CDC: unsubscribe all\n");
        break;
    case CMD_UNSUBSCRIBE_TO:
        if (len >= 1) {
            subscriptions_ &= ~data[0];
            RTT::printf("CDC: unsubscribe 0x%02x (now=0x%02x)\n",
                        static_cast<unsigned>(data[0]),
                        static_cast<unsigned>(subscriptions_));
        }
        break;
    case CMD_SET_MUX_SETTLING:
        if (len >= 4) {
            uint32_t cycles = (uint32_t)data[0] | ((uint32_t)data[1] << 8)
                            | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
            grid_->setMuxSettling(cycles);
            RTT::printf("CDC: mux_settling=%u\n", static_cast<unsigned>(cycles));
        }
        break;
    case CMD_SET_ADC_SAMPLING:
        if (len >= 1) {
            grid_->setAdcSampling(static_cast<AdcSampling>(data[0]));
            RTT::printf("CDC: adc_sampling=%s\n", adcSamplingLabel(static_cast<AdcSampling>(data[0])));
        }
        break;
    case CMD_GET_CONFIG:
        RTT::printf("CDC: get_config requested\n");
        send_config();
        break;
    case CMD_SET_ADC_DUMMY_READS:
        if (len >= 1) {
            grid_->setAdcDummyReads(data[0]);
            RTT::printf("CDC: adc_dummy_reads=%u\n", static_cast<unsigned>(data[0]));
        }
        break;
    case CMD_SET_ADC_OVERSAMPLE:
        if (len >= 1) {
            grid_->setOversampleEnabled(data[0] != 0);
            RTT::printf("CDC: adc_oversample=%s\n", data[0] ? "on" : "off");
        }
        break;
    default:
        RTT::printf("CDC: Unknown command!\n");
        break;
    }
}
