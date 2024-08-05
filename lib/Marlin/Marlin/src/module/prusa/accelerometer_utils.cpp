#include "accelerometer_utils.h"

#include <option/has_remote_accelerometer.h>

static_assert(HAS_REMOTE_ACCELEROMETER());

/**
 * Unpack 10bit samples into 16bit sample, scale to ms^-2 and swap X and Z axis to compensate for Dwarf orientation
 */
PrusaAccelerometer::Acceleration AccelerometerUtils::unpack_sample(AccelerometerUtils::SampleStatus &sampleStatus, common::puppies::fifo::AccelerometerXyzSample sample) {
    constexpr float standard_gravity = 9.80665f;
    constexpr int16_t max_value = 0b0111'1111'1111'1111;
    constexpr float factor2g = 2.f * standard_gravity / max_value;
    constexpr int16_t top_10_bits = 0b1111'1111'1100'0000u;

    PrusaAccelerometer::Acceleration accelerometer_sample;
    int16_t acceleration = (sample << x_right_shift) & top_10_bits;
    accelerometer_sample.val[2] = static_cast<float>(acceleration) * factor2g;
    acceleration = (sample >> y_left_shift) & top_10_bits;
    accelerometer_sample.val[1] = static_cast<float>(acceleration) * factor2g;
    acceleration = (sample >> z_left_shift) & top_10_bits;
    accelerometer_sample.val[0] = static_cast<float>(acceleration) * factor2g;
    sampleStatus.buffer_overflow = sample & buffer_overflow_mask;
    sampleStatus.sample_overrun = sample & sample_overrun_mask;
    return accelerometer_sample;
}
