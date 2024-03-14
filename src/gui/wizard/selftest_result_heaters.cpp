/**
 * @file selftest_result_heaters.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_heaters.hpp"
#include "img_resources.hpp"
#include "i18n.h"

ResultHeaters::ResultHeaters()
    : SelfTestGroup(_("Heaters check"))
    , noz(_("Nozzle"), &img::nozzle_16x16, TestResult_Unknown)
    , bed(_("Heatbed"), &img::heatbed_16x16, TestResult_Unknown) {
    Add(noz);
    Add(bed);
}

void ResultHeaters::SetState(TestResult res_noz, TestResult res_bed) {
    noz.SetState(res_noz);
    bed.SetState(res_bed);
    failed = (res_noz == TestResult_Failed || res_bed == TestResult_Failed);
}
