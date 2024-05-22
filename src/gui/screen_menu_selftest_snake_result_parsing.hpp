#pragma once
#include <inc/Conditionals_LCD.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

namespace SelftestSnake {
constexpr TestResult evaluate_results(std::same_as<TestResult> auto... results) {
    static_assert(sizeof...(results) > 0, "Pass at least one result");

    if (((results == TestResult_Passed) && ... && true)) { // all passed
        return TestResult_Passed;
    } else if (((results == TestResult_Failed) || ... || false)) { // any failed
        return TestResult_Failed;
    } else if (((results == TestResult_Skipped) || ... || false)) { // any skipped
        return TestResult_Skipped;
    } else { // only unknowns and passed (max n-1) are left
        return TestResult_Unknown;
    }
}

constexpr TestResult merge_hotends_evaluations(std::invocable<int8_t> auto evaluate_one) {
    TestResult res { TestResult_Passed };
    for (int8_t e = 0; e < HOTENDS; e++) {
#if HAS_TOOLCHANGER()
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
#endif
        res = evaluate_results(res, evaluate_one(e));
    }
    return res;
};

} // namespace SelftestSnake
