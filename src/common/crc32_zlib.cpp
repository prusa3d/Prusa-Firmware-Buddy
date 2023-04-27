#include <stdint.h>
#include <stddef.h>
#include "crc32.h"

extern "C" uint32_t crc32_z(uint32_t crc, const uint8_t *buf, uint32_t len) {
    return crc32_calc_ex(crc, buf, len);
}

extern "C" uint32_t crc32(uint32_t crc, const uint8_t *buf, size_t len) {
    return crc32_z(crc, buf, len);
}
