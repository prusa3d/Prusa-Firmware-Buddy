/*
 * CoreXY precise homing persistent configuration data
 */

#pragma once
#include <cmath>

struct __attribute__((packed)) CoreXYGridOrigin {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!
    float origin[2];

    friend auto operator<=>(const CoreXYGridOrigin &, const CoreXYGridOrigin &) = default;

    bool uninitialized() const {
        return std::isnan(origin[0]) || std::isnan(origin[1]);
    }
};

static constexpr CoreXYGridOrigin COREXY_NO_GRID_ORIGIN = CoreXYGridOrigin {
    .origin = { NAN, NAN },
};
