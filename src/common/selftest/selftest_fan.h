// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_fan_config.hpp"

namespace selftest {

class FanHandler {
public:
    using FanCtlFnc = CFanCtlCommon &(*)(size_t);

    FanHandler(const char *name, const FanCtlFnc &fanctl_fnc, uint8_t tool_nr);

    void enter_selftest();
    void exit_selftest();
    void set_pwm(uint8_t pwm);
    void reset_samples();
    void record_sample();
    void evaluate(const FanConfig &fan_config, uint16_t avg_rpm);
    uint16_t calculate_avg_rpm() const;

    bool is_failed() const { return failed; }

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

    static bool are_fans_switched(const FanHandler &print_fan, const FanHandler &heatbreak_fan, const SelftestFansConfig &config, const uint16_t print_fan_rpm, const uint16_t heatbreak_fan_rpm, SelftestFanHotendResult &result);

public:
    CSelftestPart_Fan(IPartHandler &state_machine, const SelftestFansConfig &config,
        SelftestFanHotendResult &result);
    ~CSelftestPart_Fan();

    LoopResult state_start();
    LoopResult state_wait_rpm_100_percent();
    LoopResult state_measure_rpm_100_percent();
#if PRINTER_IS_PRUSA_MK3_5()
    LoopResult state_manual_check_init();
    LoopResult state_manual_check_wait_fan();
    LoopResult state_manual_check_ask();
#endif
    LoopResult state_rpm_0_init();
    LoopResult state_wait_rpm_0_percent();
    LoopResult state_wait_rpm_40_percent();
    LoopResult state_measure_rpm_40_percent();
};

}; // namespace selftest
