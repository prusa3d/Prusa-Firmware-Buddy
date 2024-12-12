/*
 * CoreXY precise homing persistent configuration data
 */

#pragma once
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <cmath>

struct __attribute__((packed)) CoreXYGridOrigin {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!
    float origin[2];
    float distance[2];

    friend auto operator<=>(const CoreXYGridOrigin &, const CoreXYGridOrigin &) = default;

    bool uninitialized() const {
        return std::isnan(origin[0]) || std::isnan(origin[1])
            || std::isnan(distance[0]) || std::isnan(distance[1]);
    }
};

static constexpr CoreXYGridOrigin COREXY_NO_GRID_ORIGIN = CoreXYGridOrigin {
    .origin = { NAN, NAN },
    .distance = { NAN, NAN },
};

#if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
struct __attribute__((packed)) CoreXYHomeTMCSens {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!
    float feedrate;
    int8_t sensitivity;
    uint16_t current;

    friend auto operator<=>(const CoreXYHomeTMCSens &, const CoreXYHomeTMCSens &) = default;

    bool uninitialized() const {
        return std::isnan(feedrate) || current == 0 || sensitivity == INT8_MIN;
    }
};

static constexpr CoreXYHomeTMCSens COREXY_NO_HOME_TMC_SENS = CoreXYHomeTMCSens {
    .feedrate = NAN,
    .sensitivity = INT8_MIN,
    .current = 0,
};
#endif
