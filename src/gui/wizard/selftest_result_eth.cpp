/**
 * @file selftest_result_eth.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_eth.hpp"
#include "i18n.h"
#include "selftest_result_type.hpp"
#include "img_resources.hpp"

ResultEth::ResultEth(bool is_wifi)
    : SelfTestGroup(is_wifi ? _("WiFi connection") : _("Ethernet connection"))
    , skipped(_("Test did not run"), is_wifi ? &img::wifi_16x16 : &img::lan_16x16, is_multiline::yes)
    , not_connected(is_wifi ? _("WiFi not connected") : _("Ethernet cable not plugged in"), is_wifi ? &img::wifi_16x16 : &img::lan_16x16, is_multiline::yes)
    , inactive(_("Inactive"), is_wifi ? &img::wifi_16x16 : &img::lan_16x16, TestResult_Passed, is_multiline::yes)
    , connected(_("Connected"), is_wifi ? &img::wifi_16x16 : &img::lan_16x16, TestResult_Passed, is_multiline::yes) {
}

void ResultEth::SetState(TestResultNet res) {
    switch (res) {
    case TestResultNet_Unknown:
        Remove(not_connected);
        Remove(inactive);
        Remove(connected);
        Add(skipped);
        break;
    case TestResultNet_Unlinked:
        Remove(skipped);
        Remove(inactive);
        Remove(connected);
        Add(not_connected);
        break;
    case TestResultNet_Down:
    case TestResultNet_NoAddress:
        Remove(skipped);
        Remove(not_connected);
        Remove(connected);
        Add(inactive);
        break;
    case TestResultNet_Up:
        Remove(skipped);
        Remove(not_connected);
        Remove(inactive);
        Add(connected);
        break;
    }
}
