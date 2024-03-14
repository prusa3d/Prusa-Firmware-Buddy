/**
 * @file selftest_loop_result.hpp
 * @author Radek Vana
 * @brief return value of selftest states
 * @date 2021-10-11
 */

#pragma once
#include <cstdint>
namespace selftest {

static constexpr size_t LoopMarkCount = 8;

enum class LoopResult : uint32_t {
    RunNext,
    RunCurrent,
    Abort,
    Fail,

    MarkLoop0 = 0b0100'0000, // all MarkLoops will have this bit set
    MarkLoop1,
    MarkLoop2,
    MarkLoop3,
    MarkLoop4,
    MarkLoop5,
    MarkLoop6,
    MarkLoop7,

    GoToMark0 = 0b1000'0000, // all GoToMarks will have this bit set
    GoToMark1,
    GoToMark2,
    GoToMark3,
    GoToMark4,
    GoToMark5,
    GoToMark6,
    GoToMark7
};

}; // namespace selftest
