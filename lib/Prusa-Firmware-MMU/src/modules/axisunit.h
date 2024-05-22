/// @file axisunit.h
#pragma once
#include "../config/axis.h"
#include "pulse_gen.h"

namespace modules {
namespace motion {

// Import required types
using config::Axis;
using config::Idler;
using config::Pulley;
using config::Selector;

using config::Accel;
using config::Lenght;
using config::Speed;

using pulse_gen::pos_t;
using pulse_gen::steps_t;

/// Specialized axis unit type for compile-time conformability testing. Like for
/// unit::Unit this is done ensure quantities are not mixed between types, while also
/// providing convenience methods to convert from physical units to AxisUnits directly at
/// compile time. AxisUnits are just as efficient as the non-checked pulse_gen::pos_t and
/// pulse_gen::steps_t.
///
/// Each axis provides separate types for each quantity, since the low-level count is also
/// not directly comparable across each (depending on the configuration settings).
/// Quantities are normally defined through the literal operators. Types and base axes are
/// prefixed with a single letter identifier for the axis: P=pulley, S=selector, I=idler.
///
///     P_pos_t pulley_position = 10.0_P_mm;
///     auto pulley_zero = 0.0_P_mm; // implicit type
///     P_speed_ pulley_feedrate = 30.0_P_mm_s;
///     I_pos_t idler_position = 15.0_I_deg;
///     pulley_position + idler_position; // compile time error
///
/// motion::Motion.PlanMove (and related functions) support both physical and AxisUnit
/// natively. This is done by specifying the axis through the first template parameter,
/// which ensures related units are also conforming:
///
///     motion.PlanMoveTo<Pulley>(10.0_mm, 100._mm_s); // using physical units
///     motion.PlanMoveTo<Pulley>(10.0_P_mm, 100._P_mm_s); // using AxisUnit
///
/// Physical units are always represented with the largest floating point type, so they
/// should only preferably be used at compile-time only.
///
/// If runtime manipulation is necessary, AxisUnit should be used instead. Conversion from
/// physical to AxisUnit can be done through motion::unitToAxisUnit:
///
///     unitToAxisUnit<final_type>(physical_type)
///
/// Examples:
///
///     P_pos_t pulley_pos = unitToAxisUnit<P_pos_t>(10.0_mm);
///     P_speed_t pulley_speed = unitToAxisUnit<P_speed_t>(100.0_mm_s);
///
/// Conversion to pos_t or steps_t works the same using motion::unitToSteps instead.
///
/// The low-level step count can be accessed when necessary through AxisUnit::v, which
/// should be avoided as it bypasses all type checks. AxisUnit can also be constructed
/// without checks by providing a counter as the first initializer.
///
/// The scaling factor is stored with the pair config::AxisConfig::uSteps and
/// config::AxisConfig::stepsPerUnit.
template <typename T, Axis A, config::UnitType U>
struct AxisUnit {
    T v;

    static constexpr Axis axis = A;
    static constexpr config::UnitType unit = U;

    typedef T type_t;
    typedef AxisUnit<T, A, U> self_t;

    // same-type operations
    constexpr self_t operator+(const self_t r) const { return { v + r.v }; }
    constexpr self_t operator-(const self_t r) const { return { v - r.v }; }
    constexpr self_t operator-() const { return { -v }; }
    constexpr self_t operator*(const self_t r) const { return { v * r.v }; }
    constexpr self_t operator/(const self_t r) const { return { v / r.v }; }

    // allow an unitless multiplier to scale the quantity: AU * f => AU
    constexpr self_t operator*(const long double f) const { return { (T)(v * f) }; }
    constexpr self_t operator/(const long double f) const { return { (T)(v / f) }; }
};

// complementary f * AU => AU * f
template <typename T, Axis A, config::UnitType U>
constexpr AxisUnit<T, A, U> operator*(const long double f, const AxisUnit<T, A, U> u) { return u * f; }

/// Axis type conversion table for template expansion
struct AxisScale {
    unit::UnitBase base;
    long double stepsPerUnit;
};

static constexpr AxisScale axisScale[config::NUM_AXIS] = {
    { config::pulleyLimits.base, config::pulley.stepsPerUnit },
    { config::selectorLimits.base, config::selector.stepsPerUnit },
    { config::idlerLimits.base, config::idler.stepsPerUnit },
};

/// Convert a unit::Unit to AxisUnit.
/// The scaling factor is stored with the pair config::AxisConfig::uSteps and
/// config::AxisConfig::stepsPerUnit (one per-axis).
template <typename AU, typename U>
static constexpr AU unitToAxisUnit(U v) {
    static_assert(AU::unit == U::unit, "incorrect unit type conversion");
    static_assert(U::base == axisScale[AU::axis].base, "incorrect unit base conversion");
    return { (typename AU::type_t)(v.v * axisScale[AU::axis].stepsPerUnit) };
}

/// Convert an AxisUnit to a physical unit with a truncated integer type (normally int32_t).
/// Inverse of unitToAxisUnit, with the additional constraint that type casts are
/// performed earlier so that no floating point computation is required at runtime.
/// @tparam U Base unit type (normally U_mm)
/// @tparam AU AxisUnit type (implicit)
/// @tparam T Resulting integer type
/// @param v Value to truncate
/// @param mul Optional pre-multiplier
/// @returns Truncated unit (v * mul)
/// @see unitToAxisUnit
template <typename U, typename AU, typename T = int32_t>
static constexpr T axisUnitToTruncatedUnit(AU v, long double mul = 1.) {
    static_assert(AU::unit == U::unit, "incorrect unit type conversion");
    static_assert(U::base == axisScale[AU::axis].base, "incorrect unit base conversion");
    return { ((T)v.v / (T)(axisScale[AU::axis].stepsPerUnit / mul)) };
}

/// Truncate an Unit type to an integer (normally int32_t)
/// @param v Value to truncate
/// @param mul Optional pre-multiplier
/// @returns Truncated unit (v * mul)
template <typename U, typename T = int32_t>
static constexpr T truncatedUnit(U v, long double mul = 1.) {
    return (T)(v.v * mul);
}

/// Convert a unit::Unit to a steps type (pos_t or steps_t).
/// Extract the raw step count from an AxisUnit with type checking.
template <typename AU, typename U>
static constexpr typename AU::type_t unitToSteps(U v) {
    return unitToAxisUnit<AU>(v).v;
}

// Pulley
typedef AxisUnit<pos_t, Pulley, Lenght> P_pos_t; ///< Pulley position type (steps)
typedef AxisUnit<steps_t, Pulley, Speed> P_speed_t; ///< Pulley speed type (steps/s)
typedef AxisUnit<steps_t, Pulley, Accel> P_accel_t; ///< Pulley acceleration type (steps/s2)

static constexpr P_pos_t operator"" _P_mm(long double mm) {
    return { unitToAxisUnit<P_pos_t>(config::U_mm { mm }) };
}

static constexpr P_speed_t operator"" _P_mm_s(long double mm_s) {
    return { unitToAxisUnit<P_speed_t>(config::U_mm_s { mm_s }) };
}

static constexpr P_accel_t operator"" _P_mm_s2(long double mm_s2) {
    return { unitToAxisUnit<P_accel_t>(config::U_mm_s2 { mm_s2 }) };
}

// Selector
typedef AxisUnit<pos_t, Selector, Lenght> S_pos_t; ///< Selector position type (steps)
typedef AxisUnit<steps_t, Selector, Speed> S_speed_t; ///< Selector speed type (steps/s)
typedef AxisUnit<steps_t, Selector, Accel> S_accel_t; ///< Selector acceleration type (steps/s2)

static constexpr S_pos_t operator"" _S_mm(long double mm) {
    return { unitToAxisUnit<S_pos_t>(config::U_mm { mm }) };
}

static constexpr S_speed_t operator"" _S_mm_s(long double mm_s) {
    return { unitToAxisUnit<S_speed_t>(config::U_mm_s { mm_s }) };
}

static constexpr S_accel_t operator"" _S_mm_s2(long double mm_s2) {
    return { unitToAxisUnit<S_accel_t>(config::U_mm_s2 { mm_s2 }) };
}

// Idler
typedef AxisUnit<pos_t, Idler, Lenght> I_pos_t; ///< Idler position type (steps)
typedef AxisUnit<steps_t, Idler, Speed> I_speed_t; ///< Idler speed type (steps/s)
typedef AxisUnit<steps_t, Idler, Accel> I_accel_t; ///< Idler acceleration type (steps/s2)

static constexpr I_pos_t operator"" _I_deg(long double deg) {
    return { unitToAxisUnit<I_pos_t>(config::U_deg { deg }) };
}

static constexpr I_speed_t operator"" _I_deg_s(long double deg_s) {
    return { unitToAxisUnit<I_speed_t>(config::U_deg_s { deg_s }) };
}

static constexpr I_accel_t operator"" _I_deg_s2(long double deg_s2) {
    return { unitToAxisUnit<I_accel_t>(config::U_deg_s2 { deg_s2 }) };
}

} // namespace motion
} // namespace modules

// Inject literal operators into the global namespace for convenience
using modules::motion::operator"" _P_mm;
using modules::motion::operator"" _P_mm_s;
using modules::motion::operator"" _P_mm_s2;
using modules::motion::operator"" _S_mm;
using modules::motion::operator"" _S_mm_s;
using modules::motion::operator"" _S_mm_s2;
using modules::motion::operator"" _I_deg;
using modules::motion::operator"" _I_deg_s;
using modules::motion::operator"" _I_deg_s2;
