/**
 * @file selftest_result_loadcell.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_loadcell.hpp"
#include "i18n.h"
#include "img_resources.hpp"

ResultLoadcell::ResultLoadcell()
    : SelfTestGroup(_("Loadcell check"))
    , loadcell(_("Loadcell"), &img::nozzle_16x16, TestResult_Unknown) {
    Add(loadcell);
}

void ResultLoadcell::SetState(TestResult res) {
    loadcell.SetState(res);
    failed = (res == TestResult_Failed);
}
