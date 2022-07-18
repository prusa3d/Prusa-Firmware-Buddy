/**
 * @file selftest_result_fans.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of fans test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_eeprom.hpp"

class ResultFans : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult heatbreak;
    SelfTestViewTextWithIconAndResult print;

public:
    ResultFans(TestResult_t hb_fan, TestResult_t print_fan);
};
