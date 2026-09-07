#pragma once

#include <cmath>
#include <cstddef>
#include <arm_math.h>
#include <dsp/fast_math_functions.h>
#include <bsod/bsod.h>

namespace sp {

// Wrappers around CMSIS-DSP math functions to provide overloads for float and
// double so they work seamlessly in templated code. Use this functions if you
// want fast math but you can accept the reduced precision of CMSIS-DSP's fast
// math functions. Not all functions have fast math versions, so for those we
// just use std:: versions. using math from sp:: implies "use the fastest
// available math but be mindful of precision tradeoffs".

inline float sin(float x) { return arm_sin_f32(x); }
inline double sin(double x) { return std::sin(x); }

inline float cos(float x) { return arm_cos_f32(x); }
inline double cos(double x) { return std::cos(x); }

inline float sqrt(float x) {
    debug_assert(x >= 0.0f);
    float out = 0.0f;
    arm_sqrt_f32(x, &out);
    return out;
}

inline double sqrt(double x) { return std::sqrt(x); }

inline float atan2(float y, float x) {
    float out = 0.0f;
    arm_atan2_f32(y, x, &out);
    return out;
}
inline double atan2(double y, double x) { return std::atan2(y, x); }

// CMSIS-DSP doesn't provide arm_tan_f32. Use std::tan here asi division of
// sp::sin and sp::cos would be too inaccurate.
inline float tan(float x) { return std::tan(x); }
inline double tan(double x) { return std::tan(x); }

struct Complex;

// Magnitude and phase representation of an (unwrapped) complex value or polar
// coordinates.
struct MagPhase {
    float magnitude;
    float phase;

    Complex to_complex() const;
};

// Complex value type matching CMSIS-DSP memory layout: { re, im }
// This is a POD, standard-layout aggregate so it is layout-compatible with
// interleaved float arrays used by CMSIS-DSP (Re, Im, Re, Im, ...).
struct Complex {
    using value_type = float;
    float re;
    float im;

    // Aggregate type: no user-provided constructors to keep triviality

    // Unary
    Complex operator-() const { return Complex { -re, -im }; }

    // Complex-Complex arithmetic
    Complex operator+(const Complex &o) const { return Complex { re + o.re, im + o.im }; }
    Complex operator-(const Complex &o) const { return Complex { re - o.re, im - o.im }; }
    Complex operator*(const Complex &o) const { return Complex { re * o.re - im * o.im, re * o.im + im * o.re }; }
    Complex operator/(const Complex &o) const {
        const float denom = o.re * o.re + o.im * o.im;
        return Complex { (re * o.re + im * o.im) / denom,
            (im * o.re - re * o.im) / denom };
    }

    Complex &operator+=(const Complex &o) {
        re += o.re;
        im += o.im;
        return *this;
    }
    Complex &operator-=(const Complex &o) {
        re -= o.re;
        im -= o.im;
        return *this;
    }
    Complex &operator*=(const Complex &o) { return *this = *this * o; }
    Complex &operator/=(const Complex &o) { return *this = *this / o; }

    // Scalar arithmetic
    Complex operator*(float s) const { return Complex { re * s, im * s }; }
    Complex operator/(float s) const { return Complex { re / s, im / s }; }
    Complex &operator*=(float s) {
        re *= s;
        im *= s;
        return *this;
    }
    Complex &operator/=(float s) {
        re /= s;
        im /= s;
        return *this;
    }
    friend Complex operator*(float s, const Complex &c) { return Complex { s * c.re, s * c.im }; }

    // Properties
    Complex conj() const { return Complex { re, -im }; }
    float magnitude() const { return sqrt(re * re + im * im); }
    float magnitude_squared() const { return re * re + im * im; }
    float angle() const { return sp::atan2(im, re); }

    // Conversions
    static Complex from_polar(float mag, float phase) {
        return Complex { mag * cos(phase), mag * sin(phase) };
    }

    static Complex from_polar(const MagPhase &mp) {
        return from_polar(mp.magnitude, mp.phase);
    }

    MagPhase to_mag_phase() const {
        return MagPhase { magnitude(), angle() };
    }
};

inline Complex MagPhase::to_complex() const {
    return Complex::from_polar(*this);
}

static_assert(std::is_trivial_v<Complex> && std::is_standard_layout_v<Complex>, "Complex must be trivial and standard-layout");
static_assert(sizeof(Complex) == 2 * sizeof(float), "Complex must be tightly packed");
static_assert(offsetof(Complex, re) == 0, "Complex.re must be at offset 0");
static_assert(offsetof(Complex, im) == sizeof(float), "Complex.im must follow re");

static_assert(std::is_trivial_v<MagPhase> && std::is_standard_layout_v<MagPhase>, "MagPhase must be trivial and standard-layout");
static_assert(sizeof(MagPhase) == 2 * sizeof(float), "MagPhase must be tightly packed");
static_assert(sizeof(MagPhase) == sizeof(Complex), "MagPhase and Complex must have identical size");
static_assert(alignof(MagPhase) == alignof(Complex), "MagPhase and Complex must have identical alignment");

} // namespace sp
