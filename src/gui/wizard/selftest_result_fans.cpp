/**
 * @file selftest_result_fans.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_fans.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <option/has_switched_fan_test.h>

ResultFans::ResultFans()
    : SelfTestGroup(_("Fans check"))
    , heatbreak(_("Hotend fan"), &img::fan_16x16, TestResult_Unknown)
    , print(_("Print fan"), &img::turbine_16x16, TestResult_Unknown)
#if HAS_SWITCHED_FAN_TEST()
    , fans_switched(_("Checking for switched fans"), nullptr, TestResult_Unknown)
#endif /* HAS_SWITCHED_FAN_TEST() */
{
    Add(heatbreak);
    Add(print);
#if HAS_SWITCHED_FAN_TEST()
    Add(fans_switched);
#endif /* HAS_SWITCHED_FAN_TEST() */

    failed = false;
}

void ResultFans::SetState(SelftestTool &tool) {
    heatbreak.SetState(tool.heatBreakFan);
    print.SetState(tool.printFan);
#if HAS_SWITCHED_FAN_TEST()
    fans_switched.SetState(tool.fansSwitched);
#endif /* HAS_SWITCHED_FAN_TEST() */

    failed = tool.evaluate_fans() == TestResult_Failed;
}
