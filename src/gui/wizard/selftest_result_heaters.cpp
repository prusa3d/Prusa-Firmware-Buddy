/**
 * @file selftest_result_heaters.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_heaters.hpp"
#include "i18n.h"

ResultHeaters::ResultHeaters(TestResult_t res_noz, TestResult_t res_bed)
    : SelfTestGroup(_("Heaters check"))
    , noz(_("Nozzle"), &png::nozzle_16x16, res_noz)
    , bed(_("Heatbed"), &png::heatbed_16x16, res_bed) {
    Add(noz);
    Add(bed);
    if (res_noz == TestResult_t::Failed || res_bed == TestResult_t::Failed) {
        failed = true;
    }
}
