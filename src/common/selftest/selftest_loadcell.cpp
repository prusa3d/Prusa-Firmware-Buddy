/**
 * @file selftest_loadcell.cpp
 * @author Radek Vana
 * @copyright Copyright (c) 2021
 */

#include "selftest_loadcell.h"
#include "config_features.h" //EXTRUDER_AUTO_loadcell_TEMPERATURE
#include "marlin_server.hpp"
#include "selftest_log.hpp"
#include "loadcell.hpp"
#include <sound.hpp>
#include <mapi/parking.hpp>
#include <module/temperature.h>
#include <module/endstops.h>
#include <feature/motordriver_util.h>
#include <gcode/gcode.h>
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <climits>
#include <sensor_data.hpp>

#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

static constexpr int32_t acceptable_noise_range_g = 60;

CSelftestPart_Loadcell::CSelftestPart_Loadcell(IPartHandler &state_machine, const LoadcellConfig_t &config,
    SelftestLoadcell_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , begin_target_temp(Hotend::for_tool(rConfig.tool_nr).nozzle_target_temp())
    , time_start(SelftestInstance().GetTime())
    , log(1000)
    , log_fast(100) // this is only during 1s (will generate 9-10 logs)
{
    Hotend::for_tool(rConfig.tool_nr).set_nozzle_target_temp(0);
    endstops.enable(true);
    log_info(Selftest, "%s Started", rConfig.partname);
}

CSelftestPart_Loadcell::~CSelftestPart_Loadcell() {
    Hotend::for_tool(rConfig.tool_nr).set_nozzle_target_temp(begin_target_temp);
    endstops.enable(false);
}

LoopResult CSelftestPart_Loadcell::stateParkingInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_move_away);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::statePrepareParking() {
    planner.synchronize(); // finish current move (there should be none)
    endstops.validate_homing_move();

    set_current_from_steppers();
    sync_plan_position();

    // Disable stealthChop if used. Enable diag1 pin on driver.
#if ENABLED(SENSORLESS_HOMING)
    start_sensorless_homing_per_axis(AxisEnum::Z_AXIS);
#endif
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateParking() {
    log_info(Selftest, "%s, parking", rConfig.partname);

    if (rConfig.z_extra_pos > current_position.z) {
        // Z move might hit the end of the axis
        current_position.z = rConfig.z_extra_pos;
        line_to_current_position(rConfig.z_extra_pos_fr);
        planner.synchronize();
    }

    mapi::home_if_needed_and_park(mapi::get_parking_position(mapi::ParkPosition::loadcell_selftest).without_z_move());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateCooldownInit() {
    Hotend::for_tool(rConfig.tool_nr).set_nozzle_target_temp(0); // Disable heating for tested hotend
    const auto temp_opt = Hotend::for_tool(rConfig.tool_nr).nozzle_temp();
    if (!temp_opt.has_value()) {
        // No reading yet — wait until the next manage tick.
        return LoopResult::RunCurrent;
    }
    const float temp = temp_opt.value();
    rResult.temperature = static_cast<int16_t>(temp);
    need_cooling = temp > rConfig.cool_temp; // Check if temperature is safe
    if (need_cooling) {
#if HAS_TOOLCHANGER() && !HAS_INDX()
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            if (!axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS))) {
                // Nozzle is hot and axes are known, park it and don't let user touch it
                marlin_server::enqueue_gcode_printf("P0 S1");
            }
            // else we would have to home near user which defeats the purpose of hiding the nozzle
        }
#endif
        IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_cooldown);
        log_info(Selftest, "%s cooling needed, target: %d current: %f", rConfig.partname,
            static_cast<int>(rConfig.cool_temp), static_cast<double>(temp));
        rConfig.print_fan_fnc(rConfig.tool_nr).enter_selftest_mode();
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).enter_selftest_mode();
        rConfig.print_fan_fnc(rConfig.tool_nr).selftest_set_pwm(255); // it will be restored by exitSelftestMode
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).selftest_set_pwm(255); // it will be restored by exitSelftestMode
        log_info(Selftest, "%s fans set to maximum", rConfig.partname);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateCooldown() {
    const auto temp_opt = Hotend::for_tool(rConfig.tool_nr).nozzle_temp();
    if (!temp_opt.has_value()) {
        // No reading yet — wait until the next manage tick.
        return LoopResult::RunCurrent;
    }
    const float temp = temp_opt.value();
    rResult.temperature = static_cast<int16_t>(temp);

    // still cooling
    // Check need_cooling and skip in case we didn't show PhasesSelftest::Loadcell_cooldown.
    if (need_cooling && temp > rConfig.cool_temp) {
        LogInfoTimed(log, "%s cooling down, target: %d current: %f", rConfig.partname,
            static_cast<int>(rConfig.cool_temp), static_cast<double>(temp));
        return LoopResult::RunCurrent;
    }

    log_info(Selftest, "%s cooled down", rConfig.partname);
    return LoopResult::RunNext; // cooled
}

LoopResult CSelftestPart_Loadcell::stateCooldownDeinit() {
    if (need_cooling) { // if cooling was needed, return control of fans
        rConfig.print_fan_fnc(rConfig.tool_nr).exit_selftest_mode();
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).exit_selftest_mode();
        log_info(Selftest, "%s fans disabled", rConfig.partname);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateToolSelectInit() {
    if (!stdext::holds_value(PhysicalToolIndex::currently_selected(), rConfig.tool_nr)) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_tool_select);

        marlin_server::enqueue_gcode_printf("T%d S1 L0 D0", rConfig.tool_nr.to_raw());

        // go to some reasonable position
        // Use reasonable feedrate as it was likely set by previous Z move
        float pos[] = XYZ_LOADCELL_SELFTEST_POINT;
        auto x = static_cast<int>(pos[0]);
        auto y = static_cast<int>(pos[1]);
        marlin_server::enqueue_gcode_printf("G0 X%d Y%d F%d", x, y, XY_PROBE_SPEED_INITIAL);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateToolSelectWaitFinish() {
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

// disconnected sensor -> raw_load == undefined_value
// test rely on hw being unstable, raw_load must be different from undefined_value at least once during test period
LoopResult CSelftestPart_Loadcell::stateConnectionCheck() {
    int32_t raw_load = loadcell.get_raw_value();
    if (raw_load == Loadcell::undefined_value) {
        log_error(Selftest, "%s returned undefined_value", rConfig.partname);
        IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_fail);
        return LoopResult::Fail;
    }

    const uint32_t timestamp1 = loadcell.GetLastSampleTimeUs();
    osDelay(200); // wait for some samples
    const uint32_t timestamp2 = loadcell.GetLastSampleTimeUs();
    const bool loadcell_is_processing_samples = timestamp1 != timestamp2;

    if (!loadcell_is_processing_samples || raw_load == 0) {
        if ((SelftestInstance().GetTime() - time_start) > rConfig.max_validation_time) {
            log_error(Selftest, "%s invalid", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_fail);
            return LoopResult::Fail;
        } else {
            log_debug(Selftest, "%s data not ready", rConfig.partname);
            return LoopResult::RunCurrent;
        }
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateAskAbortInit() {
    const bool show_ignore = rResult.loadcell_noisy && !rResult.ignore_noisy;
    IPartHandler::SetFsmPhase(show_ignore ? PhasesSelftest::Loadcell_user_tap_ask_ignore_abort : PhasesSelftest::Loadcell_user_tap_ask_abort);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateAskAbort() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort: // Abort is automatic at state machine level, it should never get here
        log_error(Selftest, "%s user pressed abort, code should not reach this place", rConfig.partname);
        return LoopResult::Abort;
    case Response::Ignore:
        log_error(Selftest, "%s user selected to ignore loadcell noise", rConfig.partname);
        rResult.ignore_noisy = true;
        return LoopResult::RunNext;
    case Response::Continue:
        log_info(Selftest, "%s user pressed continue", rConfig.partname);
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_Loadcell::stateTapCheckCountDownInit() {
    // Enable high precision and take a reference tare
    loadcell_high_precision_enabler.emplace(loadcell);
    safe_delay(Z_FIRST_PROBE_DELAY);
    loadcell.WaitBarrier();
    loadcell.Tare(Loadcell::TareMode::Static);

    loadcell_value_range = {};
    time_start_countdown = SelftestInstance().GetTime();
    // Preserve the ignore noisy across retries.
    const bool ignore_noisy = rResult.ignore_noisy;
    rResult = { .ignore_noisy = ignore_noisy };

    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_countdown);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateTapCheckCountDown() {
    const int32_t load = -1 * static_cast<int32_t>(sensor_data().loadCell.load()); // Positive when pushing the nozzle up
    loadcell_value_range.min = std::min(loadcell_value_range.min, load);
    loadcell_value_range.max = std::max(loadcell_value_range.max, load);

    // Show tared value at 1/10 of the range, threshold tap_min_load_ok is needed to pass the test
    rResult.progress = scale_percent_avoid_overflow(load, rConfig.tap_min_load_ok / -9, rConfig.tap_min_load_ok);
    if (std::abs(load) >= rConfig.countdown_load_error_value) {
        log_info(Selftest, "%s load during countdown %dg exceeded error value %dg", rConfig.partname,
            static_cast<int>(load), static_cast<int>(rConfig.countdown_load_error_value));
        rResult.wrong_tap = true;
        return LoopResult::GoToMark0;
    }
    LogDebugTimed(log, "%s load during countdown %dg", rConfig.partname, static_cast<int>(load));

    uint32_t countdown_running_ms = SelftestInstance().GetTime() - time_start_countdown;
    uint8_t new_countdown = std::min(int32_t(countdown_running_ms / 1000), int32_t(rConfig.countdown_sec));
    new_countdown = rConfig.countdown_sec - new_countdown;

    rResult.countdown = new_countdown;

    if (countdown_running_ms >= rConfig.countdown_sec * 1000) {
        return LoopResult::RunNext;
    } else {
        return LoopResult::RunCurrent;
    }
}

LoopResult CSelftestPart_Loadcell::stateTapCheckInit() {
    if (loadcell_value_range.max - loadcell_value_range.min > acceptable_noise_range_g && !rResult.ignore_noisy) {
        log_info(Selftest, "%s range: %" PRIi32 "-%" PRIi32 " outside of %" PRIi32, rConfig.partname, loadcell_value_range.min, loadcell_value_range.max, acceptable_noise_range_g);
        rResult.loadcell_noisy = true;
        return LoopResult::GoToMark0;
    }

    rResult.countdown = SelftestLoadcell_t::countdown_undef;
    time_start_tap = SelftestInstance().GetTime();
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_check);
    sound::play(SoundType::single_beep_always_loud);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateTapCheck() {
    if ((SelftestInstance().GetTime() - time_start_tap) >= rConfig.tap_timeout_ms) {
        log_info(Selftest, "%s user did not tap", rConfig.partname);
        rResult.wrong_tap = true;
        return LoopResult::GoToMark0; // timeout, retry entire touch sequence
    }

    int32_t load = -1 * static_cast<int32_t>(sensor_data().loadCell.load()); // Positive when pushing the nozzle up
    bool pass = (load >= rConfig.tap_min_load_ok) && (load <= rConfig.tap_max_load_ok);
    if (pass) {
        log_info(Selftest, "%s tap check, load %dg successful in range <%d, %d>",
            rConfig.partname, static_cast<int>(load), static_cast<int>(rConfig.tap_min_load_ok),
            static_cast<int>(rConfig.tap_max_load_ok));
        return LoopResult::RunNext;
    }

    LogInfoTimed(log_fast, "%s tap check, load %dg not in range <%d, %d>",
        rConfig.partname, static_cast<int>(load), static_cast<int>(rConfig.tap_min_load_ok),
        static_cast<int>(rConfig.tap_max_load_ok));
    // Show tared value at 1/10 of the range, threshold tap_min_load_ok is needed to pass the test
    rResult.progress = scale_percent_avoid_overflow(load, rConfig.tap_min_load_ok / -9, rConfig.tap_min_load_ok);
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_Loadcell::stateTapOk() {
    loadcell_high_precision_enabler.reset();

    log_info(Selftest, "%s finished", rConfig.partname);
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_ok);
    rResult.progress = 100;
    return LoopResult::RunNext;
}
