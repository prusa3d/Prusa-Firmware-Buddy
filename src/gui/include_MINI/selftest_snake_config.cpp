#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <selftest_result_evaluation.hpp>
#include <config_store/store_instance.hpp>
#include <common/SteelSheets.hpp>
#include <bsod/bsod.h>

namespace SelftestSnake {
TestResult get_test_result(Action action, [[maybe_unused]] ToolMask tool) {
    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](PhysicalToolIndex e) {
                return sr.evaluate_fans(e);
            });
    case Action::XYCheck:
        return test_result::evaluate_results(sr.get_xaxis(), sr.get_yaxis());
    case Action::ZCheck:
        return sr.get_zaxis();
    case Action::Heaters:
        return test_result::evaluate_results(sr.get_bed_heater(), merge_hotends_evaluations([&](PhysicalToolIndex e) {
            return sr.get_nozzle_heater(e);
        }));
    case Action::FirstLayer:
        // instead of test result that isn't saved to eeprom, consider calibrated sheet as passed.
        return SteelSheets::IsSheetCalibrated(config_store().active_sheet.get()) ? TestResult::passed : TestResult::unknown;
    case Action::FilamentSensorCalibration:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return get_fsensor_calibration_result(e);
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
    case Action::FirstLayer:
        return stmFirstLayer;

    case Action::Fans:
    case Action::FilamentSensorCalibration:
    case Action::_count:
        // Implemented as a gcode/invalid
        bsod_unreachable();
        break;
    }

    bsod_unreachable();
}

} // namespace SelftestSnake
