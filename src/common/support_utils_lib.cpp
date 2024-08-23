#include <string.h>
#include <stdio.h>

#include "support_utils_lib.hpp"

void block2hex(char *str, uint32_t str_size, uint8_t *pdata, size_t length) {
    for (; length > 0; --length, str += 2, str_size -= 2) {
        snprintf(str, str_size, "%02X", *(pdata++));
    }
}

void leave_numbers(const char *const str_in, char *str_out) {
    int i = 0;
    while (str_in[i++] != 0) {
        if (str_in[i] < '0' || '9' < str_in[i]) {
            str_out[i] = '_';
        } else {
            str_out[i] = str_in[i];
        }
    }
}

void setBit(uint8_t *c, const uint8_t b) {
    *c |= 1 << b;
}

void clearBit(uint8_t *c, const uint8_t b) {
    *c &= ~(1 << b);
}

void rShift2Bits(uint32_t &toShift, uint32_t &overflow) {
    overflow &= 0x3FFF'FFFF; /// clear 2 MBS bits
    overflow |= (toShift & 3) << 30; /// add 2 bits
    toShift >>= 2; /// shift number
}

char to32(uint8_t number[], uint8_t startBit) {
    uint8_t val = 0;

    const uint8_t byte = startBit / 8;
    const uint8_t bit = startBit % 8;

    val = ((0b11111000 >> bit) & number[byte]);
    /// align the first part so it starts at 4th bit from the left
    if (bit > 3) {
        val <<= (bit - 3);
        val |= ((0b11111000 << (8 - bit)) & number[byte + 1]) >> (8 - bit + 3);
    } else {
        val >>= (3 - bit);
    }

    if (val >= 32) {
        return '!';
    }

    if (val < 10) {
        return char(val + '0');
    }
    return char(val - 10 + 'A'); // 10 = A, 11 =B, ...
}
