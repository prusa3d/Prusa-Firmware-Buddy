/**
 * @file selftest_result_fans.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_fans.hpp"
#include "i18n.h"
#include "resource.h"

ResultFans::ResultFans(TestResult_t hb_fan, TestResult_t print_fan)
    : SelfTestGroup(_("Fans check"))
    , heatbreak(_("Extruder fan"), &png::fan_16x16, hb_fan)
    , print(_("Print fan"), &png::turbine_16x16, print_fan) {
    Add(heatbreak);
    Add(print);
    if (hb_fan == TestResult_t::Failed || print_fan == TestResult_t::Failed) {
        failed = true;
    }
}
