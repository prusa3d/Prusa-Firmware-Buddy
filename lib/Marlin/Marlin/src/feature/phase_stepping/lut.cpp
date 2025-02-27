#include "lut.hpp"
#include "common.hpp"

#include <hwio_pindef.h>
#include <buddy/phase_stepping_opts.h>

#include <array>
#include <numbers>
#include <cstdint>

using namespace phase_stepping;
using namespace phase_stepping::opts;

// Module definitions

static const auto sin_lut_values = []() consteval {
    // Build 1/4 of sin period mapping sin values to 0-FIXED_ONE range. This
    // range is selected such that:
    // - we have 2 bytes per value
    // - any scaling can be done by multiplying followed by a SIN_LUT_FRACTIONAL
    //   right shift

    static_assert(SIN_LUT_FRACTIONAL < 16, "SIN_LUT_FRACTIONAL must be less than 16 to fit in uint16_t");
    static constexpr int QUARTER = SIN_PERIOD / 4;
    static constexpr int FIXED_ONE = 1 << SIN_LUT_FRACTIONAL;

    std::array<uint16_t, QUARTER + 1> values;
    for (size_t i = 0; i != values.size(); i++) {
        values[i] = std::lround(FIXED_ONE * sin(std::numbers::pi_v<float> / 2.f * i / QUARTER));
    }

    return values;
}();

// Function definitions

int phase_stepping::sin_lut(int x, int range) {
    x = normalize_sin_phase(x);

    auto rescale = [&](uint32_t x) -> int {
        return (x * range) >> SIN_LUT_FRACTIONAL;
    };

    if (x <= SIN_PERIOD / 4) {
        return rescale(sin_lut_values[x]);
    }
    if (x <= SIN_PERIOD / 2) {
        return rescale(sin_lut_values[SIN_PERIOD / 2 - x]);
    }
    if (x <= 3 * SIN_PERIOD / 4) {
        return -rescale(sin_lut_values[x - SIN_PERIOD / 2]);
    }
    return -rescale(sin_lut_values[SIN_PERIOD - x]);
};

int phase_stepping::cos_lut(int x, int range) {
    return sin_lut(x + SIN_PERIOD / 4, range);
}

int phase_stepping::normalize_sin_phase(int phase) {
    phase = phase % SIN_PERIOD;
    if (phase < 0) {
        phase += SIN_PERIOD;
    }
    return phase;
}

int phase_stepping::normalize_motor_phase(int phase) {
    phase = phase % MOTOR_PERIOD;
    if (phase < 0) {
        phase += MOTOR_PERIOD;
    }
    return phase;
}

void CorrectedCurrentLut::clear() {
    _spectrum.fill({});
    _phase_shift.fill({});
}

void CorrectedCurrentLut::_update_phase_shift() {
    for (size_t i = 0; i != _phase_shift.size(); i++) {
        float item_phase = i * 2 * std::numbers::pi_v<float> / MOTOR_PERIOD;
        float phase_shift = 0;
        for (size_t n = 0; n != _spectrum.size(); n++) {
            const SpectralItem &s = _spectrum[n];
            phase_shift += s.mag * std::sin(n * item_phase + s.pha);
        }
        _phase_shift[i] = std::round(SIN_PERIOD / (2 * std::numbers::pi_v<float>)*phase_shift);
    }
}

CoilCurrents CorrectedCurrentLut::get_current(int idx) const {
    int pha = SIN_FRACTION * idx + _phase_shift[normalize_motor_phase(idx)];
    return {
        sin_lut(pha, CURRENT_AMPLITUDE), cos_lut(pha, CURRENT_AMPLITUDE)
    };
}

int CorrectedCurrentLut::get_phase_shift(int idx) const {
    return _phase_shift[normalize_motor_phase(idx)] / SIN_FRACTION;
}

void CorrectedCurrentLutSimple::_update_phase_shift() {
    for (size_t i = 0; i != MOTOR_PERIOD; i++) {
        float item_phase = i * 2 * std::numbers::pi_v<float> / MOTOR_PERIOD;
        float phase_shift = 0;
        for (size_t n = 0; n != _spectrum.size(); n++) {
            const SpectralItem &s = _spectrum[n];
            phase_shift += s.mag * std::sin(n * item_phase + s.pha);
        }
        _sin.set(i, std::round(CURRENT_AMPLITUDE * std::sin(item_phase + phase_shift)));
        _cos.set(i, std::round(CURRENT_AMPLITUDE * std::cos(item_phase + phase_shift)));
    }
}

CoilCurrents CorrectedCurrentLutSimple::get_current(int idx) const {
    return { _sin.get(idx), _cos.get(idx) };
}
