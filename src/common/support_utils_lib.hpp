#pragma once

#include <cstdint>
#include <cstdlib>

/// Converts binary data to string of hex numbers
/// \param str output string space
/// \param st_size available space for string
/// \param pdata binary data to conver
/// \param length size of data to convert
void block2hex(char *str, uint32_t str_size, uint8_t *pdata, size_t length);

/// Replace everything but numbers by underscore.
void leave_numbers(const char *const str_in, char *str_out);

void setBit(uint8_t *c, const uint8_t b);
void clearBit(uint8_t *c, const uint8_t b);

/// Shifts 1st number by 2 bits.
/// Places overflow to the MSBs of the 2nd number.
void rShift2Bits(uint32_t &toShift, uint32_t &overflow);

/// Encodes number into character
/// Uses 5 bit encoding (0-9,A-V)
/// \param number array of bytes representing big number
/// \param startBit location of the first bit
/// 0 == 1st byte, 1st bit == number MSB == output MSB
/// \returns character
char to32(uint8_t number[], uint8_t startBit);
