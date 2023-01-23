/**
 * @file selftest_result_wifi.cpp
 * @author Radek Vana
 * @date 2022-01-23
 */

#include "selftest_result_wifi.hpp"
#include "i18n.h"
#include "selftest_eeprom.hpp"
#include "png_resources.hpp"

static string_view_utf8 getText(TestResultNet_t res) {
    switch (res) {
    case TestResultNet_t::Unknown:
        return _("Test did not run");
    case TestResultNet_t::Down:
    case TestResultNet_t::NoAddress:
        return _("Inactive");
    case TestResultNet_t::Up:
        return _("Connected");
    case TestResultNet_t::Unlinked:
        break;
    }
    return _("WiFi module not connected");
}

ResultWifi::ResultWifi(TestResultNet_t res)
    : SelfTestGroup(_("WiFi connection"))
    , connected(getText(res), png::wifi_16x16, TestResult_t::Passed, is_multiline::yes)
    , not_connected(getText(res), png::wifi_16x16, is_multiline::yes) {
    switch (res) {
    case TestResultNet_t::Up:
    case TestResultNet_t::NoAddress:
    case TestResultNet_t::Down:
        Add(connected);
        break;
    default:
        Add(not_connected);
        break;
    }
}
