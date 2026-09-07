#include "app.hpp"

#include "critical_section.hpp"
#include "hal.hpp"
#include "heater.hpp"
#include "hotend_temp_compensation.hpp"
#include <tpis/tpis.hpp>
#include "timing.hpp"
#include "rtt.hpp"
#include "watchdog.hpp"

#include <coroutines/inplace_coroutine.hpp>
#include <filters/debouncer.hpp>
#include <freertos/mutex.hpp>
#include <freertos/timing.hpp>
#include <raii/lock_guard.hpp>

#include <algorithm>
#include <atomic>
#include <bsod/bsod.h>

namespace {

// default 25*C stored in centiDeg - modbus reads before the first valid TPiS reading don't report 0°C,
std::atomic<int16_t> nozzle_temp_uncompensated_c100 = 25 * 100;
std::atomic<int16_t> nozzle_temp_compensated_c100 = 25 * 100;
std::atomic<int16_t> tpis_ambient_temp_c100 = 25 * 100;

// Whether nozzle_temp_uncompensated_c100, nozzle_temp_compensated_c100 and tpis_ambient_temp_c100 contain valid values instead of initial garbage
std::atomic<uint8_t> temps_valid = false;

/// See indx_head::modbus::Status::hotend_temp_raw_c100_dt_s
std::atomic<int16_t> hotend_temp_raw_c100_dt_s = 0;

constexpr tpis::fixed max_nozzle_temp = tpis::fixed(330);
constexpr tpis::fixed min_nozzle_temp = tpis::fixed(5);
constexpr tpis::fixed max_tpis_ambient_temp = tpis::fixed(100);
constexpr tpis::fixed min_tpis_ambient_temp = tpis::fixed(10);
constexpr uint32_t invalid_nozzle_temp_timeout_ms = 1000 * 2;
/// Target nozzle temperature in DegC
std::atomic<uint16_t> target_temp = 0;
constexpr uint16_t max_target_temp = 300;

std::atomic<indx_head::NozzlePresence> nozzle_present = indx_head::NozzlePresence::unknown;
std::atomic<uint16_t> nozzle_invalidation_ack = 0;
std::atomic<int16_t> ringdown_decay = 0;

/// Last-sampled induction heater coil current [mA]. Reads ~0 when the heater is not driving.
std::atomic<uint16_t> last_heater_current_mA = 0;

Debouncer<bool> nozzle_debouncer { false, 3 }; // 3 consecutive identical readings to settle

constexpr uint32_t control_frequency = 300 /*Hz*/;
constexpr uint32_t control_delay_us = 1'000'000 / control_frequency;

constexpr uint32_t fan_update_frequency = 10 /*Hz*/;
constexpr uint32_t fan_update_delay_us = 1'000'000 / fan_update_frequency;

std::atomic<uint8_t> printfan_pwm = 0;
std::atomic<uint8_t> heatbreak_fan_pwm = 0;

std::atomic<uint32_t> printfan_start_ms = 0;
std::atomic<uint32_t> heatbreak_fan_start_ms = 0;

std::atomic<uint8_t> hotend_pwm_averaged { 0 };
fpm::fixed_16_16 hotend_pwm_ema_buffer { 0 };

struct LedsConfig : public indx_head::leds::LedConfig {
    using indx_head::leds::LedConfig::operator=;
    bool leds_changed = true;
    freertos::Mutex mtx {};
};

LedsConfig leds_config;

std::atomic<bool> selftest_mode = false;

std::atomic<uint32_t> hotend_energy_consumed_uJ { 0 };

/// Internally used by step_hotend_energy
uint32_t hotend_energy_nJ_accum = 0;

int16_t validate_board_temperature() {
    constexpr int16_t min_board_temp_degC = 10;
    constexpr int16_t max_board_temp_degC = 95;
    const int16_t board_temp_degC = hal::adc::get_board_temp();
    if (board_temp_degC < min_board_temp_degC) {
        hal::panic(indx_head::errors::FaultStatusMask::board_min_temp);
    } else if (board_temp_degC > max_board_temp_degC) {
        hal::panic(indx_head::errors::FaultStatusMask::board_max_temp);
    }
    return board_temp_degC;
}

void step_hotend_energy(uint32_t dt_us) {
    // Integrate V × I power [mW × ms]
    // All integer arithmetic, no 64-bit division (expensive on Cortex-M0)
    const uint32_t voltage_mV = hal::adc::get_input_voltage_mV();
    const uint32_t current_mA = hal::adc::get_heater_current_mA();

    // Expose the latest coil current for the heater selftest current check on xBuddy
    last_heater_current_mA.store(static_cast<uint16_t>(current_mA));

    // max ~75000, fits uint32_t
    const uint32_t power_mW = voltage_mV * current_mA / 1000;

    // To avoid truncation of dt_us/1000 (3333→3), accumulate sub-ms remainder
    hotend_energy_nJ_accum += power_mW * dt_us;
    hotend_energy_consumed_uJ += uint32_t { hotend_energy_nJ_accum / 1000 };
    hotend_energy_nJ_accum %= 1000;
}

void step_hotend() {
    const uint64_t now_us = timing::get_timestamp_us();
    const uint32_t now_ms = timing::get_timestamp_ms();

    static uint64_t last_induction_control_us = now_us;

    /// Timestamp of last valid nozzle temp reading
    static uint32_t last_valid_nozzle_temp_ms = now_ms;

    const uint32_t dt_us = uint32_t(now_us - last_induction_control_us);
    if (dt_us < control_delay_us) {
        return;
    }
    // Just to give scope what numbers we're dealing with
    // Duty cycle is 0-1, control_delay_us is in thousands - so when we cast to uint32_t after the duty cycle multiplication, we should get reasonably precise values
    static_assert(control_delay_us >= 1000 && control_delay_us <= 10000);

    hotend_temp_compensation::step();

    last_induction_control_us = now_us;
    hal::CheckedTemperatureReading nozzle_temp_reading = hal::i2c::read_tpis_temperature();

    if (nozzle_temp_reading.valid) {
        if (nozzle_temp_reading.temps.object_temperature_celsius > max_nozzle_temp) {
            hal::panic(indx_head::errors::FaultStatusMask::nozzle_max_temp);
        } else if (nozzle_temp_reading.temps.object_temperature_celsius < min_nozzle_temp) {
            hal::panic(indx_head::errors::FaultStatusMask::nozzle_min_temp);
        }

        if (nozzle_temp_reading.temps.ambient_temperature_celsius > max_tpis_ambient_temp) {
            hal::panic(indx_head::errors::FaultStatusMask::tpis_ambient_max_temp);
        } else if (nozzle_temp_reading.temps.ambient_temperature_celsius < min_tpis_ambient_temp) {
            hal::panic(indx_head::errors::FaultStatusMask::tpis_ambient_min_temp);
        }

        last_valid_nozzle_temp_ms = now_ms;

    } else if (now_ms - last_valid_nozzle_temp_ms > invalid_nozzle_temp_timeout_ms) {
        hal::panic(indx_head::errors::FaultStatusMask::tpis_invalid_timeout);
    }

    // Note: If !nozzle_temp_reading.valid, nozzle_temp_reading contains last valid value

    const int16_t nozzle_temp_uncompensated_c100 = static_cast<int16_t>(nozzle_temp_reading.temps.object_temperature_celsius * tpis::fixed(100));
    const int16_t nozzle_temp_compensated_c100 = nozzle_temp_uncompensated_c100 - hotend_temp_compensation::get_current_compensation_c100();

    // Calculate slope
    if (temps_valid) {
        const int64_t uncompensated_raw_slope = int64_t(nozzle_temp_uncompensated_c100 - ::nozzle_temp_uncompensated_c100.load()) * 1000000 / int64_t(dt_us);

        // Clamp to sane values to prevent too big spikes
        const int32_t clamped_raw_slope = int32_t(std::clamp<int64_t>(uncompensated_raw_slope, -100 * 100, 100 * 100));

        // Apply exponential fadeout to average the values a bit
        const int16_t filtered_slope = int16_t((int32_t(hotend_temp_raw_c100_dt_s.load()) * 7 + clamped_raw_slope * 1) / 8);

        ::hotend_temp_raw_c100_dt_s.store(filtered_slope);
    }

    ::nozzle_temp_uncompensated_c100.store(nozzle_temp_uncompensated_c100);
    ::nozzle_temp_compensated_c100.store(nozzle_temp_compensated_c100);
    ::tpis_ambient_temp_c100.store(static_cast<int16_t>(nozzle_temp_reading.temps.ambient_temperature_celsius * tpis::fixed(100)));

    // Start calculating slope only after the nozzle_temps store actual readouts
    // to prevent a slope spike on first valid readout
    // Note: If !nozzle_temp_reading.valid, nozzle_temp_reading contains last valid value
    temps_valid |= nozzle_temp_reading.valid;

    inductionHeater.heater_control(target_temp.load() * 100 /*centiDeg*/, nozzle_temp_compensated_c100);

    // computing averaged pwm value to show in gui
    const fpm::fixed_16_16 pwm { inductionHeater.current_pwm() };
    constexpr fpm::fixed_16_16 ema_weight { 0.02f };
    hotend_pwm_ema_buffer = hotend_pwm_ema_buffer * (1 - ema_weight) + pwm * ema_weight;
    hotend_pwm_averaged.store(static_cast<uint8_t>(hotend_pwm_ema_buffer));

    step_hotend_energy(dt_us);
}

namespace leds {
    struct CoroutineTag {};
    using CoroType = coroutines::InplaceCoroutine<CoroutineTag, 52>;
    using indx_head::leds::Color;
    Color desired_color = { .r = 0, .g = 0, .b = 0 };

    CoroType switch_color(uint32_t millis_delay) {
        uint32_t last_change = freertos::millis();
        enum class ColorToSet : uint8_t {
            primary,
            secondary
        } color_to_set
            = ColorToSet::secondary;
        {
            LockGuard guard(leds_config.mtx);
            desired_color.r = leds_config.primary.r;
            desired_color.g = leds_config.primary.g;
            desired_color.b = leds_config.primary.b;
        }
        hal::i2c::set_led_pwm(desired_color.r, desired_color.g, desired_color.b);
        while (true) {
            const uint32_t now = freertos::millis();
            if (now - last_change >= millis_delay) {
                switch (color_to_set) {
                case ColorToSet::primary:
                    color_to_set = ColorToSet::secondary;
                    {
                        LockGuard guard(leds_config.mtx);
                        desired_color.r = leds_config.primary.r;
                        desired_color.g = leds_config.primary.g;
                        desired_color.b = leds_config.primary.b;
                    }
                    break;
                case ColorToSet::secondary:
                    color_to_set = ColorToSet::primary;
                    {
                        LockGuard guard(leds_config.mtx);
                        desired_color.r = leds_config.secondary.r;
                        desired_color.g = leds_config.secondary.g;
                        desired_color.b = leds_config.secondary.b;
                    }
                    break;
                }
                last_change = now;
                hal::i2c::set_led_pwm(desired_color.r, desired_color.g, desired_color.b);
            }
            co_await std::suspend_always {};
        }
        co_return;
    }

    /// Linearly interpolate a color channel value based on the current temperature.
    ///
    /// Naive implementation that works for ourc case - will not work generally - test before you copy it somewhere else.
    constexpr uint8_t lerp_color_channel(uint8_t color_min, uint8_t color_max, uint16_t temp_min, uint16_t temp_max, uint16_t curr_temp) {
        if (curr_temp <= temp_min) {
            return color_min;
        }
        if (curr_temp >= temp_max) {
            return color_max;
        }
        const int16_t temp_range = static_cast<int16_t>(temp_max) - temp_min;
        const int16_t color_range = static_cast<int16_t>(color_max) - color_min;
        const int16_t temp_offset = static_cast<int16_t>(curr_temp) - temp_min;
        return static_cast<uint8_t>((color_min * temp_range + color_range * temp_offset) / temp_range);
    }

    constexpr Color convert_temperature(uint16_t temperature) {
        static constexpr Color light_blue { .r = 128, .g = 128, .b = 255 };
        static constexpr uint16_t min_temp = 20;
        static constexpr Color yellow { .r = 255, .g = 255, .b = 0 };
        static constexpr uint16_t mid_temp = 50;
        static constexpr Color red { .r = 255, .g = 0, .b = 0 };
        static constexpr uint16_t max_temp = 300;
        if (temperature <= min_temp) {
            return light_blue;
        }
        if (temperature >= max_temp) {
            return red;
        }
        if (temperature <= mid_temp) {
            return { .r = lerp_color_channel(light_blue.r, yellow.r, min_temp, mid_temp, temperature),
                .g = lerp_color_channel(light_blue.g, yellow.g, min_temp, mid_temp, temperature),
                .b = lerp_color_channel(light_blue.b, yellow.b, min_temp, mid_temp, temperature) };
        }
        return { .r = lerp_color_channel(yellow.r, red.r, mid_temp, max_temp, temperature),
            .g = lerp_color_channel(yellow.g, red.g, mid_temp, max_temp, temperature),
            .b = lerp_color_channel(yellow.b, red.b, mid_temp, max_temp, temperature) };
    };

    CoroType follow_nozzle_temp() {
        while (true) {
            desired_color = convert_temperature(nozzle_temp_compensated_c100.load() / 100);
            hal::i2c::set_led_pwm(desired_color.r, desired_color.g, desired_color.b);
            co_await std::suspend_always {};
        }
        co_return;
    }

    std::optional<CoroType> coro = std::nullopt;
} // namespace leds

void update_leds() {
    indx_head::leds::Mode mode;
    indx_head::leds::Color primary_color;
    uint16_t delay;
    {
        LockGuard guard(leds_config.mtx);
        mode = leds_config.mode;
        primary_color = leds_config.primary;
        delay = leds_config.delay_ms;
    }

    // Change desired color if needed
    switch (mode) {
    case indx_head::leds::Mode::off:
        leds::desired_color.r = 0;
        leds::desired_color.g = 0;
        leds::desired_color.b = 0;
        break;
    case indx_head::leds::Mode::solid:
        leds::desired_color.r = primary_color.r;
        leds::desired_color.g = primary_color.g;
        leds::desired_color.b = primary_color.b;
        break;
    default:
        break;
    }

    // Update fade if needed (blinking needs to have fade 0 so it blinks)
    if (delay > 0 && mode != indx_head::leds::Mode::blinking) {
        hal::i2c::set_led_fade(delay / 2);
    } else {
        hal::i2c::set_led_fade(0);
    }

    // Set the update function
    leds::coro.reset();
    switch (mode) {
    case indx_head::leds::Mode::off:
    case indx_head::leds::Mode::solid:
        leds::coro = std::nullopt;
        hal::i2c::set_led_pwm(leds::desired_color.r, leds::desired_color.g, leds::desired_color.b);
        break;
    case indx_head::leds::Mode::match_nozzle_temp:
        leds::coro = leds::follow_nozzle_temp();
        break;
    case indx_head::leds::Mode::blinking:
    case indx_head::leds::Mode::pulsing:
        leds::coro = leds::switch_color(delay);
        break;
    }
}

} // namespace

namespace app {

// Drives heatbreak fan in auto mode
class HeatbreakController {
    static constexpr uint32_t turn_off_delay_ms = 2 * 60 * 1000;
    uint32_t last_running_timestamp_ms;

public:
    HeatbreakController() {
        last_running_timestamp_ms = timing::get_timestamp_ms() - turn_off_delay_ms;
    }

    void update_loop(uint32_t now_ms) {
        if (should_run()) {
            last_running_timestamp_ms = now_ms;
        }
        const bool keep_running = now_ms - last_running_timestamp_ms < turn_off_delay_ms;
        set_pwm(keep_running ? 255 : 0);
    }

private:
    bool should_run() {
        const bool is_heating = target_temp.load() > 0;
        const bool nozzle_temp_threshold_reached = get_nozzle_temp_compensated_c100() > 50 * 100; /*stored in centiDeg*/
        const bool board_temp_threshold_reached = validate_board_temperature() > 40; /*stored in Deg*/
        return is_heating || nozzle_temp_threshold_reached || board_temp_threshold_reached || selftest_mode.load();
    }

    void set_pwm(uint8_t pwm) {
        hal::tim::set_heatbreak_fan_pwm(pwm);
        if (heatbreak_fan_pwm == 0) {
            // MUST be before setting the PWM to avoid race conditions
            heatbreak_fan_start_ms = freertos::millis();
        }
        heatbreak_fan_pwm = pwm;
    }
};

void run() {
    hal::i2c::init_comm();

    // Wait for the temperature data to be stable
    freertos::delay(300);

    uint64_t startup_timestamp_us = timing::get_timestamp_us();

    uint64_t last_fan_update_us = startup_timestamp_us;

    HeatbreakController heatbreak_ctl;

    FOREVER_WITH_WATCHDOG(100) {
        const uint64_t now_us = timing::get_timestamp_us();
        const uint32_t now_ms = timing::get_timestamp_ms();

        step_hotend();

        // Fans and leds control loop
        if (now_us - last_fan_update_us >= fan_update_delay_us) {
            last_fan_update_us = now_us;

            // Print fan - direct PWM control
            hal::tim::set_printfan_pwm(printfan_pwm.load());

            // Heatbreak fan
            heatbreak_ctl.update_loop(now_ms);

            // LEDs control loop
            bool run_update = false;
            {
                // TODO validate recursive mutexes
                LockGuard guard(leds_config.mtx);
                if (leds_config.leds_changed) {
                    leds_config.leds_changed = false;
                    run_update = true;
                }
            }
            if (run_update) {
                update_leds();
            }
            if (leds::coro.has_value()) {
                [[maybe_unused]] const auto res = (*leds::coro)();
                debug_assert(res == false);
            }
        }

        freertos::delay(1);
    }
}

int16_t get_nozzle_temp_uncompensated_c100() {
    return nozzle_temp_uncompensated_c100.load();
}

int16_t get_nozzle_temp_compensated_c100() {
    return nozzle_temp_compensated_c100.load();
}

int16_t get_hotend_temp_raw_c100_dt_s() {
    return hotend_temp_raw_c100_dt_s.load();
}

uint8_t get_hotend_pwm_averaged() {
    return hotend_pwm_averaged.load();
}

uint32_t get_hotend_energy_consumed_uJ() {
    return hotend_energy_consumed_uJ.load();
}

int16_t get_tpis_ambient_temp_c100() {
    return tpis_ambient_temp_c100.load();
}

bool get_temps_valid() {
    return temps_valid.load();
}

void set_nozzle_present(indx_head::NozzlePresence state) {
    if (state == indx_head::NozzlePresence::unknown) {
        // Invalid analysis or partial coupling (decay between thresholds) — skip the sample.
        // The debouncer is left untouched, so its current stability state is preserved until
        // clean present/absent samples resume.
        return;
    }

    CriticalSection cs; // ISR-safe guard against concurrent invalidate_nozzle_presence() from modbus task
    nozzle_debouncer.push(state == indx_head::NozzlePresence::present);
    if (nozzle_debouncer.is_stable()) {
        nozzle_present.store(nozzle_debouncer.value()
                ? indx_head::NozzlePresence::present
                : indx_head::NozzlePresence::absent);
    }
}

indx_head::NozzlePresence get_nozzle_present() {
    return nozzle_present.load();
}

void invalidate_nozzle_presence(uint16_t ack_value) {
    {
        CriticalSection cs; // ISR-safe guard against concurrent set_nozzle_present() from DMA ISR
        nozzle_debouncer.destabilize();
        nozzle_present.store(indx_head::NozzlePresence::unknown);
    }
    nozzle_invalidation_ack.store(ack_value);
}

uint16_t get_nozzle_invalidation_ack() {
    return nozzle_invalidation_ack.load();
}

void set_ringdown_decay(int16_t value) {
    ringdown_decay.store(value);
}

int16_t get_ringdown_decay() {
    return ringdown_decay.load();
}

uint16_t get_heater_current_mA() {
    return last_heater_current_mA.load();
}

void set_nozzle_target_temp(uint16_t new_target) {
    if (new_target > max_target_temp) {
        target_temp.store(max_target_temp);
    } else {
        target_temp.store(new_target);
    }
}

void set_led_config(const indx_head::leds::LedConfig cfg) {
    LockGuard guard(leds_config.mtx);
    leds_config.leds_changed = true;
    leds_config = cfg;
}

void set_printfan_pwm(uint8_t pwm) {
    // INDX_TODO: The RPM measuring doesn't work for PWM below ~90%. We do want
    // to check the RPM at least when PWM is close to 100%, but at the point of
    // a new, higher PWM being set the RPM may have not caught up, hence we use
    // this timer any time we're increasing PWM as a grace period before
    // starting to check the RPM.
    // The original condition was:
    // if (printfan_pwm == 0) {
    if (pwm > printfan_pwm) {
        // MUST be before setting the PWM to avoid race conditions
        printfan_start_ms = freertos::millis();
    }
    printfan_pwm = pwm;
}

uint8_t get_printfan_pwm() {
    return printfan_pwm.load();
}

uint8_t get_heatbreak_fan_pwm() {
    return heatbreak_fan_pwm.load();
}

static constexpr uint32_t fan_startup_duration_ms = 2000;

bool is_printfan_rpm_ok() {
    // TODO Workaround: RPM measurement doesn't work when PWM is off (need to
    // measure in the periods when PWM is high). Until this is fixed, consider
    // the fan always OK if PWM is not high enough. After RPM measurement is
    // fixed, the printfan_pwm check below should be printfan_pwm == 0.
    return printfan_pwm < 250 || hal::tim::get_printfan_rpm_counter() > 0 || freertos::millis() - printfan_start_ms < fan_startup_duration_ms;
}

bool is_heatbreak_fan_rpm_ok() {
    return heatbreak_fan_pwm == 0 || hal::tim::get_heatbreak_fan_rpm_counter() > 0 || freertos::millis() - heatbreak_fan_start_ms < fan_startup_duration_ms;
}

void set_selftest_mode(bool enabled) {
    selftest_mode.store(enabled);
}
} // namespace app
