/**
 * @file selftest_result_heaters.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of heater test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"

class ResultHeaters : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult noz;
    SelfTestViewTextWithIconAndResult bed;

public:
    ResultHeaters();

    void SetState(TestResult res_noz, TestResult res_bed);
};
