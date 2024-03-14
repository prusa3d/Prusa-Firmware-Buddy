/// @file axis.h
#pragma once
#include <stdint.h>
#include "../unit.h"

namespace config {

using namespace unit;

/// Available microstepping resolutions
enum MRes : uint8_t {
    MRes_256 = 0,
    MRes_128 = 1,
    MRes_64 = 2,
    MRes_32 = 3,
    MRes_16 = 4,
    MRes_8 = 5,
    MRes_4 = 6,
    MRes_2 = 7,
    MRes_1 = 8
};

/// Axis configuration data
struct AxisConfig {
    bool dirOn; ///< direction ON state (for inversion)
    MRes mRes; ///< microstepping [0-8, where 0 is x256 and 8 is fullstepping]
    uint8_t iRun; ///< running current
    uint8_t iHold; ///< holding current
    bool stealth; ///< Default to Stealth mode
    long double stepsPerUnit; ///< steps per unit
    int8_t sg_thrs; /// @todo 7bit two's complement for the sg_thrs
};

/// List of available axes
enum Axis : uint8_t {
    Pulley,
    Selector,
    Idler,
    _Axis_Last = Idler
};

/// Number of available axes
static constexpr uint8_t NUM_AXIS = Axis::_Axis_Last + 1;

/// Phisical limits for an axis
template <UnitBase B>
struct AxisLimits {
    static constexpr UnitBase base = B;
    Unit<long double, B, Lenght> lenght; ///< Longest move that can be performed by the axis
    Unit<long double, B, Speed> jerk; ///< Maximum jerk for the axis
    Unit<long double, B, Accel> accel; ///< Maximum acceleration for the axis
};

typedef AxisLimits<Millimeter> PulleyLimits; ///< Pulley axis limits
typedef AxisLimits<Millimeter> SelectorLimits; ///< Selector axis limits
typedef AxisLimits<Degree> IdlerLimits; ///< Idler axis limits

} // namespace config
