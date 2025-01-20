#pragma once

#include <printers.h>

#include <optional>
#include <array>

namespace phase_stepping {

struct CalibrationPhase {
    int harmonic;
    float speed;
    float revs = 0.5f;

    std::optional<float> pha = {}; // Optionally force starting point
    float pha_window = 0.1f;

    std::optional<float> mag = {}; // Optionally force starting point
    float mag_window = 0.01f;

    int iteration_count = 18;
};

template <size_t N>
struct PrinterCalibrationConfig {
    std::array<CalibrationPhase, N> phases;
};

#if PRINTER_IS_PRUSA_XL()
static inline constexpr const PrinterCalibrationConfig<4> printer_calibration_config {
    .phases = {
        CalibrationPhase {
            .harmonic = 2,
            .speed = 3.f,
            .pha = 3.14f,
            .pha_window = 4.f,
            .mag = 0.02f,
            .mag_window = 0.05f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 1.5f,
            .pha = 0.f,
            .pha_window = 4.f,
            .mag = 0.015f,
            .mag_window = 0.04f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 3.f,
            .pha_window = 1.f,
            .mag_window = 0.02f,
            .iteration_count = 16,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 1.5f,
            .pha_window = 1.5f,
            .mag_window = 0.02f,
            .iteration_count = 16,
        },
    },
};
#elif PRINTER_IS_PRUSA_iX() // TODO for now it is just copy-paste of XL values; needs changes when iX specific values are measured
static inline constexpr const PrinterCalibrationConfig<4> printer_calibration_config {
    .phases = {
        CalibrationPhase {
            .harmonic = 2,
            .speed = 3.f,
            .pha = 3.14f,
            .pha_window = 4.f,
            .mag = 0.02f,
            .mag_window = 0.05f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 1.5f,
            .pha = 0.f,
            .pha_window = 4.f,
            .mag = 0.015f,
            .mag_window = 0.04f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 3.f,
            .pha_window = 1.f,
            .mag_window = 0.02f,
            .iteration_count = 16,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 1.5f,
            .pha_window = 1.5f,
            .mag_window = 0.02f,
            .iteration_count = 16,
        },
    },
};
#elif PRINTER_IS_PRUSA_MK4()
static inline constexpr const PrinterCalibrationConfig<6> printer_calibration_config {
    .phases = {
        CalibrationPhase {
            .harmonic = 1,
            .speed = 2.f,
            .revs = 2.f,
            .pha = 3.14f,
            .pha_window = 4.f,
            .mag = 0.016f,
            .mag_window = 0.01f,
            .iteration_count = 8,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 1.f,
            .revs = 1.f,
            .pha = 3.14f,
            .pha_window = 3.f,
            .mag = 0.015f,
            .mag_window = 0.01f,
            .iteration_count = 8,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 0.5f,
            .revs = 0.5f,
            .pha = 0.f,
            .pha_window = 3.f,
            .mag = 0.015f,
            .mag_window = 0.01f,
            .iteration_count = 8,
        },
        CalibrationPhase {
            .harmonic = 1,
            .speed = 2.f,
            .revs = 2.f,
            .pha_window = 0.5f,
            .mag_window = 0.005f,
            .iteration_count = 12,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 1.f,
            .revs = 1.f,
            .pha_window = .5f,
            .mag_window = 0.005f,
            .iteration_count = 12,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 0.5f,
            .revs = 0.5f,
            .pha_window = 0.5f,
            .mag_window = 0.005f,
            .iteration_count = 12,
        },
    },
};
#elif PRINTER_IS_PRUSA_COREONE()
static inline constexpr const PrinterCalibrationConfig<6> printer_calibration_config {
    .phases = {
        CalibrationPhase {
            .harmonic = 1,
            .speed = 2.f,
            .pha = 0.f,
            .pha_window = 4.f,
            .mag = 0.02f,
            .mag_window = 0.02f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 1.f,
            .pha = 0.f,
            .pha_window = 3.f,
            .mag = 0.01f,
            .mag_window = 0.04f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 0.5f,
            .pha = 0.f,
            .pha_window = 6.f,
            .mag = 0.0025f,
            .mag_window = 0.0025f,
            .iteration_count = 10,
        },
        CalibrationPhase {
            .harmonic = 1,
            .speed = 2.f,
            .pha_window = 1.f,
            .mag_window = 0.01f,
            .iteration_count = 16,
        },
        CalibrationPhase {
            .harmonic = 2,
            .speed = 1.f,
            .pha_window = .5f,
            .mag_window = 0.01f,
            .iteration_count = 16,
        },
        CalibrationPhase {
            .harmonic = 4,
            .speed = 0.5f,
            .pha_window = 0.5f,
            .mag_window = 0.001f,
            .iteration_count = 16,
        },
    },
};
#else
    #error "Unsupported printer"
#endif

consteval inline bool is_valid(auto config) {
    const size_t size = config.phases.size();
    const size_t halfsize = size / 2;
    if (2 * halfsize != size) {
        return false && "require odd number of phases";
    }
    for (size_t i = 0; i < halfsize; ++i) {
        if (config.phases[i].harmonic != config.phases[i + halfsize].harmonic) {
            return false && "harmonic mismatch";
        }
    }
    return true;
}
static_assert(is_valid(printer_calibration_config));

}; // namespace phase_stepping
