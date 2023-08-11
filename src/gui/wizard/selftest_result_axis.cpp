/**
 * @file selftest_result_axis.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_axis.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <option/has_loadcell.h>

ResultAxis::ResultAxis()
    : SelfTestGroup(_("Axis check"))
    , x(_("axis"), &img::x_axis_16x16, TestResult_Unknown)
    , y(_("axis"), &img::y_axis_16x16, TestResult_Unknown)
    , z(_("axis"), &img::z_axis_16x16, TestResult_Unknown)
#if HAS_LOADCELL()
    , txt(_("Axis check was skipped because Loadcell check failed."), is_multiline::yes)
#else
    , txt(_("Axis check was skipped"), is_multiline::yes)
#endif
{
}

void ResultAxis::SetState(TestResult x_res, TestResult y_res, TestResult z_res) {
    // if all axis were skipped, show message instead of results
    // TODO there should be some information send from selftest instead
    if (x_res == TestResult_Skipped) {
        Remove(x);
        Remove(y);
        Remove(z);
        Add(txt);
        failed = false;
    } else {
        Remove(txt);
        x.SetState(x_res);
        y.SetState(y_res);
        z.SetState(z_res);
        Add(x);
        Add(y);
        Add(z);
        failed = (x_res == TestResult_Failed || y_res == TestResult_Failed || z_res == TestResult_Failed);
    }
}
