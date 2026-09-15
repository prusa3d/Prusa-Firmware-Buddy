/**
 * @file xl_enclosure.cpp
 */

#include "xl_enclosure.hpp"
#include "timing.h"
#include "config_store/store_instance.hpp"
#include "fanctl.hpp"
#include "gcode_info.hpp"
#include "filament.hpp"
#include <ctime>
#include <tools_mapping.hpp>
#include <marlin_server.hpp>
#include <bsod/bsod.h>
#include <option/xl_enclosure_support.h>
#include <option/has_chamber_filtration_api.h>
#include <feature/chamber_filtration/chamber_filtration.hpp>
#include <utils/math/ema.hpp>

static_assert(XL_ENCLOSURE_SUPPORT() && HAS_CHAMBER_FILTRATION_API());

Enclosure xl_enclosure;

Enclosure::Enclosure()
    : active_mode(EnclosureMode::Idle)
    , fan_presence_test_sec(0)
    , is_enabled_(config_store().xl_enclosure_enabled.get()) //
{
    last_sec = ticks_s();
}

void Enclosure::set_enabled(bool set) {
    if (is_enabled_ == set) {
        return;
    }

    is_enabled_ = set;
    config_store().xl_enclosure_enabled.set(is_enabled_);
    if (set) {
        buddy::chamber_filtration().set_backend(buddy::ChamberFiltrationBackend::xl_enclosure);
    }
}

void Enclosure::test_fan_presence(uint32_t curr_tick) {
    static constexpr uint32_t fan_presence_test_period_sec = 3;
    if (curr_tick - fan_presence_test_sec >= fan_presence_test_period_sec) {
        if (Fans::enclosure().get_rpm_is_ok()) {
            set_enabled(true);
            active_mode = EnclosureMode::Active;
        } else {
            set_enabled(false);
            is_temp_valid_ = false;
            active_mode = EnclosureMode::Idle;
            marlin_server::set_warning(WarningType::EnclosureFanError);
        }
        fan_presence_test_sec = 0;
    }
}

std::optional<buddy::Temperature> Enclosure::get_enclosure_temperature() {
    const buddy::Temperature temp = enclosure_temp_.load();
    if (std::isnan(temp)) {
        return std::nullopt;
    }
    return temp;
}

void Enclosure::update_enclosure_temperature(int16_t dwarf_board_temp, float nozzle_temp) {
    if (!is_temp_valid_) {
        enclosure_temp_ = NAN;
        return;
    }

    constexpr float estimated_max_ambient_temp_c = 30;
    // These parameters were measured in BFW-9363
    constexpr float nozzle_temp_at_given_correction_c = 275;
    constexpr float given_correction_c = -15;

    constexpr float filter_tau_sec = 100;

    float estimate = dwarf_board_temp;
    if (nozzle_temp > estimated_max_ambient_temp_c) {
        // Nozzle hot enough to influence the dwarf board temperature
        estimate += (nozzle_temp - estimated_max_ambient_temp_c) * given_correction_c / (nozzle_temp_at_given_correction_c - estimated_max_ambient_temp_c);
    }

    const float previous = enclosure_temp_.load();
    // Filter smooths out the step in the dwarf board temperature on a tool change
    enclosure_temp_ = std::isnan(previous) ? estimate : exponential_moving_average(previous, estimate, static_cast<float>(tick_delay_sec), filter_tau_sec);
}

void Enclosure::update_temp_validation_timer() {
    const auto print_state = marlin_vars().print_state.get();
    if (!marlin_server::is_printing_state(print_state) && !marlin_server::printer_paused_extended()) {
        // Reset the counter
        is_temp_valid_ = false;
    } else if (!is_temp_valid_) {
        // Printer is printing/pausing/paused/resuming and temp is not validated yet
        static constexpr uint32_t footer_temp_delay_s = 5 * 60;
        const auto print_dur = marlin_vars().print_duration.get();
        if (print_dur >= footer_temp_delay_s) {
            is_temp_valid_ = true;
        }
    }
}

PWM255 Enclosure::calculate_pwm(int32_t mcu_modular_bed_temp) {

    if (is_mcu_overheating(mcu_modular_bed_temp)) {
        // Override Fan pwm control
        // Overheating modular bed MCU has priority over active_mode
        return PWM255::from_percent(100);
    }

    switch (active_mode) {

    case EnclosureMode::Idle:
        return PWM255(0);

    case EnclosureMode::Test:
        return PWM255::from_percent(50);

    case EnclosureMode::Active:
        return buddy::chamber_filtration().output_pwm();
    }

    bsod_unreachable();
}

bool Enclosure::is_mcu_overheating(int32_t mcu_modular_bed_temp) {
    static constexpr int32_t mb_mcu_max_temp = 80; // °C
    static constexpr int32_t mb_mcu_safe_temp_threshold = 75; // °C

    if (mcu_modular_bed_temp > mb_mcu_max_temp) {
        is_mcu_overheated_ = true;
    } else if (mcu_modular_bed_temp <= mb_mcu_safe_temp_threshold) {
        is_mcu_overheated_ = false;
    }

    return is_mcu_overheated_;
}

void Enclosure::loop(int32_t mcu_modular_bed_temp, int16_t dwarf_board_temp, float nozzle_temp) {
    const uint32_t curr_sec = ticks_s();
    if (curr_sec - last_sec < tick_delay_sec) {
        return;
    }

    last_sec = curr_sec;

    // Deactivated enclosure
    if (!is_enabled() && active_mode != EnclosureMode::Idle) {
        active_mode = EnclosureMode::Idle;
        is_temp_valid_ = false;
    }

    switch (active_mode) {
    case EnclosureMode::Idle:

        if (is_enabled()) {
            active_mode = EnclosureMode::Test;
            fan_presence_test_sec = curr_sec;
        }
        break;

    case EnclosureMode::Test:

        test_fan_presence(curr_sec);
        break;

    case EnclosureMode::Active:

        // Update temperature validation timer (even during MCU Cooling)
        update_temp_validation_timer();
        break;
    }

    update_enclosure_temperature(dwarf_board_temp, nozzle_temp);

    // Control Fan PWM
    PWM255 fan_pwm = calculate_pwm(mcu_modular_bed_temp);
    Fans::enclosure().set_pwm(fan_pwm.value);
}
