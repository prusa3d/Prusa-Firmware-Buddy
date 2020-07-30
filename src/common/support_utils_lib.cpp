#include <array>
#include <string.h>

#include "support_utils_lib.hpp"

char *eofstr(char *str) {
    return (str + strlen(str));
}

void block2hex(char *str, uint32_t str_size, uint8_t *pdata, size_t length) {
    for (; length > 0; length--)
        snprintf(eofstr(str), str_size - strlen(str), "%02X", *(pdata++));
}

void leave_numbers(const char *const str_in, char *str_out) {
    int i = 0;
    while (str_in[i++] != 0) {
        if (str_in[i] < '0' || '9' < str_in[i])
            str_out[i] = '_';
        else {
            str_out[i] = str_out[i];
        }
    }
}

inline void setBit(uint8_t *c, const uint8_t b) {
    *c |= 1 << b;
}

inline void clearBit(uint8_t *c, const uint8_t b) {
    *c &= ~(1 << b);
}

inline void rShift2Bits(uint32_t &toShift, uint32_t &overflow) {
    overflow &= 0x3FFF'FFFF;         /// clear 2 MBS bits
    overflow |= (toShift & 3) << 30; /// add 2 bits
    toShift >>= 2;                   /// shift number
}

char to32(uint8_t number[], uint8_t startBit) {
    uint8_t val = 0;

    const uint8_t byte = startBit / 8;
    const uint8_t bit = startBit % 8;

    val = ((255 >> bit) & number[byte]) << (5 - bit);
    /// ensure at least 1 bit is used, otherwise you can read out of number
    if (bit > 3)
        val += (255 << (8 - bit + 3)) & number[byte + 1];

    if (val < 10)
        return char(val);
    return char(val - 10 + 65); // 10 = A, 11 =B, ...
}
