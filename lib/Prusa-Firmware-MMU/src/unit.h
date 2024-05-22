/// @file unit.h
#pragma once
#include <stdint.h>

/// Introduce a minimal Unit class that can be used for conformability, type checking and
/// conversion at compile time. Template parameters are abused to create unique types,
/// which then can go through (explicit) overload and conversion. Despite looking
/// daunting, usage is quite straightforward once the appropriate aliases and inline
/// operators are defined:
///
///     U_mm distance = 10.0_mm;
///     auto another = 20.5_mm;
///     auto sum = distance + another;
///
///     auto angle = 15.0_deg;
///     auto test = distance + angle; // compile time error
///
/// Template parameters are only used for type checking. Unit contains a single value
/// Unit<T>::v and is thus well suited for parameter passing and inline initialization.
///
/// Conversion to physical steps is done in modules::motion through the sister class
/// modules::motion::AxisUnit, modules::motion::unitToAxisUnit and
/// modules::motion::unitToSteps, which also ensures quantities from different axes are
/// not mixed together. AxisUnit are the normal type that *should* be used at runtime.
namespace unit {

/// Base units for conformability testing
enum UnitBase : uint8_t {
    Millimeter,
    Degree,
};

/// Unit types for conformability testing
enum UnitType : uint8_t {
    Lenght,
    Speed,
    Accel,
};

/// Generic unit type for compile-time conformability testing
template <typename T, UnitBase B, UnitType U>
struct Unit {
    T v;

    static constexpr UnitBase base = B;
    static constexpr UnitType unit = U;

    typedef T type_t;
    typedef Unit<T, B, U> self_t;

    // same-type operations
    constexpr self_t operator+(const self_t r) const { return { v + r.v }; }
    constexpr self_t operator-(const self_t r) const { return { v - r.v }; }
    constexpr self_t operator-() const { return { -v }; }
    constexpr self_t operator*(const self_t r) const { return { v * r.v }; }
    constexpr self_t operator/(const self_t r) const { return { v / r.v }; }

    // allow an unitless multiplier to scale the quantity: U * f => U
    constexpr self_t operator*(const long double f) const { return { (T)(v * f) }; }
    constexpr self_t operator/(const long double f) const { return { (T)(v / f) }; }
};

// complementary f * U => U * f
template <typename T, UnitBase B, UnitType U>
constexpr Unit<T, B, U> operator*(const long double f, const Unit<T, B, U> u) { return u * f; }

// Millimiters
typedef Unit<long double, Millimeter, Lenght> U_mm;
typedef Unit<long double, Millimeter, Speed> U_mm_s;
typedef Unit<long double, Millimeter, Accel> U_mm_s2;

static constexpr U_mm operator"" _mm(long double mm) {
    return { mm };
}

static constexpr U_mm_s operator"" _mm_s(long double mm_s) {
    return { mm_s };
}

static constexpr U_mm_s2 operator"" _mm_s2(long double mm_s2) {
    return { mm_s2 };
}

// Degrees
typedef Unit<long double, Degree, Lenght> U_deg;
typedef Unit<long double, Degree, Speed> U_deg_s;
typedef Unit<long double, Degree, Accel> U_deg_s2;

static constexpr U_deg operator"" _deg(long double deg) {
    return { deg };
}

static constexpr U_deg_s operator"" _deg_s(long double deg_s) {
    return { deg_s };
}

static constexpr U_deg_s2 operator"" _deg_s2(long double deg_s2) {
    return { deg_s2 };
}

} // namespace unit

// Inject literal operators into the global namespace for convenience
using unit::operator"" _mm;
using unit::operator"" _mm_s;
using unit::operator"" _mm_s2;
using unit::operator"" _deg;
using unit::operator"" _deg_s;
using unit::operator"" _deg_s2;
