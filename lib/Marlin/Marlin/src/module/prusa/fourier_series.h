/**
 * @file
 * @author Marek Bel
 */

#pragma once
#include "accelerometer.h"
#include "Marlin/src/core/types.h"
#include <complex>
#include <array>
#include <stdint.h>

/**
 * Debug feature. If defined it outputs on USB_CDC
 *  * samples being added
 *  * trigonometric function being correlated with
 */
// #define FOURIER_SERIES_OUTPUT_SAMPLES

/**
 * @brief Compute single point Fourier transformation
 */
class FourierSeries3d {
public:
    FourierSeries3d(float frequency);
    uint32_t add_sample(const float sample_time, const PrusaAccelerometer::Acceleration &sample);
    xyz_float_t get_magnitude();
    uint32_t get_samples_num() { return m_samples_num; }

private:
    std::array<std::complex<double>, 3> m_sum;
    const float m_freq_2pi;
    uint32_t m_samples_num;
};
