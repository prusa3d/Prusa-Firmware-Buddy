/**
 * @file selftest_result_wifi.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of wifi test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_eeprom.hpp"

class ResultWifi : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult connected;
    SelfTestViewTextWithIcon not_connected;

public:
    ResultWifi(TestResultNet_t res);
};
