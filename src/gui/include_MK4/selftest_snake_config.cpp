#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <screen_menu_selftest_snake_result_parsing.hpp>
#include <option/has_toolchanger.h>
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
                return evaluate_results(sr.tools[e].printFan, sr.tools[e].heatBreakFan, sr.tools[e].fansSwitched);
            });
    case Action::ZAlign:
        return evaluate_results(sr.zalign);
    case Action::XYCheck:
        return evaluate_results(sr.xaxis, sr.yaxis);
    case Action::Loadcell:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].loadcell);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].loadcell);
        }
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::Heaters:
        return evaluate_results(sr.bed, merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        }));
    case Action::FilamentSensorCalibration:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].fsensor);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].fsensor);
        }
    case Action::Gears:
        return evaluate_results(sr.gears);
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
}

uint8_t get_tool_mask([[maybe_unused]] Tool tool) {
#if HAS_TOOLCHANGER()
    switch (tool) {
    case Tool::Tool1:
        return ToolMask::ToolO;
    case Tool::Tool2:
        return ToolMask::Tool1;
    case Tool::Tool3:
        return ToolMask::Tool2;
    case Tool::Tool4:
        return ToolMask::Tool3;
    case Tool::Tool5:
        return ToolMask::Tool4;
        break;
    default:
        assert(false);
        break;
    }
#endif
    return ToolMask::AllTools;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::Fans:
        return stmFans;
    case Action::XYCheck:
        return stmXYAxisWithMotorDetection;
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
    case Action::Gears:
        return stmGears;
    case Action::_count:
        break;
    }
    assert(false);
    return stmNone;
}

} // namespace SelftestSnake
