#pragma once
#include <test_result.hpp>
#include <array>

struct XBEFanTestResults {
    constexpr static size_t fan_count = 3;
    std::array<TestResult, fan_count> fans { TestResult::unknown, TestResult::unknown, TestResult::unknown };

    bool operator==(const XBEFanTestResults &oth) const = default;
};
