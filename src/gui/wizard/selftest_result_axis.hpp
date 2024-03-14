/**
 * @file selftest_result_axis.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of axis test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"

class ResultAxis : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult x;
    SelfTestViewTextWithIconAndResult y;
    SelfTestViewTextWithIconAndResult z;
    SelfTestViewText txt;

public:
    ResultAxis();

    void SetState(TestResult x_res, TestResult y_res, TestResult z_res);
};
