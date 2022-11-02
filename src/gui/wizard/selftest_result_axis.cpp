/**
 * @file selftest_result_axis.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_axis.hpp"
#include "i18n.h"

ResultAxis::ResultAxis(TestResult_t x_res, TestResult_t y_res, TestResult_t z_res)
    : SelfTestGroup(_("Axis check"))
    , x(_("X axis"), &png::x_axis_16x16, x_res)
    , y(_("Y axis"), &png::y_axis_16x16, y_res)
    , z(_("Z axis"), &png::z_axis_16x16, z_res)
    , txt(_("Axis check was skipped"), is_multiline::yes) {
    // if all axis were skipped, show message instead of results
    // TODO there should be some information send from selftest instead
    if (x_res == TestResult_t::Skipped) {
        Add(txt);
    } else {
        Add(x);
        Add(y);
        Add(z);
        if (x_res == TestResult_t::Failed || y_res == TestResult_t::Failed || z_res == TestResult_t::Failed) {
            failed = true;
        }
    }
}
