/// @file
#pragma once

#include <array>
#include <test_result.hpp>

namespace bed_fan {

/// @warning This type is used in config store and must not be changed.
struct SelftestResult {
    constexpr static size_t fan_count = 2;
    std::array<TestResult, fan_count> fans { TestResult::unknown, TestResult::unknown };

    bool operator==(const SelftestResult &) const = default;
};

} // namespace bed_fan
