#pragma once
#include <cstdint>
#include "sensor_grid.hpp"

/* ── Protocol constants ─────────────────────────────────────────────── */
constexpr uint8_t PROTOCOL_VERSION = 0x01;

/* ── Subscription flags ─────────────────────────────────────────────── */
constexpr uint8_t SUB_GRID   = (1 << 0);

/* ── Message types ──────────────────────────────────────────────────── */
constexpr uint8_t MSG_GRID   = 0x10;

/* ── Command codes ──────────────────────────────────────────────────── */
constexpr uint8_t CMD_SUBSCRIBE         = 0x01;
constexpr uint8_t CMD_UNSUBSCRIBE       = 0x04;
constexpr uint8_t CMD_UNSUBSCRIBE_TO    = 0x06;
constexpr uint8_t CMD_SET_MUX_SETTLING  = 0x10;
constexpr uint8_t CMD_SET_ADC_SAMPLING  = 0x11;
constexpr uint8_t CMD_GET_CONFIG        = 0x12;
constexpr uint8_t CMD_SET_ADC_DUMMY_READS = 0x13;
constexpr uint8_t CMD_SET_ADC_OVERSAMPLE  = 0x14;

/* ── Frame structs (wire protocol) ──────────────────────────────────── */
struct __attribute__((packed)) CdcFrameHeader {
    uint8_t  sync_lo;
    uint8_t  sync_hi;
    uint8_t  version;
    uint8_t  msg_type;
    uint16_t seq;
};

struct __attribute__((packed)) CdcGridPayload {
    uint16_t values[209];
    int16_t  cursor_x;
    int16_t  cursor_y;
    uint8_t  cursor_valid;
};

struct __attribute__((packed)) CdcGridFrame {
    CdcFrameHeader header;
    CdcGridPayload payload;
    uint16_t       crc;
};

struct __attribute__((packed)) CdcConfigResponse {
    uint8_t  sync_lo;
    uint8_t  sync_hi;
    uint8_t  version;
    uint8_t  msg_type;
    uint16_t seq;
    uint32_t mux_settling;
    uint8_t  adc_sampling;
    uint8_t  adc_dummy_reads;
    uint8_t  adc_oversample;
    uint16_t crc;
};

class Telemetry {
public:
    void init(SensorGrid& grid);

    void service();
    void feed_grid(const uint16_t *grid, float cx, float cy, bool cvalid);
    void send_config();

    bool is_subscribed(uint8_t flag) const {
        return (subscriptions_ & flag) != 0;
    }

private:
    void on_command(uint8_t cmd, const uint8_t *data, uint8_t len);
    void parse_rx();

    SensorGrid* grid_ = nullptr;

    static constexpr uint8_t GRID_SEND_EVERY_N_TICKS = 11;

    uint8_t subscriptions_ = 0;
    uint16_t frame_counter_ = 0;
    uint8_t grid_tick_counter_ = 0;
};
