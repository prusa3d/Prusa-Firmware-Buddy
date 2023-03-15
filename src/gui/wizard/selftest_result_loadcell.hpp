/**
 * @file selftest_result_loadcell.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of loadcell test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"

class ResultLoadcell : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult loadcell;

public:
    ResultLoadcell();

    void SetState(TestResult res);
};
