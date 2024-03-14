// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_fan_config.hpp"

namespace selftest {

class FanHandler {
public:
    using FanCtlFnc = CFanCtl &(*)(size_t);

    FanHandler(const char *name, const FanCtlFnc &fanctl_fnc, uint8_t tool_nr);

    void enter_selftest();
    void exit_selftest();
    void set_pwm(uint8_t pwm);
    void reset_samples();
    void record_sample();
    void evaluate(const FanConfig &fan_config, uint16_t avg_rpm);
    uint16_t calculate_avg_rpm() const;

    bool is_failed() { return failed; }

private:
    const char *name;
    const FanCtlFnc fanctl_fnc;
    uint8_t tool_nr { 0 };
    bool failed { false };
    uint16_t sample_count { 0 };
    uint32_t sample_sum { 0 };
};

class CSelftestPart_Fan {
    IPartHandler &state_machine;
    const SelftestFansConfig &config;
    SelftestFanHotendResult &result;

    uint32_t start_time { 0 };
    uint32_t end_time { 0 };

    FanHandler print_fan;
    FanHandler heatbreak_fan;

    static uint32_t estimate();
    void update_progress();

public:
    CSelftestPart_Fan(IPartHandler &state_machine, const SelftestFansConfig &config,
        SelftestFanHotendResult &result);
    ~CSelftestPart_Fan();

    LoopResult state_start();
    LoopResult state_wait_rpm_100_percent();
    LoopResult state_measure_rpm_100_percent();
    LoopResult state_wait_rpm_0_percent();
    LoopResult state_wait_rpm_20_percent();
    LoopResult state_measure_rpm_20_percent();
};

}; // namespace selftest
