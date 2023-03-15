/**
 * @file selftest_result_fans.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of fans test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"

class ResultFans : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult heatbreak;
    SelfTestViewTextWithIconAndResult print;

public:
    ResultFans();

    void SetState(TestResult hb_fan, TestResult print_fan);
};
