#pragma once
#include <test_result.hpp>
#include <selftest_snake_config.hpp>
#include <inc/MarlinConfigPre.h>
#include <printers.h>
#include <feature/filament_sensor/filament_sensors_handler.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

namespace SelftestSnake {

constexpr TestResult merge_hotends_evaluations(std::invocable<PhysicalToolIndex> auto evaluate_one) {
    TestResult res { TestResult::passed };
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        res = test_result::evaluate_results(res, evaluate_one(tool));
    }
    return res;
};

constexpr TestResult merge_hotends(ToolMask tool_mask, stdext::inplace_function<TestResult(PhysicalToolIndex)> evaluate) {
    return match(
        tool_mask,
        [&](PhysicalToolIndex tool) { return evaluate(tool); },
        [&](AllTools) { return merge_hotends_evaluations(evaluate); });
}

/// Map a filament sensor's calibration status to a TestResult.
inline TestResult map_fsensor_calibration_result(IFSensor *sensor) {
    if (sensor == nullptr) {
        return TestResult::passed;
    } else if (sensor->is_calibrated()) {
        return TestResult::passed;
    } else if (!should_enable(sensor->id())) {
        return TestResult::skipped;
    } else {
        return TestResult::unknown;
    }
}

/// Map live filament sensor state to a TestResult for calibration checkmark display.
/// Checks both extruder and side sensor (if present) for the given tool index.
inline TestResult get_fsensor_calibration_result(PhysicalToolIndex tool_index) {
    return test_result::evaluate_results(
        map_fsensor_calibration_result(GetExtruderFSensor(tool_index)),
        map_fsensor_calibration_result(GetSideFSensor(tool_index)));
}

}; // namespace SelftestSnake

/**
 *  Check if all essential selftest passed
 */
bool is_selftest_successfully_completed();
