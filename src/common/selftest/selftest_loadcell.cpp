/**
 * @file selftest_loadcell.cpp
 * @author Radek Vana
 * @copyright Copyright (c) 2021
 */

#include "selftest_loadcell.h"
#include <guiconfig/wizard_config.hpp>
#include "marlin_server.hpp"
#include "selftest_log.hpp"
#include "loadcell.hpp"
#include <sound.hpp>
#include <module/temperature.h>
#include <module/planner.h>
#include <module/stepper.h>
#include <module/endstops.h>
#include <gcode/gcode.h>
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <climits>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

CSelftestPart_Loadcell::CSelftestPart_Loadcell(IPartHandler &state_machine, const LoadcellConfig_t &config,
    SelftestLoadcell_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , currentZ(0.F)
    , targetZ(0.F)
    , begin_target_temp(thermalManager.degTargetHotend(rConfig.tool_nr))
    , time_start(SelftestInstance().GetTime())
    , log(1000)
    , log_fast(100) // this is only during 1s (will generate 9-10 logs)
{
    thermalManager.setTargetHotend(0, rConfig.tool_nr);
    endstops.enable(true);
    log_info(Selftest, "%s Started", rConfig.partname);
}

CSelftestPart_Loadcell::~CSelftestPart_Loadcell() {
    thermalManager.setTargetHotend(begin_target_temp, rConfig.tool_nr);
    endstops.enable(false);
}

LoopResult CSelftestPart_Loadcell::stateMoveUpInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_move_away);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateMoveUp() {
    planner.synchronize(); // finish current move (there should be none)
    endstops.validate_homing_move();

    set_current_from_steppers();
    sync_plan_position();

    // Disable stealthChop if used. Enable diag1 pin on driver.
#if ENABLED(SENSORLESS_HOMING)
    start_sensorless_homing_per_axis(AxisEnum::Z_AXIS);
#endif

    currentZ = current_position.z;
    targetZ = rConfig.z_extra_pos;
    if (targetZ > currentZ) {
        log_info(Selftest, "%s move up, target: %f current: %f", rConfig.partname, double(targetZ), double(currentZ));
        current_position.z = rConfig.z_extra_pos;
        line_to_current_position(rConfig.z_extra_pos_fr);
    } else {
        log_info(Selftest, "%s move up not needed, target: %f > current: %f", rConfig.partname, double(targetZ), double(currentZ));
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateMoveUpWaitFinish() {
    if (planner.processing()) {
        currentZ = current_position.z;
        return LoopResult::RunCurrent;
    }
    log_info(Selftest, "%s move up finished", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateCooldownInit() {
    thermalManager.setTargetHotend(0, rConfig.tool_nr); // Disable heating for tested hotend
    marlin_server::set_temp_to_display(0, rConfig.tool_nr);
    const float temp = thermalManager.degHotend(rConfig.tool_nr);
    rResult.temperature = static_cast<int16_t>(temp);
    need_cooling = temp > rConfig.cool_temp; // Check if temperature is safe
    if (need_cooling) {
#if HAS_TOOLCHANGER()
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            if (!axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS))) {
                // Nozzle is hot and axes are known, park it and don't let user touch it
                marlin_server::enqueue_gcode_printf("P0 S1");
            }
            // else we would have to home near user which defeats the purpose of hiding the nozzle
        }
#endif /*HAS_TOOLCHANGER()*/
        IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_cooldown);
        log_info(Selftest, "%s cooling needed, target: %d current: %f", rConfig.partname,
            static_cast<int>(rConfig.cool_temp), static_cast<double>(temp));
        rConfig.print_fan_fnc(rConfig.tool_nr).enterSelftestMode();
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).enterSelftestMode();
        rConfig.print_fan_fnc(rConfig.tool_nr).selftestSetPWM(255); // it will be restored by exitSelftestMode
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).selftestSetPWM(255); // it will be restored by exitSelftestMode
        log_info(Selftest, "%s fans set to maximum", rConfig.partname);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateCooldown() {
    const float temp = thermalManager.degHotend(rConfig.tool_nr);
    rResult.temperature = static_cast<int16_t>(temp);

    // still cooling
    // Check need_cooling and skip in case we didn't show PhasesSelftest::Loadcell_cooldown.
    if (need_cooling && temp > rConfig.cool_temp) {
        LogInfoTimed(log, "%s cooling down, target: %d current: %f", rConfig.partname,
            static_cast<int>(rConfig.cool_temp), static_cast<double>(temp));
        gcode.reset_stepper_timeout(); // Do not disable steppers while cooling
        return LoopResult::RunCurrent;
    }

    log_info(Selftest, "%s cooled down", rConfig.partname);
    return LoopResult::RunNext; // cooled
}

LoopResult CSelftestPart_Loadcell::stateCooldownDeinit() {
    if (need_cooling) { // if cooling was needed, return control of fans
        rConfig.print_fan_fnc(rConfig.tool_nr).exitSelftestMode();
        rConfig.heatbreak_fan_fnc(rConfig.tool_nr).exitSelftestMode();
        log_info(Selftest, "%s fans disabled", rConfig.partname);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateToolSelectInit() {
    if (active_extruder != rConfig.tool_nr) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_tool_select);

        marlin_server::enqueue_gcode_printf("T%d S1 L0 D0", rConfig.tool_nr);

        // go to some reasonable position
        // Use reasonable feedrate as it was likely set by previous Z move
        marlin_server::enqueue_gcode_printf("G0 X50 Y50 F%d", XY_PROBE_SPEED_INITIAL);
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
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_ask_abort);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateAskAbort() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort: // Abort is automatic at state machine level, it should never get here
        log_error(Selftest, "%s user pressed abort, code should not reach this place", rConfig.partname);
        return LoopResult::Abort;
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

    time_start_countdown = SelftestInstance().GetTime();
    rResult.countdown = SelftestLoadcell_t::countdown_undef;
    rResult.pressed_too_soon = true;
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_countdown);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateTapCheckCountDown() {
    int32_t load = -1 * loadcell.get_tared_z_load(); // Positive when pushing the nozzle up
    // Show tared value at 1/10 of the range, threshold tap_min_load_ok is needed to pass the test
    rResult.progress = scale_percent_avoid_overflow(load, rConfig.tap_min_load_ok / -9, rConfig.tap_min_load_ok);
    if (std::abs(load) >= rConfig.countdown_load_error_value) {
        log_info(Selftest, "%s load during countdown %dg exceeded error value %dg", rConfig.partname,
            static_cast<int>(load), static_cast<int>(rConfig.countdown_load_error_value));
        rResult.pressed_too_soon = true;
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
    rResult.countdown = SelftestLoadcell_t::countdown_undef;
    time_start_tap = SelftestInstance().GetTime();
    IPartHandler::SetFsmPhase(PhasesSelftest::Loadcell_user_tap_check);
    Sound_Play(eSOUND_TYPE::SingleBeepAlwaysLoud);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Loadcell::stateTapCheck() {
    if ((SelftestInstance().GetTime() - time_start_tap) >= rConfig.tap_timeout_ms) {
        log_info(Selftest, "%s user did not tap", rConfig.partname);
        return LoopResult::GoToMark0; // timeout, retry entire touch sequence
    }

    int32_t load = -1 * loadcell.get_tared_z_load(); // Positive when pushing the nozzle up
    bool pass = IsInClosedRange(load, rConfig.tap_min_load_ok, rConfig.tap_max_load_ok);
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
    return LoopResult::RunNext;
    rResult.progress = 100;
}
