// selftest_fan.cpp

#include "selftest_fan.h"
#include "common/conversions.hpp"
#include <guiconfig/wizard_config.hpp>
#include "fanctl.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <config_store/store_instance.hpp>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

namespace {

constexpr std::uint8_t percentage_to_pwm(std::uint8_t target_percentage) {
    return val_mapping(true, target_percentage, 100, 255);
}

// Start at 100% and wait longer to allow spreading stuck lubricant after first assembly.
// Then set PWM to 0% and wait for quite a long time to ensure fan stopped.
// (We can't measure RPM without at least some PWM)
// Then continue with 40% PWM to test if 40% is enough to start spinning.
constexpr uint8_t pwm_100_percent = percentage_to_pwm(100);
constexpr uint8_t pwm_40_percent = percentage_to_pwm(40);
constexpr uint32_t state_measure_rpm_delay = 5000;
constexpr uint32_t state_wait_rpm_100_percent_delay = 6000;
constexpr uint32_t state_measure_rpm_100_percent_delay = state_measure_rpm_delay;
constexpr uint32_t state_wait_rpm_0_percent_delay = 10000;
constexpr uint32_t state_wait_rpm_40_percent_delay = 3000;
constexpr uint32_t state_measure_rpm_40_percent_delay = state_measure_rpm_delay;

} // namespace

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

FanHandler::FanHandler(const char *name, const FanCtlFnc &fanctl_fnc, uint8_t tool_nr)
    : name(name)
    , fanctl_fnc(fanctl_fnc)
    , tool_nr(tool_nr) {}

void FanHandler::enter_selftest() {
    fanctl_fnc(tool_nr).enterSelftestMode();
}

void FanHandler::exit_selftest() {
    fanctl_fnc(tool_nr).exitSelftestMode();
}

void FanHandler::set_pwm(uint8_t pwm) {
    fanctl_fnc(tool_nr).selftestSetPWM(pwm);
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
    , print_fan("Print fan", &Fans::print, config.tool_nr)
    , heatbreak_fan("Heatbreak fan", &Fans::heat_break, config.tool_nr) {
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
        + state_wait_rpm_40_percent_delay
        + state_measure_rpm_40_percent_delay;
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

#if PRINTER_IS_PRUSA_MK3_5
    if (heatbreak_fan_rpm > 6000 || print_fan_rpm > 6000) {
        // this rpm is unreachable by noctua therefore the fans are a lot faster and pwm fix is needed to make printer quiet
        // check both fans because they could be switched.
        config_store().has_alt_fans.set(true);
    } else {
        config_store().has_alt_fans.set(false);
    }

    // Create config specifically for alt fans presence of which cannot be done compile-time.
    SelftestFansConfig alt_config { .print_fan = { .rpm_min = 3000, .rpm_max = 4500 }, .heatbreak_fan = { .rpm_min = 7000, .rpm_max = 10000 } };

    if (config_store().has_alt_fans.get()) {
        print_fan.evaluate(alt_config.print_fan, print_fan_rpm);
        heatbreak_fan.evaluate(alt_config.heatbreak_fan, heatbreak_fan_rpm);
        if (CSelftestPart_Fan::are_fans_switched(print_fan, heatbreak_fan, alt_config, print_fan_rpm, heatbreak_fan_rpm, result)) {
            return LoopResult::Fail;
        } else if (!heatbreak_fan.is_failed() && !print_fan.is_failed()) {
            result.fans_switched_state = SelftestSubtestState_t::ok;
        }
    }
#else
    if (CSelftestPart_Fan::are_fans_switched(print_fan, heatbreak_fan, config, print_fan_rpm, heatbreak_fan_rpm, result)) {
        return LoopResult::Fail;
    }

    if (!print_fan.is_failed() && !heatbreak_fan.is_failed()) {
        result.fans_switched_state = SelftestSubtestState_t::ok;
    }
#endif
    return LoopResult::RunNext;
}

bool CSelftestPart_Fan::are_fans_switched(const FanHandler &print_fan, const FanHandler &heatbreak_fan, const SelftestFansConfig &config, const uint16_t print_fan_rpm, const uint16_t heatbreak_fan_rpm, SelftestFanHotendResult &result) {
    if (print_fan.is_failed() && heatbreak_fan.is_failed()) {
        // try if the rpms fit into the ranges when switched, if yes, fail the
        // "fans switched" test and pass the RPM tests
        if (is_rpm_within_bounds(config.heatbreak_fan, print_fan_rpm) && is_rpm_within_bounds(config.print_fan, heatbreak_fan_rpm)) {
            log_error(Selftest, "Fans test %u print and hotend fan appear to be switched (the RPM of each fits into the range of the other)", config.tool_nr);
            // Since fans switched isn't the last check, it cannot tell whether the fans are ok or not. All that is certain at this point is that they are switched. They still can fail on 20 % test.
            result.print_fan_state = SelftestSubtestState_t::undef;
            result.heatbreak_fan_state = SelftestSubtestState_t::undef;
            result.fans_switched_state = SelftestSubtestState_t::not_good;
            return true;
        }
    }
    return false;
}

#if PRINTER_IS_PRUSA_MK3_5
LoopResult CSelftestPart_Fan::state_manual_check_init() {
    if (result.fans_switched_state != SelftestSubtestState_t::ok) {
        print_fan.set_pwm(0); // stop print fan since heatbreak is the critical one
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_manual_check_wait_fan() {
    if (result.fans_switched_state != SelftestSubtestState_t::ok) {
        if (state_machine.IsInState_ms() <= state_wait_rpm_0_percent_delay) {
            update_progress();
            return LoopResult::RunCurrent;
        }
        IPartHandler::SetFsmPhase(PhasesSelftest::Fans_manual);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_manual_check_ask() {
    if (result.fans_switched_state != SelftestSubtestState_t::ok) {
        const auto response { state_machine.GetButtonPressed() };
        switch (response) {
        case Response::No:
            result.fans_switched_state = SelftestSubtestState_t::not_good;
            return LoopResult::Fail;
        case Response::Yes:
            result.fans_switched_state = SelftestSubtestState_t::ok;
            break;
        default:
            return LoopResult::RunCurrent;
        }
    }
    IPartHandler::SetFsmPhase(PhasesSelftest::Fans_second);
    return LoopResult::RunNext;
}

#endif

LoopResult
CSelftestPart_Fan::state_rpm_0_init() {
    print_fan.set_pwm(0);
    heatbreak_fan.set_pwm(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm_0_percent() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Fans_second);

    if (state_machine.IsInState_ms() <= state_wait_rpm_0_percent_delay) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.set_pwm(pwm_40_percent);
    heatbreak_fan.set_pwm(pwm_40_percent);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_wait_rpm_40_percent() {
    if (state_machine.IsInState_ms() <= state_wait_rpm_40_percent_delay) {
        update_progress();
        return LoopResult::RunCurrent;
    }

    print_fan.reset_samples();
    heatbreak_fan.reset_samples();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::state_measure_rpm_40_percent() {
    if (state_machine.IsInState_ms() <= state_measure_rpm_40_percent_delay) {
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
#if PRINTER_IS_PRUSA_MK3_5
    // Time update is necessary because of possible human interaction which causes unpredictable delay
    if (IPartHandler::GetFsmPhase() != PhasesSelftest::Fans_second) {
        end_time = SelftestInstance().GetTime() + state_wait_rpm_0_percent_delay
            + state_wait_rpm_40_percent_delay
            + state_measure_rpm_40_percent_delay;
    }
#endif
    if (start_time == end_time) {
        return; // don't have estimated end set correctly
    }

    result.progress = static_cast<uint8_t>(scale_percent(SelftestInstance().GetTime(), start_time, end_time));
}
