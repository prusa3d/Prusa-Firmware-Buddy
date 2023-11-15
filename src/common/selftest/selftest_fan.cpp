// selftest_fan.cpp

#include "selftest_fan.h"
#include "wizard_config.hpp"
#include "fanctl.hpp"
#include "config_features.h" //EXTRUDER_AUTO_FAN_TEMPERATURE
#include "marlin_server.hpp" //marlin_server::get_temp_nozzle()
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

#include <algorithm>

// Start at 100% and wait longer to allow spreading stuck lubricant after first assembly.
// Then set PWM to 0% and wait for quite a long time to ensure fan stopped.
// (We can't measure RPM without at least some PWM)
// Then continue with 20% PWM to test if 20% is enough to start spinning.
static constexpr uint8_t pwm_100_percent = 255;
static constexpr uint8_t pwm_20_percent = 51;
static constexpr uint32_t state_measure_rpm_delay = 5000;
static constexpr uint32_t state_wait_rpm_100_percent_delay = 6000;
static constexpr uint32_t state_measure_rpm_100_percent_delay = state_measure_rpm_delay;
static constexpr uint32_t state_wait_rpm_0_percent_delay = 10000;
static constexpr uint32_t state_wait_rpm_20_percent_delay = 3000;
static constexpr uint32_t state_measure_rpm_20_percent_delay = state_measure_rpm_delay;

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

FanHandler::FanHandler(const char *name, const FanCtlFnc &fanctl_fnc, uint8_t tool_nr)
    : name(name)
    , fanctl_fnc(fanctl_fnc)
    , tool_nr(tool_nr) {}

void FanHandler::enter_selftest() {
    fanctl_fnc(tool_nr).EnterSelftestMode();
}

void FanHandler::exit_selftest() {
    fanctl_fnc(tool_nr).ExitSelftestMode();
}

void FanHandler::set_pwm(uint8_t pwm) {
    fanctl_fnc(tool_nr).SelftestSetPWM(pwm);
}

void FanHandler::record_sample() {
    sample_count++;
    sample_sum += fanctl_fnc(tool_nr).getActualRPM();
}

void FanHandler::reset_samples() {
    sample_count = 0;
    sample_sum = 0;
}

static bool is_rpm_within_bounds(const FanConfig &fan_config, uint16_t rpm) {
    return rpm > fan_config.rpm_min && rpm < fan_config.rpm_max;
}

uint16_t FanHandler::calculate_avg_rpm() const {
    return sample_count ? (sample_sum / sample_count) : 0;
}

void FanHandler::evaluate(const FanConfig &fan_config, uint16_t avg_rpm) {
    const uint16_t rpm_min = fan_config.rpm_min;
    const uint16_t rpm_max = fan_config.rpm_max;

    if (is_rpm_within_bounds(fan_config, avg_rpm)) {
        log_info(Selftest, "%s %u %u RPM in range (%u - %u)",
            name,
            tool_nr,
            avg_rpm,
            rpm_min,
            rpm_max);
    } else {
        log_error(Selftest, "%s %u %u RPM out of range (%u - %u)",
            name,
            tool_nr,
            avg_rpm,
            rpm_min,
            rpm_max);

        failed = true;
    }
}

CSelftestPart_Fan::CSelftestPart_Fan(IPartHandler &state_machine, const SelftestFansConfig &config,
    SelftestFanHotendResult &result)
    : state_machine(state_machine)
    , config(config)
    , result(result)
    , print_fan("Print fan", Fans::print, config.tool_nr)
    , heatbreak_fan("Heatbreak fan", Fans::heat_break, config.tool_nr) {
    print_fan.enter_selftest();
    heatbreak_fan.enter_selftest();
}

CSelftestPart_Fan::~CSelftestPart_Fan() {
    print_fan.exit_selftest();
    heatbreak_fan.exit_selftest();
}

uint32_t CSelftestPart_Fan::estimate() {
    uint32_t total_time
        = state_wait_rpm_100_percent_delay
        + state_measure_rpm_100_percent_delay
        + state_wait_rpm_0_percent_delay
        + state_wait_rpm_20_percent_delay
        + state_measure_rpm_20_percent_delay;
    return total_time;
}

LoopResult CSelftestPart_Fan::state_start() {
#if HAS_TOOLCHANGER()
    if (!prusa_toolchanger.is_tool_enabled(config.tool_nr)) {
        start_time = end_time = SelftestInstance().GetTime();
        result.print_fan_state = SelftestSubtestState_t::undef;
        result.heatbreak_fan_state = SelftestSubtestState_t::undef;
        result.fans_switched_state = SelftestSubtestState_t::undef;
        return LoopResult::Abort;
    }
#endif

    log_info(Selftest, "Fan test %u started", config.tool_nr);

    result.print_fan_state = SelftestSubtestState_t::running;
    result.heatbreak_fan_state = SelftestSubtestState_t::running;

    start_time = SelftestInstance().GetTime();
    end_time = start_time + estimate();

    print_fan.set_pwm(pwm_100_percent);
    heatbreak_fan.set_pwm(pwm_100_percent);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm_100_percent() {
    if (state_machine.IsInState_ms() <= state_wait_rpm_100_percent_delay) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    heatbreak_fan.reset_samples();
    print_fan.reset_samples();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_measure_rpm_100_percent() {
    if (state_machine.IsInState_ms() <= state_measure_rpm_100_percent_delay) {
        print_fan.record_sample();
        heatbreak_fan.record_sample();
        update_progress();
        return LoopResult::RunCurrent;
    }

    const uint16_t print_fan_rpm = print_fan.calculate_avg_rpm();
    print_fan.evaluate(config.print_fan, print_fan_rpm);
    if (print_fan.is_failed()) {
        result.print_fan_state = SelftestSubtestState_t::not_good;
    }

    const uint16_t heatbreak_fan_rpm = heatbreak_fan.calculate_avg_rpm();
    heatbreak_fan.evaluate(config.heatbreak_fan, heatbreak_fan_rpm);
    if (heatbreak_fan.is_failed()) {
        result.heatbreak_fan_state = SelftestSubtestState_t::not_good;
    }

    if (print_fan.is_failed() && heatbreak_fan.is_failed()) {
        // try if the rpms fit into the ranges when switched, if yes, fail the
        // "fans switched" test and pass the RPM tests
        if (is_rpm_within_bounds(config.heatbreak_fan, print_fan_rpm) && is_rpm_within_bounds(config.print_fan, heatbreak_fan_rpm)) {
            log_error(Selftest, "Fans test %u print and hotend fan appear to be switched (the RPM of each fits into the range of the other)", config.tool_nr);
            result.print_fan_state = SelftestSubtestState_t::ok;
            result.heatbreak_fan_state = SelftestSubtestState_t::ok;
            result.fans_switched_state = SelftestSubtestState_t::not_good;
            return LoopResult::Fail;
        }
    }

    if (!print_fan.is_failed() && !heatbreak_fan.is_failed()) {
        result.fans_switched_state = SelftestSubtestState_t::ok;
    }

    print_fan.set_pwm(0);
    heatbreak_fan.set_pwm(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm_0_percent() {
    if (state_machine.IsInState_ms() <= state_wait_rpm_0_percent_delay) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.set_pwm(pwm_20_percent);
    heatbreak_fan.set_pwm(pwm_20_percent);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm_20_percent() {
    if (state_machine.IsInState_ms() <= state_wait_rpm_20_percent_delay) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.reset_samples();
    heatbreak_fan.reset_samples();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_measure_rpm_20_percent() {
    if (state_machine.IsInState_ms() <= state_measure_rpm_20_percent_delay) {
        print_fan.record_sample();
        heatbreak_fan.record_sample();
        update_progress();
        return LoopResult::RunCurrent;
    }

    const uint16_t print_fan_rpm = print_fan.calculate_avg_rpm();
    print_fan.evaluate(benevolent_fan_config, print_fan_rpm);
    if (print_fan.is_failed()) {
        result.print_fan_state = SelftestSubtestState_t::not_good;
    }

    const uint16_t heatbreak_fan_rpm = heatbreak_fan.calculate_avg_rpm();
    heatbreak_fan.evaluate(benevolent_fan_config, heatbreak_fan_rpm);
    if (heatbreak_fan.is_failed()) {
        result.heatbreak_fan_state = SelftestSubtestState_t::not_good;
    }

    if (!print_fan.is_failed()) {
        result.print_fan_state = SelftestSubtestState_t::ok;
    }
    if (!heatbreak_fan.is_failed()) {
        result.heatbreak_fan_state = SelftestSubtestState_t::ok;
    }

    if (result.print_fan_state == SelftestSubtestState_t::not_good || result.heatbreak_fan_state == SelftestSubtestState_t::not_good || result.fans_switched_state == SelftestSubtestState_t::not_good) {
        return LoopResult::Fail;
    }

    return LoopResult::RunNext;
}

void CSelftestPart_Fan::update_progress() {
    if (start_time == end_time) {
        return; // don't have estimated end set correctly
    }

    result.progress = static_cast<uint8_t>(scale_percent(SelftestInstance().GetTime(), start_time, end_time));
}
