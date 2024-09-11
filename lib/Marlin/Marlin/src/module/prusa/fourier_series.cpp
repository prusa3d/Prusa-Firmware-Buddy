/**
 * @file
 * @author Marek Bel
 */

#include "fourier_series.h"
#include "metric.h"
#ifdef FOURIER_SERIES_OUTPUT_SAMPLES
    #include "../../../../../tinyusb/src/class/cdc/cdc_device.h"
#endif
#include <tuple>

METRIC_DEF(accel, "tk_accel", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);

FourierSeries3d::FourierSeries3d(float frequency)
    : m_sum()
    , m_freq_2pi(std::numbers::pi_v<float> * frequency * 2.f)
    , m_samples_num() {
}

/**
 * @brief Multiply sample by sine and cosine and add it to sum
 *
 * Result of computation is the same if sine period is added or subtracted
 * from time multiple times.
 * Caller should keep sample_time small number close to zero.
 *
 * @param sample_time of the sample in seconds, it must be small number to preserve sine accuracy
 * @param sample acceleration measured in three axes
 * @return total number of samples added during object lifetime
 */
uint32_t FourierSeries3d::add_sample(const float sample_time, const PrusaAccelerometer::Acceleration &sample) {

#if 0 // error: 'sample' is not a constant expression
    static_assert(std::tuple_size_v<decltype(m_sum)> == std::size(sample.val), "Dimension doesn't match.");
#endif

    metric_record_custom(&accel, " x=%.4f,y=%.4f,z=%.4f", (double)sample.val[0], (double)sample.val[1], (double)sample.val[2]);
    const float accelerometer_time_2pi_measurement_freq = m_freq_2pi * sample_time;
    const std::complex<float> amplitude = { sinf(accelerometer_time_2pi_measurement_freq), cosf(accelerometer_time_2pi_measurement_freq) };

    for (size_t axis = 0; axis < std::size(m_sum); ++axis) {
        m_sum[axis] += amplitude * sample.val[axis];
    }

#ifdef FOURIER_SERIES_OUTPUT_SAMPLES
    char buff[40];
    snprintf(buff, 40, "%f %f %f\n", static_cast<double>(sample.val[1]), static_cast<double>(amplitude.real()), static_cast<double>(amplitude.imag()));
    tud_cdc_n_write_str(0, buff);
    tud_cdc_write_flush();
#endif

    return ++m_samples_num;
}

xyz_float_t FourierSeries3d::get_magnitude() {

    xyz_double_t retval = { std::abs(m_sum[0]), std::abs(m_sum[1]), std::abs(m_sum[2]) };
    retval *= 2.;
    retval = retval / m_samples_num;
    return retval.asFloat();
}
