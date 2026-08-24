#include "selftest_result_type.hpp"
#include <printers.h>

#include <option/has_switched_fan_test.h>
#include <option/has_dwarf.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif
#include <logging/log.hpp>
#include <selftest_result_evaluation.hpp>

LOG_COMPONENT_REF(Selftest);

void SelftestResult_Log(const SelftestResult &results) {
    for (auto tool : PhysicalToolIndex::all()) {
#if HAS_TOOLCHANGER() && HAS_DWARF()
        if (buddy::puppies::dwarfs[tool].is_enabled() == false) {
            continue;
        }
#endif

        log_info(Selftest, "Print fan %u result is %s", tool.to_raw(), ToString(results.get_print_fan(tool)));
        log_info(Selftest, "Heatbreak fan %u result is %s", tool.to_raw(), ToString(results.get_heatbreak_fan(tool)));
#if HAS_SWITCHED_FAN_TEST()
        log_info(Selftest, "Fans switched %u result is %s", tool.to_raw(), ToString(results.get_fans_switched(tool)));
#endif /* HAS_SWITCHED_FAN_TEST() */
        log_info(Selftest, "Nozzle heater %u result is %s", tool.to_raw(), ToString(results.get_nozzle_heater(tool)));
        log_info(Selftest, "Filament sensor %u result is %s", tool.to_raw(), ToString(SelftestSnake::map_fsensor_calibration_result(GetExtruderFSensor(tool))));
        log_info(Selftest, "Side filament sensor %u result is %s", tool.to_raw(), ToString(SelftestSnake::map_fsensor_calibration_result(GetSideFSensor(tool))));
#if HAS_LOADCELL()
        log_info(Selftest, "Loadcell result %u is %s", tool.to_raw(), ToString(results.get_loadcell(tool)));
#endif /*HAS_LOADCELL()*/
    }
    log_info(Selftest, "X axis result is %s", ToString(results.get_xaxis()));
    log_info(Selftest, "Y axis result is %s", ToString(results.get_yaxis()));
    log_info(Selftest, "Z axis result is %s", ToString(results.get_zaxis()));
    log_info(Selftest, "Z calibration result is %s", ToString(results.get_zalign()));
    log_info(Selftest, "Bed heater result is %s", ToString(results.get_bed_heater()));
    log_info(Selftest, "Ethernet result is %s", ToString(results.get_ethernet()));
    log_info(Selftest, "Wifi result is %s", ToString(results.get_wifi()));
}
