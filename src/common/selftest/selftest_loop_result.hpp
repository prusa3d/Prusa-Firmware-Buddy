/**
 * @file selftest_loop_result.hpp
 * @author Radek Vana
 * @brief return value of selftest states
 * @date 2021-10-11
 */

#pragma once

namespace selftest {

enum class LoopResult {
    RunNext,
    RunCurrent,
    Abort,
    Fail,
    MarkLoop,
    GoToMark
};

};
