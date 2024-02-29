#pragma once

#include <printers.h>

#include <optional>
#include <vector>
#include <array>

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

PrinterCalibrationConfig get_printer_calibration_config();

}; // namespace phase_stepping
