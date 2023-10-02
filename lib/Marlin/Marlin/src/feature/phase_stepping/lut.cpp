#include "lut.hpp"
#include "common.hpp"

#include <array>
#include <cmath>
#include <cstdint>

using namespace phase_stepping;

// Module definitions

static const auto sin_lut_values = [](){
    // Build 1/4 of sin period
    static constexpr int QUARTER = MOTOR_PERIOD / 4;
    std::array< uint8_t, QUARTER + 1 > values;
    for (size_t i = 0; i != values.size(); i++) {
        values[i] = 248 * sin(M_PI_2 * i / QUARTER) + 0.5;
    }

    return values;
}();

// Function definitions

int phase_stepping::sin_lut(int x) {
    x = normalize_phase(x);
    if (x <= MOTOR_PERIOD / 4)
        return sin_lut_values[x];
    if (x <= MOTOR_PERIOD / 2)
        return sin_lut_values[MOTOR_PERIOD / 2 - x];
    if (x <= 3 * MOTOR_PERIOD / 4)
        return -sin_lut_values[x - MOTOR_PERIOD / 2];
    return -sin_lut_values[MOTOR_PERIOD - x];
};

int phase_stepping::cos_lut(int x) {
    return sin_lut(x + MOTOR_PERIOD / 4);
}

int phase_stepping::normalize_phase(int phase) {
    phase = phase % MOTOR_PERIOD;
    if (phase < 0)
        phase += MOTOR_PERIOD;
    return phase;
}

void CorrectedCurrentLut::_update_phase_shift() {
    for (size_t i = 0; i != _phase_shift.size(); i++) {
        double item_phase = i * 2 * M_PI / MOTOR_PERIOD;
        double phase_shift = 0;
        for (size_t n = 0; n != _spectrum.size(); n++) {
            const SpectralItem& s = _spectrum[n];
            phase_shift += s.mag * std::sin(n * item_phase + s.pha);
        }
        _phase_shift[i] = MOTOR_PERIOD / (2 * M_PI) * phase_shift;
    }
}

std::pair< int, int > CorrectedCurrentLut::get_current(int idx) const {
    int pha = idx + _phase_shift[normalize_phase(idx)];
    return {
        sin_lut(pha), cos_lut(pha)
    };
}
