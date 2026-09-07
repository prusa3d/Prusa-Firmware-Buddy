/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>

namespace hal::crc {

void init();

uint16_t compute_crc16_modbus(Bytes);

} // namespace hal::crc
