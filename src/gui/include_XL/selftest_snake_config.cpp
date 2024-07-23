#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <screen_menu_selftest_snake_result_parsing.hpp>
#include <config_store/store_instance.hpp>

#include <option/has_side_fsensor.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
    #if HAS_SIDE_FSENSOR()
        #include <filament_sensors_handler_XL_remap.hpp>
    #endif /*HAS_SIDE_FSENSOR()*/
#endif /*HAS_TOOLCHANGER()*/

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
    case Action::YCheck:
        return evaluate_results(sr.yaxis);
    case Action::XCheck:
        return evaluate_results(sr.xaxis);
    case Action::DockCalibration:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].dockoffset);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].dockoffset);
        }
    case Action::Loadcell:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].loadcell);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].loadcell);
        }
    case Action::ToolOffsetsCalibration:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].tooloffset);
            });
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::BedHeaters:
        return evaluate_results(sr.bed);
    case Action::NozzleHeaters:
        return merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        });
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
    case Action::PhaseSteppingCalibration:
        return evaluate_results(config_store().selftest_result_phase_stepping.get());
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
}

ToolMask get_tool_mask(Tool tool) {
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
    case Action::YCheck:
        return stmYAxis;
    case Action::XCheck:
        return stmXAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        return stmHeaters;
    case Action::BedHeaters:
        return stmHeaters_bed;
    case Action::NozzleHeaters:
        return stmHeaters_noz;
    case Action::FilamentSensorCalibration:
        return stmFSensor;
    case Action::Loadcell:
        return stmLoadcell;
    case Action::ZAlign:
        return stmZcalib;
    case Action::DockCalibration:
        return stmDocks;
    case Action::ToolOffsetsCalibration:
        return stmToolOffsets;
    case Action::PhaseSteppingCalibration:
        bsod("get_test_mask");
        break;
    case Action::_count:
        break;
    }
    assert(false);
    return stmNone;
}

void ask_config(Action action) {
    switch (action) {
    case Action::FilamentSensorCalibration: {
#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
        side_fsensor_remap::ask_to_remap(); // Ask user whether to remap filament sensors
#endif /*HAS_TOOLCHANGER()*/
    } break;

    default:
        break;
    }
}

Tool get_last_enabled_tool() {
#if HAS_TOOLCHANGER()
    for (int i = EXTRUDERS - 1; i >= 0; --i) {
        if (prusa_toolchanger.is_tool_enabled(i)) {
            return static_cast<Tool>(i);
        }
    }
#endif /*HAS_TOOLCHANGER()*/
    return Tool::Tool1;
}

Tool get_next_tool(Tool tool) {
#if HAS_TOOLCHANGER()
    assert(tool != get_last_enabled_tool() && "Unhandled edge case");
    do {
        tool = tool + 1;
    } while (!prusa_toolchanger.is_tool_enabled(ftrstd::to_underlying(tool)));
#endif /*HAS_TOOLCHANGER()*/
    return tool;
}

} // namespace SelftestSnake
