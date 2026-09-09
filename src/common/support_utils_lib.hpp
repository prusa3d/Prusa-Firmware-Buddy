#pragma once

#include <cstdint>
#include <cstdlib>

/// Encodes number into character
/// Uses 5 bit encoding (0-9,A-V)
/// \param number array of bytes representing big number
/// \param startBit location of the first bit
/// 0 == 1st byte, 1st bit == number MSB == output MSB
/// \returns character
char to32(uint8_t number[], uint8_t startBit);
