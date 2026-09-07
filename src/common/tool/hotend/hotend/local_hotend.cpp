/// @file
#include "local_hotend.hpp"

#include <module/thermistor/thermistors.h>
#include <module/temperature.h>
#include <module/temperature/temp_defines.hpp>
#include <fanctl.hpp>
#include <freertos/task.hpp>

#include <option/has_planner.h>
#if HAS_PLANNER()
    #include <module/planner.h>
    #include <module/stepper.h>
#endif

#include <option/has_power_panic.h>
#include <bsod/bsod.h>
#if HAS_POWER_PANIC()
    #include <power_panic.hpp>
#endif

#if WATCH_HEATBREAK
static constexpr HeaterWatch::Config heatbreak_watch_config {
    .temp_increase = -WATCH_HEATBREAK_TEMP_DECREASE,
    .period_s = WATCH_HEATBREAK_TEMP_PERIOD,
    .min_temp_diff = -HEATBREAK_MAXTEMP_OFFSET,
    .watch_cooling_instead = true,
};
#endif

LocalHotend::LocalHotend(PhysicalToolIndex tool, const Config *config)
    : BaseHotend(tool, &config->base_config)
    , local_config_(*config)
    , nozzle_raw_temp_range_ { MarlinTemptableRawMinMax::compute(local_config_.nozzle_temp_table, base_config_.min_nozzle_temp, base_config_.max_nozzle_temp) }
#if HAS_TEMP_HEATBREAK
    , heatbreak_raw_temp_range_ { MarlinTemptableRawMinMax::compute(local_config_.heatbreak_temp_table, HEATBREAK_MINTEMP, HEATBREAK_MAXTEMP) }
#endif
#if WATCH_HEATBREAK
    , heatbreak_watch_(heatbreak_watch_config)
#endif
{
    // Must be eager-constructed in task context, never lazily from an ISR (BFW-8126):
    // the temperature ISR touches this singleton and would trip the C++ init-guard assert.
    debug_assert(!freertos::is_inside_interrupt());

    // Do not call pinMode - it does nothing
    // pinMode(local_config_.nozzle_heater_marlin_pin, OUTPUT);

    // Do NOT call digitalWrite here — it is unnecessary at construction:
    // - Safe state should be off anyway
    // - Should get off anyway in the first manage() call from within Temperature::init()
    // digitalWrite(local_config_.nozzle_heater_marlin_pin, false);

    // Same with autofan
}

void LocalHotend::handle_nozzle_target_change() {
    BaseHotend::handle_nozzle_target_change();
    // If turning off the hotend, set the hotend off immediately
    // This shortcut was originally in Temperature::disable_heaters
    if (nozzle_target_temp_ <= 0) {
        nozzle_heater_pwm_ = 0;
        digitalWrite(local_config_.nozzle_heater_marlin_pin, false);
    }
}

#if HAS_TEMP_HEATBREAK_CONTROL
void LocalHotend::set_heatbreak_target_temp(TargetTemperature set) {
    BaseHotend::set_heatbreak_target_temp(set);

    #if WATCH_HEATBREAK
    heatbreak_watch_.arm(heatbreak_target_temp());
    #endif
}
#endif

void LocalHotend::manage() {
    nozzle_temp_ = marlin_temptable_lookup(local_config_.nozzle_temp_table, nozzle_raw_temp_);
    nozzle_low_temp_filter_.Put(nozzle_raw_temp_);

    // Extra averaging filter for noise reduction.
    const float curr_nozzle_temp = nozzle_temp().value();
    if (curr_nozzle_temp <= local_config_.nozzle_filter_max_temp) {
        const auto filtered_raw_temp = nozzle_low_temp_filter_.GetSum() / nozzle_low_temp_filter_.GetCount();
        nozzle_temp_ = marlin_temptable_lookup(local_config_.nozzle_temp_table, filtered_raw_temp);
    }

    // !!! MUST be called after temps are set properly
    BaseHotend::manage();

    // Manage temperature regulation
    {
        auto &t = thermalManager;

#if ENABLED(PID_EXTRUSION_SCALING)
        static_assert(HAS_PLANNER());

        const bool is_current_tool = stdext::holds_value(PhysicalToolIndex::currently_selected(), tool_);

        // Note: This seems like a BS. manage_heater is called in idle(), not in the timer ISR
        // We should instead be using actual measured intervals between manage() calls
        // Or just remove this completely, as it doesn't seem to be working and noone noticed
        constexpr float sample_frequency = TEMP_TIMER_FREQUENCY / MIN_ADC_ISR_LOOPS / OVERSAMPLENR;
        constexpr float distance_to_volume = std::numbers::pi_v<float> * std::pow(DEFAULT_NOMINAL_FILAMENT_DIA / 2, 2.f);
        constexpr float distance_to_volume_per_second = distance_to_volume * sample_frequency;

        uint32_t e_position = stepper.position(E_AXIS);
        const float e_volume_delta = is_current_tool ? (e_position - last_e_position_) * planner.mm_per_step[E_AXIS] * distance_to_volume_per_second : 0;
        last_e_position_ = e_position;
#endif

        HotendRegulatorResult regulation_result {
            .pid_output = 0,
            .feed_forward = 0,
        };

        if (curr_nozzle_temp > base_config_.min_nozzle_temp && curr_nozzle_temp < base_config_.max_nozzle_temp) {
            static_assert(PhysicalToolIndex::count == 1);
            regulation_result = nozzle_regulator_.get_pid_output_hotend(HotendRegulatorArgs {
                .pid = nozzle_pid_config(),
                .hotend_index = tool_.to_raw(),
                .fan_speed = t.print_fan_speed, // FIXME: Bit of a cockup if we have multiple hotends.
                    .current_temp = curr_nozzle_temp,
                .target_temp = nozzle_target_temp(),
#if ENABLED(PID_EXTRUSION_SCALING)
                .e_volume_delta = (thermalManager.extrusion_scaling_enabled && is_current_tool) ? e_volume_delta : 0,
#endif
            });
        }

        nozzle_heater_pwm_ = static_cast<uint8_t>(std::clamp<float>(std::round(regulation_result.pid_output), 0, 255));
#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
        thermal_model_protection_.step(regulation_result.pid_output, regulation_result.feed_forward);
        thermal_model_protection_ok_ = thermal_model_protection_.is_ok();
#endif
    }

    // Write to the output, if we can. soft pwm is handled through isr_soft_pwm
    if (!local_config_.nozzle_heater_soft_pwm) {
        analogWrite(local_config_.nozzle_heater_marlin_pin, nozzle_heater_pwm_);
    }

#if ENABLED(HAS_HOTEND_AUTO_FAN)
    auto_fan_out_ = (curr_nozzle_temp >= EXTRUDER_AUTO_FAN_TEMPERATURE)
        // Give the auto fan a bit of hysteresis
        || (auto_fan_out_ && curr_nozzle_temp >= EXTRUDER_AUTO_FAN_TEMPERATURE - 5);

    const auto auto_fan_pwm =
    #if PRINTER_IS_PRUSA_MK3_5()
        // PWM value of 80 roughly translates to 4k RPM, further testing my find better value, thus far this seems precise enough plus it is the value used by MINI which uses the same fans
        auto_fan_out_ ? (config_store().has_alt_fans.get() ? 80 : 255) : 0;
    #else
        auto_fan_out_ ? 80 : 0;
    #endif

    Fans::heat_break(tool_).set_pwm(auto_fan_pwm);

    #if HAS_TEMP_HEATBREAK
        #error Cannot have both heabtreak control and autofan
    #endif
#endif

#if HAS_TEMP_HEATBREAK
    manage_heatbreak();
#endif
}

void LocalHotend::isr_on_readings_ready() {
    static_assert(PhysicalToolIndex::count == 1, "Multiple local hotends are not supported");

    const bool heater_on = (nozzle_target_temp() > 0 || nozzle_heater_pwm() > PWM255(0));

    {
        auto &acc = thermalManager.temp_hotend.acc;

        // Note: Before Hotend refactoring, updating the raw value was waiting for temp_meas_ready
        // Now, we are using std::atomic like sane people, so it shouldn't be necessary
        nozzle_raw_temp_ = acc;
        acc = 0;

        nozzle_raw_temp_range_.check_temperror(nozzle_raw_temp_, (heater_ind_t)tool_.to_raw(), heater_on);
    }

#if HAS_TEMP_HEATBREAK
    {
        auto &acc = thermalManager.temp_heatbreak.acc;

        // Note: Before Hotend refactoring, updating the raw value was waiting for temp_meas_ready
        // Now, we are using std::atomic like sane people, so it shouldn't be necessary
        heatbreak_raw_temp_ = acc;
        acc = 0;

        heatbreak_raw_temp_range_.check_temperror(heatbreak_raw_temp_, H_HEATBREAK_FIRST + tool_.to_raw(), heater_on);
    }
#endif
}

void LocalHotend::isr_soft_pwm(PWM255 phase) {
    if (local_config_.nozzle_heater_soft_pwm) {
        const auto pwm = nozzle_heater_pwm_.load();
        digitalWrite(local_config_.nozzle_heater_marlin_pin, pwm > 0 && phase.value <= pwm);
    }
}

#if HAS_TEMP_HEATBREAK
void LocalHotend::manage_heatbreak() {
    // Things got mangled together, stuff would get screwed up if this didn't apply
    static_assert(ENABLED(PIDTEMPHEATBREAK));

    heatbreak_temp_ = marlin_temptable_lookup(local_config_.heatbreak_temp_table, heatbreak_raw_temp_);

    const auto ms = millis();
    if (!ELAPSED(ms, next_heatbreak_check_ms_)) {
        return;
    }

    #if HAS_POWER_PANIC()
    // Do not re-enable the heatbreak fan during power panic
    if (power_panic::is_ac_fault_active()) {
        return;
    }
    #endif

    // !!! Needs to stay on a 1s step because of the PID controller
    next_heatbreak_check_ms_ = ms + 1000;

    #if WATCH_HEATBREAK
    if (heatbreak_watch_.update(heatbreak_temp())) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HEATBREAK_COOLING_TOO_SLOW);
    }
    #endif

    // iX has a non-constant maxtemp for the heatbreak, so we need to explicitly set it
    #if PRINTER_IS_PRUSA_iX()
    int16_t heatbreak_maxtemp = heatbreak_target_temp() + HEATBREAK_MAXTEMP_OFFSET;
    #else
    int16_t heatbreak_maxtemp = HEATBREAK_MAXTEMP;
    #endif

    const float curr_nozzle_temp = nozzle_temp().value();

    if (WITHIN(heatbreak_temp(), HEATBREAK_MINTEMP, heatbreak_maxtemp)) {
        const auto regulator_out = heatbreak_fan_regulator_.step(HeatbreakRegulator::Args {
            .current_temp = heatbreak_temp(),
            .target_temp = heatbreak_target_temp(),
            .current_hotend_temp = curr_nozzle_temp,
        });
        heatbreak_fan_pwm_ = PWM255((uint8_t)std::clamp<float>(std::round(regulator_out), 0, 255));

    } else {
    #if WATCH_HEATBREAK
        // if we are not watching heatbreak (not in process of cooling down), red screen
        if (!heatbreak_watch_.is_running() && heatbreak_target_temp() > 0) {
            fatal_error(ErrCode::ERR_TEMPERATURE_HEATBREAK_MAXTEMP_ERR);
        }
    #endif
        heatbreak_fan_pwm_ = PWM255(255);
    }

    Fans::heat_break(tool_).set_pwm(heatbreak_fan_pwm_.value);
}
#endif
