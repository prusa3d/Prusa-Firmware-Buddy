#pragma once

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Initialize the CRC peripheral and asociated mutex
extern void crc32_init(void);

/// Calculate reverse CRC32 for a given buffer
extern uint32_t crc32_calc(const uint8_t *data, uint32_t count);

/// Calculate reverse CRC32 for a given buffer with an explicit initial CRC value
extern uint32_t crc32_calc_ex(uint32_t crc, const uint8_t *data, uint32_t count);

/// Calculate normal CRC32 for EEPROM
/// Do not use anywhere else than EEPROM - necessary for backwards compatibility
/// @param data 4-byte aligned data of size divisible by 4
/// @param count numbed of dwords
extern uint32_t crc32_eeprom(const uint32_t *data, uint32_t count);

#ifdef __cplusplus
}
#endif //__cplusplus
