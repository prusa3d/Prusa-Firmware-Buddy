/**
 * @file selftest_result_fsensor.hpp
 * @author Radek Vana
 * @brief
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"

class ResultFSensor : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult tested;
    SelfTestViewTextWithIcon skipped;

public:
    ResultFSensor();

    void SetState(TestResult res);
};
