#include "bed_preheat.hpp"

#include "../../Marlin.h"
#include "../../module/temperature.h"
#include "../../gcode/gcode.h"
#include "../../lcd/ultralcd.h"
#include "../../../marlin_stubs/skippable_gcode.hpp"

#if HAS_HEATED_BED

static constexpr int16_t minimal_preheat_temp = 60;
static constexpr int16_t minimal_temp_diff = 15;

void BedPreheat::update() {
    bool temp_near_target = thermalManager.degTargetBed() && std::abs(thermalManager.degBed() - thermalManager.degTargetBed()) < minimal_temp_diff;

    bool bedlets_changed = false;
    #if ENABLED(MODULAR_HEATBED)
    if (thermalManager.getEnabledBedletMask() != last_enabled_bedlets) {
        last_enabled_bedlets = thermalManager.getEnabledBedletMask();
        bedlets_changed = true;
    }
    #endif

    if (temp_near_target && bedlets_changed == false) {
        if (heating_start_time.has_value() == false) {
            heating_start_time = millis();
        }
        can_preheat = true;
        if (remaining_preheat_time() == 0) {
            preheated = true;
        }
    } else {
        heating_start_time = std::nullopt;
        can_preheat = false;
        preheated = false;
    }
}

uint32_t BedPreheat::required_preheat_time() {
    if (thermalManager.degTargetBed() < minimal_preheat_temp) {
        return 0;
    }

    return std::max((180 + (thermalManager.degTargetBed() - 60) * (12 * 60 / 50)) * 1000, 0);
}

uint32_t BedPreheat::remaining_preheat_time() {
    if (preheated) {
        return 0;
    }

    const auto now = millis();
    const int32_t required = required_preheat_time();
    const int32_t elapsed = now - heating_start_time.value_or(now);
    return std::max((required - elapsed) / 1000, int32_t(0));
}

void BedPreheat::wait_for_preheat() {
    SkippableGCode::Guard skippable_operation;

    static constexpr uint32_t message_interval = 1000;
    uint32_t last_message_timestamp = millis() - message_interval;

    while (can_preheat && !preheated && !skippable_operation.is_skip_requested()) {
        idle(true);

        // make sure we don't turn off the motors
        gcode.reset_stepper_timeout();

        if (millis() - last_message_timestamp > message_interval) {
            const uint32_t remaining_s = remaining_preheat_time() / 1000;
            const uint32_t minutes = remaining_s / 60;
            const uint32_t seconds = remaining_s % 60;
            MarlinUI::status_printf_P(0, "Absorbing heat (%i:%02i)", minutes, seconds);
            last_message_timestamp = millis();
        }
    }

    MarlinUI::reset_status();
}

BedPreheat bed_preheat;

#endif
