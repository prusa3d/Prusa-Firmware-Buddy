/**
 * @file selftest_result_fans.hpp
 * @author Radek Vana
 * @brief Part of selftest result showing result of fans test
 * @date 2022-01-23
 */

#pragma once

#include "selftest_group.hpp"
#include "selftest_result_type.hpp"
#include <option/has_switched_fan_test.h>

class ResultFans : public SelfTestGroup {
    SelfTestViewTextWithIconAndResult heatbreak;
    SelfTestViewTextWithIconAndResult print;
#if HAS_SWITCHED_FAN_TEST()
    SelfTestViewTextWithIconAndResult fans_switched;
#endif /* HAS_SWITCHED_FAN_TEST */

public:
    ResultFans();

    void SetState(SelftestTool &tool);
};
