#pragma once

#include "common.hpp"

#include <bitset>
#include <cstdint>

// This module is responsible for implementing any lookup tables used in phase
// stepping.

namespace phase_stepping {

/**
 * Lookup table-based implementation of sin/cos that uses MOTOR_PERIOD as its
 * period. Returns integral value scaled to given range.
 */
int sin_lut(int x, int range = 1 << opts::SIN_LUT_FRACTIONAL);
int cos_lut(int x, int range = 1 << opts::SIN_LUT_FRACTIONAL);

/**
 * Given a phase, normalize it into range <0, MOTOR_PERIOD)
 **/
int normalize_motor_phase(int phase);

/**
 * Given a phase, normalize it into range <0, SIN_PERIOD)
 **/
int normalize_sin_phase(int phase);

struct CoilCurrents {
    int a;
    int b;

    bool operator==(const CoilCurrents &) const = default;
};

class CorrectedCurrentLut {

public:
    void clear();

public:
    MotorPhaseCorrection _spectrum = {};
    std::array<int8_t, opts::MOTOR_PERIOD> _phase_shift = {};

    void _update_phase_shift();
    static int _phase_shift_for_harmonic(int idx, int harmonic,
        int phase, int mag);

public:
    CorrectedCurrentLut() = default;

    const auto &get_correction() const {
        return _spectrum;
    };

    template <typename F>
        requires requires(F f) { f(_spectrum); }
    void modify_correction(F f) {
        f(_spectrum);
        _update_phase_shift();
    }

    CoilCurrents get_current(int idx) const;
    int get_phase_shift(int idx) const;

    CoilCurrents get_current_for_calibration(int idx, int extra_harmonic, int extra_phase, int extra_mag) const;
    int get_phase_shift_for_calibration(int idx, int extra_harmonic, int extra_phase, int extra_mag) const;
};

/**
 * Alternative LUT with simplified implementation for reference/testing purposes
 **/
class CorrectedCurrentLutSimple {
public:
    struct CurrentTrace {
        std::array<uint8_t, opts::MOTOR_PERIOD> _val;
        std::bitset<opts::MOTOR_PERIOD> _sign;

        int get(int idx) const {
            idx = normalize_motor_phase(idx);
            return _val[idx] * (_sign[idx] ? 1 : -1);
        }

        void set(int idx, int val) {
            idx = normalize_motor_phase(idx);
            _sign[idx] = val > 0;
            _val[idx] = abs(val);
        }
    };

    MotorPhaseCorrection _spectrum = {};
    CurrentTrace _sin, _cos;

    void _update_phase_shift();

public:
    CorrectedCurrentLutSimple() {
        _update_phase_shift();
    }

    const auto &get_correction() const {
        return _spectrum;
    };

    template <typename F>
        requires requires(F f) { f(_spectrum); }
    void modify_correction(F f) {
        f(_spectrum);
        _update_phase_shift();
    }

    CoilCurrents get_current(int idx) const;
};

} // namespace phase_stepping
