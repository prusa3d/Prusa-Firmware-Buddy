/**
 * @file
 */

#pragma once

#include <stdint.h>

class PersistentStorage {
public:
    static constexpr uint8_t homeSamplesCount = 9;
    static void pushHomeSample(uint16_t mscnt, uint8_t board_temp, uint8_t axis);
    static bool isCalibratedHome(uint16_t (&mscnt)[homeSamplesCount], uint8_t axis);
    static void erase_axis(uint8_t axis);
    static void erase();
};
