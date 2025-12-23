#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <selftest_result_evaluation.hpp>
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

namespace SelftestSnake {
TestResult get_test_result(Action action, Tool tool) {
    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].evaluate_fans());
            });
    case Action::ZAlign:
        return evaluate_results(sr.zalign);
    case Action::XYCheck:
        return evaluate_results(sr.xaxis, sr.yaxis);
    case Action::Loadcell:
        return merge_hotends(tool, [&](const int8_t e) {
            return evaluate_results(sr.tools[e].loadcell);
        });
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::Heaters:
        return evaluate_results(sr.bed, merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        }));
#if HAS_DOOR_SENSOR_CALIBRATION()
    case Action::DoorSensor:
        return evaluate_results(config_store().selftest_result_door_sensor.get());
#endif
    case Action::FilamentSensorCalibration:
        return merge_hotends(tool, [&](const int8_t e) {
            return evaluate_results(sr.tools[e].fsensor);
        });
    case Action::Gears:
        return merge_hotends(tool, [&](const int8_t e) {
            return evaluate_results(sr.tools[e].gears);
        });
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
}

ToolMask get_tool_mask([[maybe_unused]] Tool tool) {
    return ToolMask::AllTools;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::XYCheck:
        return stmXYAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        return stmHeaters;
    case Action::FilamentSensorCalibration:
        return stmFSensor;
    case Action::Loadcell:
        return stmLoadcell;
    case Action::ZAlign:
        return stmZcalib;
#if HAS_DOOR_SENSOR_CALIBRATION()
    case Action::DoorSensor:
#endif
    case Action::Fans:
    case Action::Gears:
        bsod("get_test_mask");
    case Action::_count:
        break;
    }
    assert(false);
    return stmNone;
}

} // namespace SelftestSnake
