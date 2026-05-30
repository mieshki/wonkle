# CDC Protocol

Binary protocol over USB CDC (115200 8N1). Sync word `0xAA 0x55`, CRC-16-CCITT on payload (excluding CRC field). All multi-byte fields little-endian.

## Frame Header (6 bytes)

| Offset | Size | Type   | Field     |
|--------|------|--------|-----------|
| 0      | 1    | uint8  | sync_lo (0xAA) |
| 1      | 1    | uint8  | sync_hi (0x55) |
| 2      | 1    | uint8  | version (0x01) |
| 3      | 1    | uint8  | msg_type  |
| 4      | 2    | uint16 | seq (wraps) |

## Device → Host

### Grid Frame (msg_type = 0x10) — 431 bytes

| Offset | Size | Type    | Field        |
|--------|------|---------|--------------|
| 0      | 6    | header  |              |
| 6      | 418  | uint16[209] | values — row-major (11×19) |
| 424    | 2    | int16   | cursor_x — scaled ×100 |
| 426    | 2    | int16   | cursor_y — scaled ×100 |
| 428    | 1    | uint8   | cursor_valid — 0 or 1 |
| 429    | 2    | uint16  | crc16        |

Grid is 11 rows × 19 columns, sent every Nth tick (default N=10) when subscribed.

### Config Response (msg_type = 0x20) — 15 bytes

| Offset | Size | Type   | Field         |
|--------|------|--------|---------------|
| 0      | 6    | header |               |
| 6      | 4    | uint32 | mux_settling (cycles) |
| 10     | 1    | uint8  | adc_sampling (index 0–7) |
| 11     | 1    | uint8  | adc_re_reads (0–10) |
| 12     | 1    | uint8  | adc_oversample (0 or 1) |
| 13     | 2    | uint16 | crc16         |

### Perf Response (msg_type = 0x30) — 40 bytes

| Offset | Size | Type   | Field             |
|--------|------|--------|-------------------|
| 0      | 6    | header |                   |
| 6      | 4    | uint32 | hz                |
| 10     | 4    | uint32 | telemetry_service_us|
| 14     | 4    | uint32 | scan_us           |
| 18     | 4    | uint32 | centroid_us       |
| 22     | 4    | uint32 | usb_us            |
| 26     | 4    | uint32 | mux_us            |
| 30     | 4    | uint32 | single_read_us    |
| 34     | 4    | uint32 | tuning_overhead_us|
| 38     | 2    | uint16 | crc16             |

## Host → Device Commands

Single-byte command + optional payload.

| Cmd  | Name            | Payload                      |
|------|-----------------|------------------------------|
| 0x01 | Subscribe       | uint8 flags (0x01=grid). Omit payload to subscribe all. |
| 0x04 | Unsubscribe All | —                            |
| 0x06 | Unsubscribe     | uint8 flags to clear         |
| 0x10 | Set Mux Settling| uint32 (cycles)              |
| 0x11 | Set ADC Sampling| uint8 index: 0=3c, 1=15c, 2=28c, 3=56c, 4=84c, 5=112c, 6=144c, 7=480c |
| 0x12 | Get Config      | —                            |
| 0x13 | Set ADC Re-Reads| uint8 count (0–10)           |
| 0x14 | Set ADC Oversample | uint8: 0=off, non-zero=on |
| 0x15 | Get Perf        | —                            |

## Example: Sending a Command

Set ADC Re-Reads to 5 (cmd `0x13`):

```
Bytes to write:  [0x13, 0x05]
```

Set Mux Settling to 2000 cycles (cmd `0x10`, uint32):

```
2000 decimal = 0x000007D0
Bytes to write:  [0x10, 0xD0, 0x07, 0x00, 0x00]
```

Subscribe to grid streaming (cmd `0x01`, flag `0x01`):

```
Bytes to write:  [0x01, 0x01]
```
