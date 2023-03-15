#include "box_unfinished_selftest.hpp"
#include <selftest_result_type.hpp>
#include "printers.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include <module/prusa/toolchanger.h>
#endif

namespace {
bool all_selftests_passed() {
#if (PRINTER_TYPE != PRINTER_PRUSA_XL)
    assert(false && "Not yet implemented");
    return false;
#else
    SelftestResult sr;
    eeprom_get_selftest_results(&sr);

    auto all_passed = [](std::same_as<TestResult> auto... results) -> bool {
        static_assert(sizeof...(results) > 0, "Pass at least one result");

        return ((results == TestResult_Passed) && ...); // all passed
    };

    if (!all_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed)) {
        return false;
    }
    for (int8_t e = 0; e < HOTENDS; e++) {
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
        if (!all_passed(sr.tools[e].printFan, sr.tools[e].heatBreakFan, sr.tools[e].nozzle, sr.tools[e].fsensor, sr.tools[e].loadcell)) {
            return false;
        }
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            if (!all_passed(sr.tools[e].kenneloffset, sr.tools[e].tooloffset)) {
                return false;
            }
        }
    }

    return true;
#endif
}
}

void warn_unfinished_selftest_msgbox() {
    if (!all_selftests_passed()) {
        MsgBoxWarning(_("Please complete Calibrations & Tests before using the printer."), Responses_Ok);
    }
}
