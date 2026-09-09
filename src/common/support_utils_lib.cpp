#include "support_utils_lib.hpp"

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
        // unreachable
        return '!';
    }

    if (val < 10) {
        return char(val + '0');
    }
    return char(val - 10 + 'A'); // 10 = A, 11 =B, ...
}
