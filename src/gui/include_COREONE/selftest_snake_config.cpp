#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <selftest_result_evaluation.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_switched_fan_test.h>

#include <option/has_chamber_filtration_api.h>

#if HAS_PRECISE_HOMING_COREXY()
    #include <module/prusa/homing_corexy.hpp>
#endif

#include <option/has_chamber_api.h>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

#include <option/has_xbuddy_extension.h>
#include <bsod/bsod.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

using namespace buddy;

namespace SelftestSnake {
TestResult get_test_result(Action action, [[maybe_unused]] ToolMask tool) {

    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans: {
        TestResult res = merge_hotends_evaluations(
            [&](PhysicalToolIndex e) {
                return sr.evaluate_fans(e);
            });
#if HAS_CHAMBER_API()
        switch (chamber().backend()) {
    #if HAS_XBUDDY_EXTENSION()
        case Chamber::Backend::xbuddy_extension: {
            const auto chamber_results = config_store().xbe_fan_test_results.get();
            static_assert(HAS_CHAMBER_FILTRATION_API());
            if (buddy::xbuddy_extension().using_filtration_fan_instead_of_cooling_fans()) {
                res = test_result::evaluate_results(res, chamber_results.fans[2]);
            } else {
                res = test_result::evaluate_results(res, chamber_results.fans[0]);
                res = test_result::evaluate_results(res, chamber_results.fans[1]);
            }
            break;
        }
    #endif /* HAS_XBUDDY_EXTENSION() */
        case Chamber::Backend::none:
            break;
        }
#endif /* HAS_CHAMBER_API() */
        return res;
    }
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
    case Action::Gears:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return sr.get_gearbox(e);
        });
    case Action::DoorSensor:
        return test_result::evaluate_results(config_store().selftest_result_door_sensor.get());
    case Action::FilamentSensorCalibration:
        return merge_hotends(tool, [&](const PhysicalToolIndex e) {
            return get_fsensor_calibration_result(e);
        });
    case Action::BeltTuning:
        return config_store().manual_belt_tuning_completed.get() ? TestResult::passed : TestResult::unknown;
    case Action::_count:
        break;
    }
    return TestResult::unknown;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::Fans:
    case Action::Gears:
    case Action::DoorSensor:
    case Action::FilamentSensorCalibration:
    case Action::BeltTuning:
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
#endif
        bsod("This should be gcode");
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
    case Action::_count:
        break;
    }
    debug_assert(false);
    return stmNone;
}

} // namespace SelftestSnake
