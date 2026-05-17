#pragma once

#include <cstdint>

namespace USB {

    void set_sensor_grid(void* grid);
    void* get_sensor_grid();

    constexpr uint8_t PROTOCOL_VERSION = 0x01;

    /* ── Subscription flags ────────────────────────────────────────── */
    constexpr uint8_t SUB_GRID   = (1 << 0);

    /* ── Message types ─────────────────────────────────────────────── */
    constexpr uint8_t MSG_GRID   = 0x10;

    /* ── Command codes ─────────────────────────────────────────────── */
    constexpr uint8_t CMD_SUBSCRIBE      = 0x01;
    constexpr uint8_t CMD_UNSUBSCRIBE    = 0x04;
    constexpr uint8_t CMD_UNSUBSCRIBE_TO = 0x06;

    /* ── Composite class index ─────────────────────────────────────── */
    constexpr uint8_t CDC_CLASS_ID = 1;

    /* ── Frame structs ─────────────────────────────────────────────── */
    struct __attribute__((packed)) CdcFrameHeader {
        uint8_t  sync_lo;       // 0xAA
        uint8_t  sync_hi;       // 0x55
        uint8_t  version;       // PROTOCOL_VERSION
        uint8_t  msg_type;      // MSG_GRID, ...
        uint16_t seq;           // frame counter (little-endian)
    };

    struct __attribute__((packed)) CdcGridPayload {
        uint16_t values[209];   // 11×19 = 418 bytes
        int16_t  cursor_x;      // × 100
        int16_t  cursor_y;      // × 100
        uint8_t  cursor_valid;
    };

    struct __attribute__((packed)) CdcGridFrame {
        CdcFrameHeader header;
        CdcGridPayload payload;
        uint16_t       crc;     // CRC16-CCITT over header + payload
    };

    /* ── Core ──────────────────────────────────────────────────────── */
    void init();
    bool send_report(uint16_t x, uint16_t y, bool in_range);
    void drain_commands();

    /* ── CDC telemetry ─────────────────────────────────────────────── */
    void send_cdc_grid(const uint16_t *grid, float cx, float cy, bool cvalid);
    void send_cdc_config();
    bool is_subscribed(uint8_t flag);
    void on_cdc_command(uint8_t cmd, const uint8_t *data, uint8_t len);
}
