/**
 * @file selftest_result_fsensor.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_fsensor.hpp"
#include "i18n.h"
#include "img_resources.hpp"

ResultFSensor::ResultFSensor()
    : SelfTestGroup(_("Filament sensor check"))
    , tested(_("Filament sensor"), &img::spool_16x16, TestResult_Unknown)
    , skipped(_("Test skipped by user."), &img::spool_16x16) {
}

void ResultFSensor::SetState(TestResult res) {
    if (res == TestResult_Skipped) {
        Remove(tested);
        Add(skipped);
    } else {
        Remove(skipped);
        tested.SetState(res);
        Add(tested);
    }

    failed = (res == TestResult_Failed);
}
