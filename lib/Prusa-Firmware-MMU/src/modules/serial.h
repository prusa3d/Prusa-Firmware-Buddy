/// @file
#pragma once
#include <stdint.h>

namespace modules {

namespace serial {

bool WriteToUSART(const uint8_t *src, uint8_t len);

bool Available();

uint8_t ConsumeByte();

} // namespace serial

} // namespace modules
