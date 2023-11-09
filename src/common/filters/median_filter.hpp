//
// Created by Tomas Kucera on 17.10.22.
//
#pragma once

#include <cinttypes>
#include <cstdio>

class MedianFilter {
public:
    MedianFilter() = default;
    bool filter(int32_t &sample);
    void reset();
#ifndef UNITTESTS
private:
#endif
    static constexpr uint32_t median_3_i32(const int32_t *nums) {
        // Compare each three number to find middle number
        if (nums[0] > nums[1]) {
            if (nums[1] > nums[2]) {
                return 1;
            }
            if (nums[0] > nums[2]) {
                return 2;
            }
            return 0;
        } else {
            if (nums[0] > nums[2]) {
                return 0;
            }
            if (nums[1] > nums[2]) {
                return 2;
            }
            return 1;
        }
    }

    static constexpr uint32_t fs_raw_buffer_size = 3;
    static_assert(fs_raw_buffer_size == 3, "Current implementation handles median of 3 only");
    int32_t m_samples[fs_raw_buffer_size] = { 0 };
    bool m_output_valid = false;
    uint8_t m_next_sample_index = 0;
};
