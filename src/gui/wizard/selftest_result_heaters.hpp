/**
 * @file selftest_result_heaters.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of heater test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_eeprom.hpp"

class ResultHeaters : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult noz;
    SelfTestViewTextWithIconAndResult bed;

public:
    ResultHeaters(TestResult_t res_noz, TestResult_t res_bed);
};
