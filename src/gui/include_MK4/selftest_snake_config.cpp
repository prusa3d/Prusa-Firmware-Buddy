#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <selftest_result_evaluation.hpp>
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#include <bsod/bsod.h>
#if HAS_TOOLCHANGER()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include <module/prusa/toolchanger.h>
#endif

namespace SelftestSnake {
TestResult get_test_result(Action action, ToolMask tool) {
    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](PhysicalToolIndex e) {
                return sr.evaluate_fans(e);
            });
    case Action::ZAlign:
        return sr.get_zalign();
    case Action::XYCheck:
        return test_result::evaluate_results(sr.get_xaxis(), sr.get_yaxis());
    case Action::Loadcell:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return sr.get_loadcell(e);
        });
    case Action::ZCheck:
        return sr.get_zaxis();
    case Action::Heaters:
        return test_result::evaluate_results(sr.get_bed_heater(), merge_hotends_evaluations([&](PhysicalToolIndex e) {
            return sr.get_nozzle_heater(e);
        }));
    case Action::FilamentSensorCalibration:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return get_fsensor_calibration_result(e);
        });
    case Action::Gears:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return sr.get_gearbox(e);
        });
    case Action::_count:
        break;
    }
    return TestResult::unknown;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::XYCheck:
        return stmXYAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        bsod("This should be gcode");
    case Action::Loadcell:
        return stmLoadcell;
    case Action::ZAlign:
        return stmZcalib;

    case Action::FilamentSensorCalibration:
    case Action::Fans:
    case Action::Gears:
    case Action::_count:
        // These should have special handling
        bsod_unreachable();
    }

    bsod_unreachable();
}

} // namespace SelftestSnake
