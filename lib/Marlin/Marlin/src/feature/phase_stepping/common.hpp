#pragma once

#include <array>
#include <buddy/phase_stepping_opts.h>

namespace phase_stepping {

struct SpectralItem {
    float mag = 0, pha = 0;
};

// WARNING: changing this type has consequences in regards to data persistency!
using MotorPhaseCorrection = std::array<SpectralItem, opts::CORRECTION_HARMONICS + 1>;
} // namespace phase_stepping
