/**
 * @file
 */

#pragma once

#include "accelerometer.h"
#include "puppies/fifo_coder.hpp"
#include <stdint.h>
#include <option/has_remote_accelerometer.h>

class AccelerometerUtils {
private:
    /// X raw acceleration will occupy bits 0 .. 9 (LSB first)
    static constexpr unsigned x_right_shift = 6;
    /// Y raw acceleration will occupy bits 10 .. 19 (LSB first)
    static constexpr unsigned y_left_shift = 4;
    /// Z raw acceleration will occupy bits 20 .. 29 (LSB first)
    static constexpr unsigned z_left_shift = 14;
    /// Buffer overflow bit will occupy bit 30
    static constexpr uint_least32_t buffer_overflow_mask = 1 << 30;
    /// Sample overrun bit will occupy bit 31
    static constexpr uint_least32_t sample_overrun_mask = 1 << 31;

public:
    /**
     * @brief Pack top 10 bits of each XYZ axis raw acceleration
     */
    static common::puppies::fifo::AccelerometerXyzSample pack_record(dwarf::accelerometer::AccelerometerRecord record) {
        static_assert(sizeof(common::puppies::fifo::AccelerometerXyzSample) >= 4, "At least 30 bits needed.");
        constexpr uint_least32_t bottom_10_bits = 0b11'1111'1111u;
        constexpr uint_least32_t x_mask = bottom_10_bits;
        constexpr uint_least32_t y_mask = x_mask << 10;
        constexpr uint_least32_t z_mask = y_mask << 10;
        return ((static_cast<common::puppies::fifo::AccelerometerXyzSample>(record.z) << z_left_shift) & z_mask)
            | ((static_cast<common::puppies::fifo::AccelerometerXyzSample>(record.y) << y_left_shift) & y_mask)
            | ((static_cast<common::puppies::fifo::AccelerometerXyzSample>(record.x) >> x_right_shift) & x_mask)
            | (record.buffer_overflow ? buffer_overflow_mask : 0)
            | (record.sample_overrun ? sample_overrun_mask : 0);
    }

#if HAS_REMOTE_ACCELEROMETER()
    struct SampleStatus {
        bool buffer_overflow;
        bool sample_overrun;
    };
    static PrusaAccelerometer::Acceleration unpack_sample(SampleStatus &sampleStatus, common::puppies::fifo::AccelerometerXyzSample sample);
#endif
};
