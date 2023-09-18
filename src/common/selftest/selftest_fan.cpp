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

#define FANTEST_STOP_DELAY    2000
#define FANTEST_WAIT_DELAY    3000
#define FANTEST_MEASURE_DELAY 5000

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

FanHandler::FanHandler(const char *name, const FanConfig<2> &config, uint8_t tool_nr)
    : name(name)
    , config(config)
    , tool_nr(tool_nr) {}

void FanHandler::enter_selftest() {
    config.fanctl_fnc(tool_nr).EnterSelftestMode();
}

void FanHandler::exit_selftest() {
    config.fanctl_fnc(tool_nr).ExitSelftestMode();
}

void FanHandler::set_pwm(uint8_t pwm) {
    config.fanctl_fnc(tool_nr).SelftestSetPWM(pwm);
}

uint8_t FanHandler::get_pwm() {
    return config.fanctl_fnc(tool_nr).getPWM();
}

uint8_t FanHandler::get_pwm_percent() {
    return scale_percent(config.fanctl_fnc(tool_nr).getPWM(), uint8_t(0), config.fanctl_fnc(tool_nr).getMaxPWM());
}

uint16_t FanHandler::get_actual_rpm() {
    return config.fanctl_fnc(tool_nr).getActualRPM();
}

void FanHandler::record_sample() {
    if (failed) {
        return;
    }

    sample_count++;
    sample_sum += get_actual_rpm();
}

void FanHandler::next_step() {
    if (failed) {
        return;
    }

    sample_count = 0;
    sample_sum = 0;

    set_pwm(get_pwm() + config.pwm_step);
}

void FanHandler::evaluate(uint8_t step) {
    if (failed) {
        return;
    }

    avg_rpm = sample_sum / sample_count;

    SelftestInstance().log_printf("%s %u at %u%% PWM = %u RPM\n", name, tool_nr, get_pwm_percent(), avg_rpm);
    log_info(Selftest, "%s %u pwm: %u rpm: %u", name, tool_nr, get_pwm(), avg_rpm);

    // N.B. Law of trichotomy ensures the check is skipped if we set min == max
    //      This is used in MK3.5 "fine fan test".
    if ((avg_rpm < config.rpm_min_table[step]) || (avg_rpm > config.rpm_max_table[step])) {
        SelftestInstance().log_printf("%s %u %u RPM out of range (%u - %u)\n",
            name,
            tool_nr,
            avg_rpm,
            config.rpm_min_table[step],
            config.rpm_max_table[step]);

        log_error(Selftest, "%s %u %u RPM out of range (%u - %u)",
            name,
            tool_nr,
            avg_rpm,
            config.rpm_min_table[step],
            config.rpm_max_table[step]);

        if (step == config.rpm_min_table.size() - 1) {
            failed_at_last_step = true;
        } else {
            failed = true;
        }
    }
}

CSelftestPart_Fan::CSelftestPart_Fan(IPartHandler &state_machine, const SelftestFansConfig &config,
    SelftestFanHotendResult &result)
    : state_machine(state_machine)
    , config(config)
    , result(result)
    , print_fan("Print fan", config.print_fan, config.tool_nr)
    , heatbreak_fan("Heatbreak fan", config.heatbreak_fan, config.tool_nr) {
    print_fan.enter_selftest();
    heatbreak_fan.enter_selftest();
}

CSelftestPart_Fan::~CSelftestPart_Fan() {
    print_fan.exit_selftest();
    heatbreak_fan.exit_selftest();
}

uint32_t CSelftestPart_Fan::estimate(const SelftestFansConfig &config) {
    uint32_t total_time = FANTEST_STOP_DELAY + config.steps * (FANTEST_WAIT_DELAY + FANTEST_MEASURE_DELAY);
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
    SelftestInstance().log_printf("Fan test %u started\n", config.tool_nr);

    if (print_fan.get_actual_rpm() == 0 && heatbreak_fan.get_actual_rpm() == 0) {
        // no need to wait for spindown
        wait_for_spindown = false;
        end_time -= FANTEST_STOP_DELAY;
    }

    result.print_fan_state = SelftestSubtestState_t::running;
    result.heatbreak_fan_state = SelftestSubtestState_t::running;

    start_time = SelftestInstance().GetTime();
    end_time = start_time + estimate(config);

    print_fan.set_pwm(0);
    heatbreak_fan.set_pwm(0);

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_spindown() {
    if (wait_for_spindown && state_machine.IsInState_ms() <= FANTEST_STOP_DELAY) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.set_pwm(config.print_fan.pwm_start);
    heatbreak_fan.set_pwm(config.heatbreak_fan.pwm_start);

    log_info(Selftest, "Fan test %u waited for spindown", config.tool_nr);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm() {
    if (state_machine.IsInState_ms() <= FANTEST_WAIT_DELAY) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_measure_rpm() {
    if (state_machine.IsInState_ms() <= FANTEST_MEASURE_DELAY) {
        print_fan.record_sample();
        heatbreak_fan.record_sample();
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.evaluate(step);
    if (print_fan.is_failed()) {
        result.print_fan_state = SelftestSubtestState_t::not_good;
    }

    heatbreak_fan.evaluate(step);
    if (heatbreak_fan.is_failed()) {
        result.heatbreak_fan_state = SelftestSubtestState_t::not_good;
    }

    if (++step < config.steps) {
        print_fan.next_step();
        heatbreak_fan.next_step();

        return LoopResult::GoToMark0;
    }

    if (!print_fan.is_failed()) {
        result.print_fan_state = SelftestSubtestState_t::ok;
    }
    if (!heatbreak_fan.is_failed()) {
        result.heatbreak_fan_state = SelftestSubtestState_t::ok;
    }
    if (!print_fan.is_failed() && !heatbreak_fan.is_failed()) {
        result.fans_switched_state = SelftestSubtestState_t::ok;
    }

    if (print_fan.is_failed_at_last_step() && heatbreak_fan.is_failed_at_last_step()) {
        // try if the rpms fit into the ranges when switched, if yes, fail the
        // "fans switched" test and pass the RPM tests
        if (print_fan.get_avg_rpm() > *config.heatbreak_fan.rpm_min_table.rbegin() && print_fan.get_avg_rpm() < *config.heatbreak_fan.rpm_max_table.rbegin() && heatbreak_fan.get_avg_rpm() > *config.print_fan.rpm_min_table.rbegin() && heatbreak_fan.get_avg_rpm() < *config.print_fan.rpm_max_table.rbegin()) {
            log_error(Selftest, "Fans test %u print and hotend fan appear to be switched (the RPM of each fits into the range of the other)", config.tool_nr);
            result.print_fan_state = SelftestSubtestState_t::ok;
            result.heatbreak_fan_state = SelftestSubtestState_t::ok;
            result.fans_switched_state = SelftestSubtestState_t::not_good;
        }
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
