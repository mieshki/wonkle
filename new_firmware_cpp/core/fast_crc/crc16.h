#ifndef __CRC16_H
#define __CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
