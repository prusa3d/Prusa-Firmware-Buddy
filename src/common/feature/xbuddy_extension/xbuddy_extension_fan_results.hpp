#pragma once
#include <selftest_result.hpp>
#include <array>

struct XBEFanTestResults {
    constexpr static size_t fan_count = 3;
    std::array<TestResult, fan_count> fans { TestResult_Unknown, TestResult_Unknown, TestResult_Unknown };

    bool operator==(const XBEFanTestResults &oth) const = default;
};
