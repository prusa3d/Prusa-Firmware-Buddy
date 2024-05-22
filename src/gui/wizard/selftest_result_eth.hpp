/**
 * @file selftest_result_eth.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of ethernet test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"

class ResultEth : public SelfTestGroup {
    SelfTestViewTextWithIcon skipped;
    SelfTestViewTextWithIcon not_connected;
    SelfTestViewTextWithIconAndResult inactive;
    SelfTestViewTextWithIconAndResult connected;

public:
    /**
     * @brief Test result for wifi or ethernet.
     * @param is_wifi true to use wifi icon and wifi not connected text
     */
    ResultEth(bool is_wifi);

    void SetState(TestResultNet res);
};
