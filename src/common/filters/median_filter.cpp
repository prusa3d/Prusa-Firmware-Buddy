//
// Created by Tomas Kucera on 17.10.22.
//
#include "median_filter.hpp"

/**
 * @brief filter next sample
 *
 * @param [in,out] Input sample, outputs filtered sample
 * @retval true output valid
 * @retval false output not valid, insufficient number of samples collected yet
 */
bool MedianFilter::filter(int32_t &sample) {
    m_samples[m_next_sample_index] = sample;
    ++m_next_sample_index;
    if (m_next_sample_index >= fs_raw_buffer_size) {
        m_next_sample_index = 0;
        m_output_valid = true;
    }

    // Optional: We can store single comparison that can be used next time.
    sample = m_samples[median_3_i32(m_samples)];
    return m_output_valid;
}

/// Returns index of median of 3 numbers
int MedianFilter::median_3_i32(int32_t *nums) {
    // Compare each three number to find middle number
    if (nums[0] > nums[1]) {
        if (nums[1] > nums[2])
            return 1;
        if (nums[0] > nums[2])
            return 2;
        return 0;
    } else {
        if (nums[0] > nums[2])
            return 0;
        if (nums[1] > nums[2])
            return 2;
        return 1;
    }
}

void MedianFilter::reset() {
    m_output_valid = false;
    m_next_sample_index = 0;
}
