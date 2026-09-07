/// @file
#include "all_tools_quick_cooling_manager.hpp"

#include <Marlin/src/module/temperature.h>
#include <fanctl.hpp>
#include <marlin_server.hpp>

// Fans engage only this much above the target temperature
// Prevents fan flapping when the nozzles regulate right at the target
static constexpr float engage_hysteresis = 5;

AllToolsQuickCoolingManager::AllToolsQuickCoolingManager(float target_temp)
    : target_temp(target_temp)
    , idle_hook(marlin_server::idle_publisher, [this] { step(); }) {}

void AllToolsQuickCoolingManager::set_active(bool set) {
    active = set;
    step();
}

void AllToolsQuickCoolingManager::step() {
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        const float temp = thermalManager.degHotend(tool);
        // both fans are always switched together.
        const bool cooling = Fans::print(tool).is_selftest();

        if (active && !cooling && temp > target_temp + engage_hysteresis) {
            Fans::print(tool).enter_selftest_mode();
            Fans::heat_break(tool).enter_selftest_mode();
            Fans::print(tool).selftest_set_pwm(255);
            Fans::heat_break(tool).selftest_set_pwm(255);

        } else if (cooling && (!active || temp <= target_temp)) {
            Fans::print(tool).exit_selftest_mode();
            Fans::heat_break(tool).exit_selftest_mode();
        }
    }
}
