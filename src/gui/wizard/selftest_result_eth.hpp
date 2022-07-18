/**
 * @file selftest_result_eth.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of ethernet test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"

class ResultEth : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult connected;
    SelfTestViewTextWithIcon not_connected;

public:
    ResultEth(TestResultNet_t res);
};
