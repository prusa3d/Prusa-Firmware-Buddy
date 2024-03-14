/// @file
#include "crc.h"

#ifdef __AVR__
#include <util/crc16.h>
#endif

namespace modules {
namespace crc {

#ifdef __AVR__
uint8_t CRC8::CCITT_update(uint8_t crc, uint8_t b) {
    return _crc8_ccitt_update(crc, b);
}
#else
uint8_t CRC8::CCITT_update(uint8_t crc, uint8_t b) {
    return CCITT_updateCX(crc, b);
}
#endif

} // namespace crc
} // namespace modules
