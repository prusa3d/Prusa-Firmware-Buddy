/**
 * @file selftest_result_fans.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_fans.hpp"
#include "i18n.h"
#include "img_resources.hpp"

ResultFans::ResultFans()
    : SelfTestGroup(_("Fans check"))
    , heatbreak(_("Hotend fan"), &img::fan_16x16, TestResult_Unknown)
    , print(_("Print fan"), &img::turbine_16x16, TestResult_Unknown)
#if not PRINTER_IS_PRUSA_MINI()
    , fans_switched(_("Checking for switched fans"), nullptr, TestResult_Unknown)
#endif
{
    Add(heatbreak);
    Add(print);
#if not PRINTER_IS_PRUSA_MINI()
    Add(fans_switched);
#endif

    failed = false;
}

void ResultFans::SetState(TestResult hb_fan, TestResult print_fan, [[maybe_unused]] TestResult fans_swtchd) {
    heatbreak.SetState(hb_fan);
    print.SetState(print_fan);
#if not PRINTER_IS_PRUSA_MINI()
    fans_switched.SetState(fans_swtchd);
#endif

    failed = (hb_fan == TestResult_Failed || print_fan == TestResult_Failed
#if not PRINTER_IS_PRUSA_MINI()
        || fans_swtchd == TestResult_Failed
#endif
    );
}
