#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <selftest_result_evaluation.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_precise_homing_corexy.h>
#include <bsod/bsod.h>

#if HAS_PRECISE_HOMING_COREXY()
    #include <module/prusa/homing_corexy.hpp>
#endif

namespace SelftestSnake {
TestResult get_test_result(Action action, [[maybe_unused]] ToolMask tool) {

    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](PhysicalToolIndex e) {
                return sr.evaluate_fans(e);
            });
    case Action::ZAlign:
        return sr.get_zalign();
    case Action::YCheck:
        return sr.get_yaxis();
    case Action::XCheck:
        return sr.get_xaxis();
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
        return corexy_home_is_calibrated() ? TestResult::passed : TestResult::unknown;
#endif
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
    case Action::PhaseSteppingCalibration:
        return test_result::evaluate_results(config_store().selftest_result_phase_stepping.get());
    case Action::BeltTuning:
        return config_store().manual_belt_tuning_completed.get() ? TestResult::passed : TestResult::unknown;
    case Action::_count:
        break;
    }
    return TestResult::unknown;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::YCheck:
        return stmYAxis;
    case Action::XCheck:
        return stmXAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        bsod("This should be gcode");

    case Action::Loadcell:
        return stmLoadcell;
    case Action::ZAlign:
        return stmZcalib;

    case Action::Fans:
    case Action::PhaseSteppingCalibration:
    case Action::BeltTuning:
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
#endif
    case Action::FilamentSensorCalibration:
    case Action::_count:
        // Implemented as a gcode/invalid
        bsod_unreachable();
    }
    debug_assert(false);
    return stmNone;
}

} // namespace SelftestSnake
