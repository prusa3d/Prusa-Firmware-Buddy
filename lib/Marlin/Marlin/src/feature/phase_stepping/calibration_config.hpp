#pragma once

#include <Configuration.h>

namespace phase_stepping {

struct CalibrationPhase {
    int harmonic;
    float speed;

    std::optional<float> pha = {}; // Optionally force starting point
    float pha_window = 0.1f;

    std::optional<float> mag = {}; // Optionally force starting point
    float mag_window = 0.01f;

    int iteration_count = 18;
};

struct PrinterCalibrationConfig {
    float calib_revs;
    std::vector<CalibrationPhase> phases;
};

inline PrinterCalibrationConfig get_printer_calibration_config() {
#if PRINTER_IS_PRUSA_XL
    return {
        .calib_revs = 0.5f,
        .phases = { { { .harmonic = 2,
                          .speed = 3.f,
                          .pha = 3.14f,
                          .pha_window = 2.5f,
                          .mag = 0.038f,
                          .mag_window = 0.25f,
                          .iteration_count = 14 },
            { .harmonic = 4,
                .speed = 1.5f,
                .pha = 0.f,
                .pha_window = 4.f,
                .mag = 0.02f,
                .mag_window = 0.01f,
                .iteration_count = 14 },
            { .harmonic = 2,
                .speed = 3.f,
                .pha_window = 1.f,
                .mag_window = 0.01f },
            { .harmonic = 4,
                .speed = 1.5f,
                .pha_window = 0.8f,
                .mag_window = 0.01f } } }
    };
#endif

    bsod("Unsupported printer");
}

}; // namespace phase_stepping
