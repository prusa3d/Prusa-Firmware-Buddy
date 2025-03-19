#pragma once

#include <array>
#include <numbers>
#include <cmath>
#include <buddy/phase_stepping_opts.h>

namespace phase_stepping {

template <typename T>
struct SpectralItemG {
    T mag = 0, pha = 0;
};

using SpectralItem = SpectralItemG<float>;

inline int mag_to_fixed(float mag) {
    return std::lround(mag * opts::SIN_PERIOD * (1 << opts::MAG_FRACTIONAL) / (2 * std::numbers::pi_v<float>));
}

inline float mag_to_float(int mag) {
    return 2 * std::numbers::pi_v<float> * float(mag) / (opts::SIN_PERIOD * (1 << opts::MAG_FRACTIONAL));
}

inline int pha_to_fixed(float pha) {
    return std::lround(pha * opts::SIN_PERIOD / (2 * std::numbers::pi_v<float>));
}

inline float pha_to_float(int pha) {
    return (pha * 2 * std::numbers::pi_v<float>) / opts::SIN_PERIOD;
}

// WARNING: changing this type has consequences in regards to data persistency!
using MotorPhaseCorrection = std::array<SpectralItem, opts::CORRECTION_HARMONICS + 1>;
} // namespace phase_stepping
