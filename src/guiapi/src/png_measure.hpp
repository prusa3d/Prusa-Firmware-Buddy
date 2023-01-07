/**
 * @file png_measure.hpp
 * @brief measure time needed for png to draw
 */

#pragma once
#include <cstdint>

struct PNGMeasure {
    volatile uint32_t start_us;

    [[nodiscard]] PNGMeasure();
    ~PNGMeasure();
};
