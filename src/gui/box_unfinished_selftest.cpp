#include "box_unfinished_selftest.hpp"
#include <selftest_result_type.hpp>
#include "printers.h"
#include <config_store/store_instance.hpp>
#include <common/SteelSheets.hpp>

#if PRINTER_IS_PRUSA_XL
    #include <module/prusa/toolchanger.h>
#endif

bool selftest_warning_selftest_finished() {

    [[maybe_unused]] SelftestResult sr = config_store().selftest_result.get();

    [[maybe_unused]] auto all_passed = [](std::same_as<TestResult> auto... results) -> bool {
        static_assert(sizeof...(results) > 0, "Pass at least one result");

        return ((results == TestResult_Passed) && ...); // all passed
    };
#if (PRINTER_IS_PRUSA_XL)
    if (!all_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed, config_store().selftest_result_nozzle_diameter.get(), config_store().selftest_result_phase_stepping.get())) {
        return false;
    }
    for (int8_t e = 0; e < HOTENDS; e++) {
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
        if (!all_passed(sr.tools[e].printFan, sr.tools[e].heatBreakFan,
                sr.tools[e].nozzle, sr.tools[e].fsensor,
                sr.tools[e].loadcell, sr.tools[e].fansSwitched)) {
            return false;
        }
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            if (!all_passed(sr.tools[e].dockoffset, sr.tools[e].tooloffset)) {
                return false;
            }
        }
    }

    return true;
#elif (PRINTER_IS_PRUSA_MK4)
    if (!all_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed)) {
        return false;
    }

    if (sr.gears == TestResult_Failed) { // skipped/unknown gears are also OK
        return false;
    }

    HOTEND_LOOP()
    if (!all_passed(sr.tools[e].printFan, sr.tools[e].heatBreakFan, sr.tools[e].nozzle, sr.tools[e].fsensor, sr.tools[e].loadcell, sr.tools[e].fansSwitched)) {
        return false;
    }

    return true;

#elif PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_MINI
    if (!all_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed)) {
        return false;
    }

    if (!SteelSheets::IsSheetCalibrated(config_store().active_sheet.get())) {
        return false;
    }

    HOTEND_LOOP()
    if (!all_passed(sr.tools[e].printFan, sr.tools[e].heatBreakFan, sr.tools[e].nozzle
    #if not PRINTER_IS_PRUSA_MINI
            ,
            sr.tools[e].fansSwitched
    #endif
            )) {
        return false;
    }

    return true;

#else
    assert(false && "Not yet implemented");
    return false;
#endif
}

void warn_unfinished_selftest_msgbox() {
    if (!selftest_warning_selftest_finished()) {
        MsgBoxWarning(_("Please complete Calibrations & Tests before using the printer."), Responses_Ok);
    }
}
