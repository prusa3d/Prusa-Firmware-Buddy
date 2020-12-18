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
    , reset_treshold(0) {
}

void SafetyTimer::ReInit() {
    interval = default_interval;
    reset_treshold = millis();
}

void SafetyTimer::SetInterval(millis_t ms) {
    interval = ms;
    reset_treshold = ms;
}

SafetyTimer::expired_t SafetyTimer::Loop() {
    millis_t now = millis();
    if (!interval)
        return expired_t::no;

    if (!anyHeatherIsActive() || printingIsActive() || !Pause::Instance().CanSafetyTimerExpire()) {
        reset_treshold = now;
        return expired_t::no;
    }

    if (!ELAPSED(now, reset_treshold + interval)) {
        //timer is counting, but did not reach reset_treshold yet
        return expired_t::no;
    }

    //timer is expired
    Pause::Instance().NotifyExpiredFromSafetyTimer(thermalManager.degTargetHotend(0), thermalManager.degTargetBed());
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
