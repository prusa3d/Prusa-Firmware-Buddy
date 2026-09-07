#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <bsod/bsod.h>

/// Q48.16 fixed-point time type for the motion pipeline.
///
/// Representation: int64_t raw, 1 unit = 2^-16 µs.  Range ±2^47 µs ≈ ±4.5 years.
/// Signed so IS pulse offsets and PA interpolation intermediates may be negative.
///
/// Rounding rules (recorded here because they affect observable step timing):
///   1. from_seconds(double) rounds to nearest LSB via llround().  Half-LSB ≈ 7.6 fs
///      (physically meaningless); rounding is the numerically honest choice.
///   2. to_us_floor() uses arithmetic right-shift (floors toward −∞).
///      from_seconds(sec).to_us_floor() differs from direct truncation int64_t(sec * 1e6)
///      by at most 1 µs, and only when sec·1e6 lies within 2⁻¹⁷ µs below an integer.
///   3. from_seconds(float) truncates toward zero (plain cast): cheapest codegen;
///      the float 2^-24 relative error dwarfs the LSB anyway.
///
/// max() and min() are comparison-only sentinels (INT64_MAX / INT64_MIN).
/// Never do arithmetic on them; debug asserts catch violations.
///
/// from_raw() is for tests and named-constant definitions only.
/// Production code constructs values via from_us() / from_seconds().
///
/// Why debug asserts instead of wrap-around: the ±4.5-year range is unreachable
/// by design — the timeline resets to zero on motion halt, and continuous sessions
/// are hard-capped at MAX_PRINT_TIME (≈116 days).  Wrap-around would destroy the
/// total ordering that the nearest-event min-search and max()/min() sentinels rely on.
class TimeTicks {
public:
    static constexpr int FRACTIONAL_BITS = 16;
    /// Raw units per microsecond (2^16).
    static constexpr int64_t PER_US = int64_t(1) << FRACTIONAL_BITS;
    /// Raw units per second (PER_US * 1e6).
    static constexpr double PER_SEC = double(PER_US) * 1e6;

    /// Zero-initialised (default).
    constexpr TimeTicks() = default;

    // --- Construction ---

    /// For tests and named-constant definitions only; not for general production use.
    static constexpr TimeTicks from_raw(int64_t raw) {
        return TimeTicks(raw);
    }

    /// Exact conversion from whole microseconds.  Debug-asserts the value fits in the integer part.
    static constexpr TimeTicks from_us(int64_t us) {
        // Integer part is 64 - FRACTIONAL_BITS - 1 (sign) bits wide.
        [[maybe_unused]] static constexpr int US_BITS = 64 - FRACTIONAL_BITS - 1;
        debug_assert(us > -(int64_t(1) << US_BITS) && us < (int64_t(1) << US_BITS));
        return from_raw(us << FRACTIONAL_BITS);
    }

    /// Setup/boundary conversion from double seconds.  Rounds to nearest LSB.
    /// Runtime-only (llround constexpr support is inconsistent across stdlibs).
    /// Debug-asserts the input is within the type's representable range (≈±4.5 years);
    /// beyond it llround is implementation-defined.
    static TimeTicks from_seconds(double sec) {
        // Overflow guard: sec * PER_SEC must fit in int64_t.
        debug_assert(std::abs(sec) < double(std::numeric_limits<int64_t>::max()) / PER_SEC);
        return from_raw(static_cast<int64_t>(llround(sec * PER_SEC)));
    }

    /// Hot-path conversion from float seconds.  Truncates toward zero.
    /// Debug-asserts std::isfinite(sec) — int64_t(inf/NaN) is UB; callers must
    /// guard non-finite inputs before calling from_seconds(float).
    static TimeTicks from_seconds(float sec) {
        debug_assert(std::isfinite(sec));
        // Overflow guard: sec * PER_SEC must fit in int64_t.
        debug_assert(std::abs(sec) < static_cast<float>(double(std::numeric_limits<int64_t>::max()) / PER_SEC));
        return from_raw(static_cast<int64_t>(sec * static_cast<float>(PER_SEC)));
    }

    static constexpr TimeTicks zero() { return from_raw(0); }

    /// Comparison-only sentinel — orders above all finite values.  Never apply arithmetic.
    static constexpr TimeTicks max() { return from_raw(std::numeric_limits<int64_t>::max()); }

    /// Comparison-only sentinel — orders below all finite values.  Never apply arithmetic.
    static constexpr TimeTicks min() { return from_raw(std::numeric_limits<int64_t>::min()); }

    // --- Observers ---

    constexpr int64_t raw() const { return raw_; }

    /// Floors toward −∞ (arithmetic shift).  See header comment rule 2 for the
    /// ±1 µs relation to direct truncation.
    constexpr int64_t to_us_floor() const { return raw_ >> FRACTIONAL_BITS; }

    /// Converts to seconds as double.  For debug/boundary use only, not the hot path.
    constexpr double to_seconds() const { return static_cast<double>(raw_) / PER_SEC; }

    /// Converts to seconds as float.  For small *relative* values (IS/PA local kinematics).
    float to_seconds_float() const { return static_cast<float>(raw_) / static_cast<float>(PER_SEC); }

    // --- Arithmetic ---
    // All operations debug-assert against overflow via __builtin_*_overflow (no-op under NDEBUG).

    constexpr TimeTicks operator+(TimeTicks other) const {
        int64_t result;
#ifndef NDEBUG
        debug_assert(!__builtin_add_overflow(raw_, other.raw_, &result));
#else
        result = raw_ + other.raw_;
#endif
        return from_raw(result);
    }

    constexpr TimeTicks operator-(TimeTicks other) const {
        int64_t result;
#ifndef NDEBUG
        debug_assert(!__builtin_sub_overflow(raw_, other.raw_, &result));
#else
        result = raw_ - other.raw_;
#endif
        return from_raw(result);
    }

    constexpr TimeTicks operator-() const {
        // Negation overflows only for INT64_MIN (which is min() sentinel — never negate it).
        debug_assert(raw_ != std::numeric_limits<int64_t>::min());
        return from_raw(-raw_);
    }

    /// Multiplies by an integer scalar.
    constexpr TimeTicks operator*(int64_t factor) const {
        int64_t result;
#ifndef NDEBUG
        debug_assert(!__builtin_mul_overflow(raw_, factor, &result));
#else
        result = raw_ * factor;
#endif
        return from_raw(result);
    }

    constexpr TimeTicks &operator+=(TimeTicks other) {
        *this = *this + other;
        return *this;
    }

    constexpr TimeTicks &operator-=(TimeTicks other) {
        *this = *this - other;
        return *this;
    }

    /// Scales by a float ratio.  Uses float precision — caller must ensure |raw| < 2^32
    /// so that float(raw) is exact enough (debug-asserted).  Intended for fractional
    /// interpolation within a short interval (e.g. one pressure advance sampling period).
    TimeTicks scaled_by(float ratio) const {
        debug_assert(raw_ > -(int64_t(1) << 32) && raw_ < (int64_t(1) << 32));
        const float result_f = static_cast<float>(raw_) * ratio;
        // Overflow guard: result must fit in int64_t.
        debug_assert(std::abs(result_f) < static_cast<float>(double(std::numeric_limits<int64_t>::max())));
        return from_raw(static_cast<int64_t>(result_f));
    }

    /// Comparison (total order, including sentinels).
    constexpr auto operator<=>(const TimeTicks &) const = default;

private:
    constexpr explicit TimeTicks(int64_t raw)
        : raw_(raw) {}

    int64_t raw_ = 0;
};

/// Commuted scalar multiply: n * ticks.
constexpr TimeTicks operator*(int64_t factor, TimeTicks t) {
    return t * factor;
}
