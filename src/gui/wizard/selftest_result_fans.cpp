/**
 * @file selftest_result_fans.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_fans.hpp"
#include "i18n.h"
#include "png_resources.hpp"

ResultFans::ResultFans()
    : SelfTestGroup(_("Fans check"))
    , heatbreak(_("Hotend fan"), png::fan_16x16, TestResult_Unknown)
    , print(_("Print fan"), png::turbine_16x16, TestResult_Unknown) {
    Add(heatbreak);
    Add(print);

    failed = false;
}

void ResultFans::SetState(TestResult hb_fan, TestResult print_fan) {
    heatbreak.SetState(hb_fan);
    print.SetState(print_fan);

    failed = (hb_fan == TestResult_Failed || print_fan == TestResult_Failed);
}
