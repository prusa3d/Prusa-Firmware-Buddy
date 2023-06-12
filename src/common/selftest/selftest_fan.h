// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_fan_config.hpp"

namespace selftest {

class FanHandler {
public:
    FanHandler(const char *name, const FanConfig<2> &config, uint8_t tool_nr);

    void enter_selftest();
    void exit_selftest();
    void set_pwm(uint8_t pwm);
    uint8_t get_pwm();
    uint8_t get_pwm_percent();
    uint16_t get_actual_rpm();

    void record_sample();
    void evaluate(uint8_t step);
    void next_step();
    uint16_t get_avg_rpm() { return avg_rpm; }

    bool is_failed() { return failed || failed_at_last_step; }
    bool is_failed_at_last_step() { return failed_at_last_step; }

private:
    const char *name;
    const FanConfig<2> &config;
    uint8_t tool_nr { 0 };

    uint16_t sample_count { 0 };
    uint32_t sample_sum { 0 };
    uint16_t avg_rpm { 0 }; // stored only so we can pull it out to compare max rpm for the switched test
    bool failed { false };
    bool failed_at_last_step { false };
};

class CSelftestPart_Fan {
    IPartHandler &state_machine;
    const SelftestFansConfig &config;
    SelftestFanHotendResult &result;

    uint32_t start_time { 0 };
    uint32_t end_time { 0 };

    FanHandler print_fan;
    FanHandler heatbreak_fan;

    uint8_t step { 0 };
    bool do_fans_switched_prompt { false };

    bool wait_for_spindown { true };

    static uint32_t estimate(const SelftestFansConfig &config);
    void update_progress();

public:
    CSelftestPart_Fan(IPartHandler &state_machine, const SelftestFansConfig &config,
        SelftestFanHotendResult &result);
    ~CSelftestPart_Fan();

    LoopResult state_start();
    LoopResult state_wait_spindown();
    LoopResult state_cycle_mark() { return LoopResult::MarkLoop; }
    LoopResult state_wait_rpm();
    LoopResult state_measure_rpm();
};

};
