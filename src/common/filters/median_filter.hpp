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
    int median_3_i32(int32_t *nums);
    void reset();

private:
    static constexpr uint32_t fs_raw_buffer_size = 3;
    static_assert(fs_raw_buffer_size == 3, "Current implementation handles median of 3 only");
    int32_t m_samples[fs_raw_buffer_size] = { 0 };
    bool m_output_valid = false;
    uint8_t m_next_sample_index = 0;
};
