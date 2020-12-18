/**
 * @file safety_timer.cpp
 * @author Radek Vana
 * @date 2020-12-15
 */
#include "../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../lib/Marlin/Marlin/src/module/temperature.h"
#include "safety_timer_stubbed.hpp"
#include "pause_stubbed.hpp"
#include "marlin_server.hpp"

SafetyTimer &SafetyTimer::Instance() {
    static SafetyTimer ret;
    return ret;
}

SafetyTimer::SafetyTimer()
    : interval(default_interval)
    , last_reset(0)
    , knob_moves(0)
    , knob_clicks(0) {
}

void SafetyTimer::ReInit() {
    interval = default_interval;
    last_reset = millis();
}

void SafetyTimer::SetInterval(millis_t ms) {
    interval = ms;
    last_reset = ms;
}

SafetyTimer::expired_t SafetyTimer::Loop() {
    millis_t now = millis();
    uint32_t move_dif = marlin_server_get_user_move_count() - knob_moves;
    uint32_t click_dif = marlin_server_get_user_click_count() - knob_clicks;
    knob_moves = marlin_server_get_user_move_count();
    knob_clicks = marlin_server_get_user_click_count();

    Pause &pause = Pause::Instance();

    // auto restores temp only on click (ignore move),
    // it is also restored by Pause on any phase change
    // cannot guarante that SafetyTimer will happen first, so have to do it on both places
    if (click_dif && pause.HasTempToRestore()) {
        pause.RestoreTemp();
        last_reset = now;
        return expired_t::no;
    }

    if (!interval || move_dif || click_dif || !anyHeatherIsActive() || printingIsActive() || !pause.CanSafetyTimerExpire()) {
        last_reset = now;
        return expired_t::no;
    }

    if (!ELAPSED(now, last_reset + interval)) {
        //timer is counting, but did not reach last_reset yet
        return expired_t::no;
    }

    //timer is expired
    pause.NotifyExpiredFromSafetyTimer(thermalManager.degTargetHotend(0), thermalManager.degTargetBed());
    if (printingIsPaused()) {
        // disable only nozzle
        thermalManager.disable_hotend();
        set_warning(WarningType::NozzleTimeout);
    } else {
        // disable nozzle and bed
        thermalManager.disable_all_heaters();
        set_warning(WarningType::HeatersTimeout);
    }

    return expired_t::yes;
}

//marlin compatibility function
void safety_timer_set_interval(millis_t ms) {
    SafetyTimer::Instance().SetInterval(ms);
}
