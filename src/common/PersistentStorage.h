/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <config_store/store_definition.hpp>

class PersistentStorage {
public:
    static constexpr uint8_t homeSamplesCount = config_store_ns::CurrentStore::precise_homing_axis_sample_count;
    static void pushHomeSample(uint16_t mscnt, uint8_t axis);
    static bool isCalibratedHome(uint16_t (&mscnt)[homeSamplesCount], uint8_t axis);
    static void erase_axis(uint8_t axis);
};
