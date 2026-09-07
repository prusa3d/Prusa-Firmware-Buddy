#include <selftest_result_evaluation.hpp>
#include <selftest_result_type.hpp>
#include <option/has_switched_fan_test.h>
#include <option/has_gearbox_alignment.h>
#include <option/has_selftest.h>
#include <option/has_phase_stepping_selftest.h>
#include <option/filament_sensor.h>
#include <option/has_loadcell.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include <option/has_sheet_profiles.h>
#if HAS_SHEET_PROFILES()
    #include <common/SteelSheets.hpp>
#endif /* HAS_SHEET_PROFILES() */

#include <config_store/store_instance.hpp>

bool is_selftest_successfully_completed() {
#if DEVELOPER_MODE() || !HAS_SELFTEST() || PRINTER_IS_PRUSA_iX()
    return true;
#endif

    const SelftestResult sr = config_store().selftest_result.get();

    const auto all_passed = [](std::same_as<TestResult> auto... results) -> bool {
        static_assert(sizeof...(results) > 0, "Pass at least one result");

        return ((results == TestResult::passed) && ...); // all passed
    };

    if (!all_passed(sr.get_xaxis(), sr.get_yaxis(), sr.get_zaxis(), sr.get_bed_heater())) {
        return false;
    }

#if HAS_SHEET_PROFILES()
    if (!SteelSheets::IsSheetCalibrated(config_store().active_sheet.get())) {
        return false;
    }
#endif /* HAS_SHEET_PROFILES() */

    for (auto tool : PhysicalToolIndex::all()) {
#if HAS_TOOLCHANGER()
        if (!tool.is_enabled()) {
            continue;
        }

        // Without toolchanger, we don't have tool offsets
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            if (sr.get_dock_offset(tool) != TestResult::passed) {
                return false;
            }

    #if !HAS_INDX() // tool offset is not a selftest on INDX
            if (SelftestSnake::get_test_result(SelftestSnake::Action::ToolOffsetsCalibration, tool) != TestResult::passed) {
                return false;
            }
    #endif
        }

#endif

#if HAS_GEARBOX_ALIGNMENT()
        if (sr.get_gearbox(tool) == TestResult::failed) {
            return false;
        }
#endif /* HAS_GEARBOX_ALIGNMENT */

        // Skipped means the sensor is disabled by the user, which is acceptable.
        const auto fs_result = SelftestSnake::get_fsensor_calibration_result(tool);
        if (fs_result != TestResult::passed && fs_result != TestResult::skipped) {
            return false;
        }

#if HAS_LOADCELL()
        if (!all_passed(sr.get_loadcell(tool))) {
            return false;
        }
#endif /* HAS_LOADCELL() */

        if (sr.get_nozzle_heater(tool) != TestResult::passed) {
            return false;
        }
    }

    // Single source of truth for what the fan test covers on this printer - per-tool
    // fans plus whatever chassis fans the variant carries.
    if (SelftestSnake::get_test_result(SelftestSnake::Action::Fans, AllTools {}) != TestResult::passed) {
        return false;
    }

#if HAS_PHASE_STEPPING_SELFTEST()
    if (!all_passed(config_store().selftest_result_phase_stepping.get())) {
        return false;
    }
#endif /* HAS_PHASE_STEPPING_SELFTEST() */

    return true;
}
