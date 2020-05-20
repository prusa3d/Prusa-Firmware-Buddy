#pragma once

#include <assert.h>
#include "qrcodegen.h"

/// ***** assumed from QRCODEGEN-library
constexpr int8_t _CEXPR_ECC_CODEWORDS_PER_BLOCK[4][41] = {
    // Version: (note that index 0 is for padding, and is set to an illegal value)
    //0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
    { -1, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 },  // Low
    { -1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28 }, // Medium
    { -1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }, // Quartile
    { -1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }, // High
};

constexpr int8_t _CEXPR_NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    // Version: (note that index 0 is for padding, and is set to an illegal value)
    //0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
    { -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 6, 7, 8, 8, 9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25 },              // Low
    { -1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5, 5, 8, 9, 9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49 },     // Medium
    { -1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68 },  // Quartile
    { -1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81 }, // High
};

constexpr int _CEXPR_getNumRawDataModules(int ver) {
    assert(qrcodegen_VERSION_MIN <= ver && ver <= qrcodegen_VERSION_MAX);
    int result = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        int numAlign = ver / 7 + 2;
        result -= (25 * numAlign - 10) * numAlign - 55;
        if (ver >= 7)
            result -= 36;
    }
    assert(208 <= result && result <= 29648);
    return result;
}

constexpr int _CEXPR_getNumDataCodewords(int version, enum qrcodegen_Ecc ecl) {
    int v = version, e = (int)ecl;
    assert(0 <= e && e < 4);
    return _CEXPR_getNumRawDataModules(v) / 8
        - _CEXPR_ECC_CODEWORDS_PER_BLOCK[e][v]
        * _CEXPR_NUM_ERROR_CORRECTION_BLOCKS[e][v];
}
// *****

#define INDICATOR1 4

typedef struct
{
    size_t indicator2[3];
    size_t multiplier;
    size_t divider;
} mode_record_t;

constexpr mode_record_t data_table[] = {
    { { 10, 12, 14 }, 3, 10 }, // @ qrcodegen_Mode_NUMERIC
    { { 9, 11, 13 }, 2, 11 },  // @ qrcodegen_Mode_ALPHANUMERIC
    { { 8, 16, 16 }, 1, 8 },   // @ qrcodegen_Mode_BYTE
    { { 8, 10, 12 }, 1, 13 },  // @ qrcodegen_Mode_KANJI
};

/// maximal data length calculation
constexpr size_t grcodegen_getDataSize(int version, enum qrcodegen_Ecc ecl, enum qrcodegen_Mode mode) {
    size_t result = 0;                     // initialization due to 'constexpr'
    int version_index = 0, mode_index = 0; // initialization due to 'constexpr'

    version_index = ((version <= 9) ? 0 : ((version <= 26) ? 1 : 2));
    switch (mode) {
    case qrcodegen_Mode_NUMERIC:
        mode_index = 0;
        break;
    case qrcodegen_Mode_ALPHANUMERIC:
        mode_index = 1;
        break;
    case qrcodegen_Mode_BYTE:
        mode_index = 2;
        break;
    case qrcodegen_Mode_KANJI:
        mode_index = 3;
        break;
    default: // ECI not supported
        assert(false);
        break;
    }
    result = _CEXPR_getNumDataCodewords(version, ecl) * 8;
    result = result - INDICATOR1 - data_table[mode_index].indicator2[version_index];
    result = (result * data_table[mode_index].multiplier) / data_table[mode_index].divider;
    return (result);
}
