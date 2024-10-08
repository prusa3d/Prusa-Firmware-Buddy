#include "selftest_result_type.hpp"
#include <printers.h>

#include <option/has_switched_fan_test.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif
#include <logging/log.hpp>

LOG_COMPONENT_REF(Selftest);

namespace {
bool passed_for_all_that_always_need_to_pass(const SelftestResult &results) {
    for (int e = 0; e < HOTENDS; e++) {
#if HAS_TOOLCHANGER()
        if (buddy::puppies::dwarfs[e].is_enabled() == false) {
            continue;
        }
#endif /*HAS_TOOLCHANGER()*/

        if (results.tools[e].printFan != TestResult_Passed) {
            return false;
        }
        if (results.tools[e].heatBreakFan != TestResult_Passed) {
            return false;
        }
#if HAS_SWITCHED_FAN_TEST()
        if (results.tools[e].fansSwitched != TestResult_Passed) {
            return false;
        }
#endif /* HAS_SWITCHED_FAN_TEST() */
        if (results.tools[e].nozzle != TestResult_Passed) {
            return false;
        }
#if HAS_LOADCELL()
        if (results.tools[e].loadcell != TestResult_Passed) {
            return false;
        }
#endif /*HAS_LOADCELL()*/
    }
    if (results.xaxis != TestResult_Passed) {
        return false;
    }
    if (results.yaxis != TestResult_Passed) {
        return false;
    }
    if (results.zaxis != TestResult_Passed) {
        return false;
    }
    if (results.bed != TestResult_Passed) {
        return false;
    }

    return true;
}
} // namespace

bool SelftestResult_Passed_All(const SelftestResult &results) {
    for (int e = 0; e < HOTENDS; ++e) {
#if FILAMENT_SENSOR_IS_ADC()
        if (results.tools[e].fsensor != TestResult_Passed) {
            return false;
        }
#endif /*FILAMENT_SENSOR_IS_ADC()*/
    }
    return passed_for_all_that_always_need_to_pass(results);
}

bool SelftestResult_Passed_Mandatory(const SelftestResult &results) {
    for (int e = 0; e < HOTENDS; ++e) {
#if FILAMENT_SENSOR_IS_ADC()
    #if PRINTER_IS_PRUSA_MK4()
        if (results.tools[e].fsensor == TestResult_Failed) {
            return false;
        }
    #else
        if (results.tools[e].fsensor != TestResult_Passed) {
            return false;
        }
    #endif
#endif /*FILAMENT_SENSOR_IS_ADC()*/
    }
    return passed_for_all_that_always_need_to_pass(results);
}

bool SelftestResult_Failed(const SelftestResult &results) {
    for (int e = 0; e < HOTENDS; e++) {
#if HAS_TOOLCHANGER()
        if (buddy::puppies::dwarfs[e].is_enabled() == false) {
            continue;
        }
#endif /*HAS_TOOLCHANGER()*/

        if (results.tools[e].printFan == TestResult_Failed) {
            return true;
        }
        if (results.tools[e].heatBreakFan == TestResult_Failed) {
            return true;
        }

#if HAS_SWITCHED_FAN_TEST()
        if (results.tools[e].fansSwitched == TestResult_Failed) {
            return true;
        }
#endif /* HAS_SWITCHED_FAN_TEST() */
        if (results.tools[e].nozzle == TestResult_Failed) {
            return true;
        }
#if FILAMENT_SENSOR_IS_ADC()
        if (results.tools[e].fsensor == TestResult_Failed) {
            return true;
        }
#endif /*FILAMENT_SENSOR_IS_ADC()*/
#if HAS_LOADCELL()
        if (results.tools[e].loadcell == TestResult_Failed) {
            return true;
        }
#endif /*HAS_LOADCELL()*/
    }
    if (results.xaxis == TestResult_Failed) {
        return true;
    }
    if (results.yaxis == TestResult_Failed) {
        return true;
    }
    if (results.zaxis == TestResult_Failed) {
        return true;
    }
    if (results.bed == TestResult_Failed) {
        return true;
    }
    return false;
}

void SelftestResult_Log(const SelftestResult &results) {
    for (int e = 0; e < HOTENDS; e++) {
#if HAS_TOOLCHANGER()
        if (buddy::puppies::dwarfs[e].is_enabled() == false) {
            continue;
        }
#endif /*HAS_TOOLCHANGER()*/

        log_info(Selftest, "Print fan %u result is %s", e, ToString(results.tools[e].printFan));
        log_info(Selftest, "Heatbreak fan %u result is %s", e, ToString(results.tools[e].heatBreakFan));
#if HAS_SWITCHED_FAN_TEST()
        log_info(Selftest, "Fans switched %u result is %s", e, ToString(results.tools[e].fansSwitched));
#endif /* HAS_SWITCHED_FAN_TEST() */
        log_info(Selftest, "Nozzle heater %u result is %s", e, ToString(results.tools[e].nozzle));
#if FILAMENT_SENSOR_IS_ADC()
        log_info(Selftest, "Filament sensor %u result is %s", e, ToString(results.tools[e].fsensor));
        log_info(Selftest, "Side filament sensor %u result is %s", e, ToString(results.tools[e].sideFsensor));
#endif /*FILAMENT_SENSOR_IS_ADC()*/
#if HAS_LOADCELL()
        log_info(Selftest, "Loadcell result %u is %s", e, ToString(results.tools[e].loadcell));
#endif /*HAS_LOADCELL()*/
    }
    log_info(Selftest, "X axis result is %s", ToString(results.xaxis));
    log_info(Selftest, "Y axis result is %s", ToString(results.yaxis));
    log_info(Selftest, "Z axis result is %s", ToString(results.zaxis));
    log_info(Selftest, "Z calibration result is %s", ToString(results.zalign));
    log_info(Selftest, "Bed heater result is %s", ToString(results.bed));
    log_info(Selftest, "Ethernet result is %s", ToString(results.eth));
    log_info(Selftest, "Wifi result is %s", ToString(results.wifi));
}
