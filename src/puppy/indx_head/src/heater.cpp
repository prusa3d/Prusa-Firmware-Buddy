/// @file
#include "heater.hpp"

#include <stm32c0xx_hal.h>
#include "hal.hpp"
#include <app.hpp>
#include <raii/scope_guard.hpp>

namespace {

struct IntervalLUTItem {
    float factor_pre;
    float factor_post;
};

struct IntervalLUTItemExt {
    float factor_pre;
    float factor_post;

    /// Squared duty cycle, calculated as factor_post + 0.25 * factor_pre
    fpm::fixed_16_16 duty_cycle_sq;

    constexpr IntervalLUTItemExt(const IntervalLUTItem &i)
        : factor_pre { i.factor_pre }
        , factor_post { i.factor_post }
        , duty_cycle_sq(i.factor_post + 0.25f * i.factor_pre) {
    }
};

// Values were found experimentally to switch MOSFET in zero voltage
// crossing. Do not mess with them without measuring changes with
// oscilloscope, or the MOSFET WILL BURN!
// !!! INDEX intervalLUT[0] ~ (current_power = 1), current_power 0 is not in this table
constexpr std::array<IntervalLUTItemExt, InductionHeater::max_power> intervalLUT {
    // 0
    IntervalLUTItem {
        .factor_pre = 0.00f,
        .factor_post = 0.17f,
    },
    // 1
    IntervalLUTItem {
        .factor_pre = 0.12f,
        .factor_post = 0.20f,
    },
    // 2
    IntervalLUTItem {
        .factor_pre = 0.18f,
        .factor_post = 0.25f,
    },
    // 3
    IntervalLUTItem {
        .factor_pre = 0.20f,
        .factor_post = 0.30f,
    },
    // 4
    IntervalLUTItem {
        .factor_pre = 0.24f,
        .factor_post = 0.35f,
    },
    // 5
    IntervalLUTItem {
        .factor_pre = 0.27f,
        .factor_post = 0.40f,
    },
    // 6
    IntervalLUTItem {
        .factor_pre = 0.28f,
        .factor_post = 0.45f,
    },
    // 7
    IntervalLUTItem {
        .factor_pre = 0.29f,
        .factor_post = 0.50f,
    },
    // 8
    IntervalLUTItem {
        .factor_pre = 0.30f,
        .factor_post = 0.55f,
    },
    // 9
    IntervalLUTItem {
        .factor_pre = 0.31f,
        .factor_post = 0.60f,
    },
    // 10
    IntervalLUTItem {
        .factor_pre = 0.32f,
        .factor_post = 0.65f,
    },
    // 11
    IntervalLUTItem {
        .factor_pre = 0.33f,
        .factor_post = 0.70f,
    },
    // 12
    IntervalLUTItem {
        .factor_pre = 0.34f,
        .factor_post = 0.75f,
    },
    // 13
    IntervalLUTItem {
        .factor_pre = 0.34f,
        .factor_post = 0.80f,
    },
};

// Induction heater PID constants
static constexpr fpm::fixed_16_16 Tu = static_cast<fpm::fixed_16_16>(0.3345f); // in seconds
static constexpr fpm::fixed_16_16 Ku = static_cast<fpm::fixed_16_16>(0.1f);

static constexpr fpm::fixed_16_16 pid_p = static_cast<fpm::fixed_16_16>(0.6f) * Ku;
static constexpr fpm::fixed_16_16 pid_i = (static_cast<fpm::fixed_16_16>(1.2f) * Ku * Tu) / 300;
static constexpr fpm::fixed_16_16 pid_d = (static_cast<fpm::fixed_16_16>(0.075f) * Ku / Tu) * 300;

} // namespace

#define ADC_SAMPLE_CYCLES 14 // 12.5 + 1.5, divider set to 1
#define SAMPLE_BUFFER_LEN 256
volatile uint16_t sample_buffer[SAMPLE_BUFFER_LEN];

InductionHeater::InductionHeater()
    : last_analysis {
        .status = RingdownAnalysisStatus::NOT_ENOUGH_PEAKS,
        .interval = 0,
        .decay = 0,
        .nozzle_presence = indx_head::NozzlePresence::unknown,
        .time = 0,
    }
    , dcOffset(0)
    , current_power(0)
    , last_valid_interval(0)
    , ramp_current_power(0)
    , ramp_target_power(0) {
}

uint16_t InductionHeater::pid_calculate_pwr_i(int32_t target_centideg, int32_t current_centideg) {
    // During TURBO/LIMITED, PID output is computed but overridden with fixed power.
    int32_t err = target_centideg - current_centideg;
    int32_t derr = last_centideg - current_centideg;
    last_centideg = current_centideg;

    fpm::fixed_16_16 pwr = err * pid_p + derr * pid_d;
    if (!((err < 0 && pwr + pid_i_state < static_cast<fpm::fixed_16_16>(0))
            || (err > 0 && pwr + pid_i_state > static_cast<fpm::fixed_16_16>(limited_max_power)))) {
        pid_i_state += err * pid_i; // change integral only if it has any effect, so we avoid windup
    }
    pwr += pid_i_state;
    return std::clamp<int32_t>((int32_t)pwr, 0, limited_max_power);
}

void InductionHeater::heater_control(int32_t target_centideg, int32_t current_centideg) {
    using enum HeaterControlMode;

    constexpr int32_t proximity_threshold_centideg = 10 * 100;

    const bool way_below_target = current_centideg + proximity_threshold_centideg < target_centideg;
    const bool close_to_target = !way_below_target && current_centideg < target_centideg;

    // Limit power to the heater when we are closing target temperature
    if (target_centideg <= 0) {
        heater_control_mode = HEATER_CONTROL_OFF;
    } else if (way_below_target) {
        // Target increased significantly - go to turbo mode to heat up faster, then switch to limited for stable approach
        heater_control_mode = HEATER_CONTROL_TURBO;
    } else if (heater_control_mode != HEATER_CONTROL_PID && close_to_target) {
        // Supply limited power when we are <10deg below target, do not overwrite PID
        heater_control_mode = HEATER_CONTROL_LIMITED;
    } else {
        heater_control_mode = HEATER_CONTROL_PID;
    }

    // PID runs in all active modes for bumpless transfer
    if (heater_control_mode != HEATER_CONTROL_OFF) {
        background_pwr_i = pid_calculate_pwr_i(target_centideg, current_centideg);
    }

    uint16_t pwr_i = 0;
    switch (heater_control_mode) {
    case HEATER_CONTROL_TURBO:
        // Full power to heat up fast
        pwr_i = max_power;
        break;
    case HEATER_CONTROL_LIMITED:
        // Limited power - causes slight overshoot which is beneficial to transfer heat to nozzle core
        pwr_i = limited_max_power;
        break;
    case HEATER_CONTROL_PID:
        // pwr_i already computed by PID above, clamped to limited_max_power
        pwr_i = background_pwr_i;
        break;
    case HEATER_CONTROL_OFF:
    default:
        break;
    }

    update(pwr_i);
}

uint8_t InductionHeater::current_pwm() const {
    if (current_power == 0) {
        return 0;
    }
    constexpr auto full_power = intervalLUT[max_power - 1].duty_cycle_sq;
    const auto current = intervalLUT[current_power - 1].duty_cycle_sq;
    constexpr auto scaling_factor = 255 / full_power;
    return static_cast<uint8_t>(current * scaling_factor);
}

bool InductionHeater::should_measure() const {
    if (last_analysis.status == RingdownAnalysisStatus::VALID) {
        // When heating, we don't want to lose power to measurement.
        // It is safe to heat with full power even for 0.5 seconds.
        // In idle, we want to minimize time to missing nozzle detection.
        const bool idle = current_power == 0;
        const uint32_t period = idle ? 100 : 500;

        return HAL_GetTick() - last_analysis.time > period;
    } else {
        return true;
    }
}

void InductionHeater::measure() {
    last_analysis.status = RingdownAnalysisStatus::NOT_ENOUGH_PEAKS;

    stop_timer();

    hal::config_adc_induction_heater();
    hal::peripherals::adc_semaphore.try_acquire_for(0); // Drain after config switch to catch any spurious DMA completion
    ScopeGuard g = [] { hal::config_adc_default(); };

    // Calibrate DC offset
    if (HAL_ADC_Start_DMA(&hal::peripherals::hadc1, (uint32_t *)sample_buffer, SAMPLE_BUFFER_LEN) != HAL_OK) {
        return;
    }
    if (!hal::peripherals::adc_semaphore.try_acquire_for(5)) {
        return;
    }
    retare_analysis();

    // Fire a single impulse and capture the LC circuit ringdown
    if (HAL_ADC_Stop_DMA(&hal::peripherals::hadc1) != HAL_OK) {
        return;
    }
    if (HAL_ADC_Start_DMA(&hal::peripherals::hadc1, (uint32_t *)sample_buffer, SAMPLE_BUFFER_LEN) != HAL_OK) {
        return;
    }

    uint16_t duration = 10;
    WRITE_REG(TIM16->ARR, duration); // Pulse width in MCU cycles
    WRITE_REG(TIM16->CCR1, 1);
    WRITE_REG(TIM16->CNT, 0);
    WRITE_REG(TIM16->EGR, TIM_EGR_UG); // Forcing to generate a update
    WRITE_REG(TIM16->SR, 0);
    SET_BIT(TIM16->CR1, TIM_CR1_CEN); // enable counter

    if (!hal::peripherals::adc_semaphore.try_acquire_for(5)) {
        return;
    }

    ringdown_analysis();
}

void InductionHeater::update(uint16_t target_power) {
    target_power = std::min(target_power, max_power);

    bool did_measure = false;
    if (should_measure()) {
        measure();
        did_measure = true;
    }

    if (last_analysis.status != RingdownAnalysisStatus::VALID
        || last_analysis.nozzle_presence != indx_head::NozzlePresence::present) {
        // Do not heat without a valid analysis and a fully present nozzle — MOSFET would burn
        // without a nozzle (verified experimentally :) ). Partial-coupling readings classify as
        // `unknown`, so this single guard also covers them. The control loop will retry on the
        // next update() call in a few milliseconds.
        target_power = 0;
    }

    if (target_power == current_power && !did_measure) {
        return;
    }

    if (target_power == 0) {
        stop_timer();
        last_valid_interval = 0;
    } else {
        CLEAR_BIT(TIM16->DIER, TIM_DIER_UIE);
        ramp_target_power = target_power;
        if (current_power == 0) {
            apply_timer_config(1);
        } else if (current_power < target_power) {
            apply_timer_config(current_power);
        } else {
            apply_timer_config(target_power);
        }
        if (current_power == 0 || did_measure) {
            CLEAR_BIT(TIM16->CR1, TIM_CR1_OPM); // Disable one-pulse mode
            WRITE_REG(TIM16->CNT, 0);
            WRITE_REG(TIM16->EGR, TIM_EGR_UG); // Forcing to generate a update
            SET_BIT(TIM16->CR1, TIM_CR1_CEN); // Enable counter
        }
        WRITE_REG(TIM16->SR, 0);
        SET_BIT(TIM16->DIER, TIM_DIER_UIE);
    }
    current_power = target_power;
}

void InductionHeater::stop_timer() {
    // Disable update interrupt and set one-pulse mode, gracefully finishing
    // current pulse irrespective of its state and disabling counter after.
    CLEAR_BIT(TIM16->DIER, TIM_DIER_UIE);
    SET_BIT(TIM16->CR1, TIM_CR1_OPM);
}

void InductionHeater::apply_timer_config(uint16_t power) {
    const TimerConfig tc = timer_config[power - 1];
    WRITE_REG(TIM16->ARR, tc.ARR);
    WRITE_REG(TIM16->CCR1, tc.CCR1);
    ramp_current_power = power;
}

void InductionHeater::ramp_isr() {
    // clear interrupt flags
    WRITE_REG(TIM16->SR, 0);
    if (ramp_current_power < ramp_target_power) {
        // still ramping
        apply_timer_config(ramp_current_power + 1);
    } else {
        // done ramping
        CLEAR_BIT(TIM16->DIER, TIM_DIER_UIE);
    }
}

void InductionHeater::ringdown_analysis(void) {
    const uint8_t nulls_cnt = 10;
    uint16_t nulls[nulls_cnt]; // zero-crossing positions in CPU cycles
    uint8_t nulls_index = 0;

    const uint8_t peaks_cnt = nulls_cnt;
    uint16_t peaks[peaks_cnt];
    uint8_t peaks_index = 0;

    bool start_noise = true;

    for (int i = 1; i < SAMPLE_BUFFER_LEN - 1; i++) {
        // wait for actual oscillations
        if (start_noise) {
            if (sample_buffer[i] > dcOffset - 200) {
                continue;
            } else {
                start_noise = false;
            }
        }
        // find zero crossings to determine frequency
        if (nulls_index < nulls_cnt && sample_buffer[i] <= dcOffset && sample_buffer[i + 1] > dcOffset) {
            // Linear-interpolate the zero crossing with sub-sample precision, but stay
            // in integer math. Resolution is one MCU cycle.
            nulls[nulls_index] = i * ADC_SAMPLE_CYCLES
                + (dcOffset - sample_buffer[i]) * ADC_SAMPLE_CYCLES / (sample_buffer[i + 1] - sample_buffer[i]);
            nulls_index++;
        }

        // find peak maximums to determine decay
        if (nulls_index > 0 && peaks_index < peaks_cnt) {
            int16_t slope1 = sample_buffer[i] - sample_buffer[i - 1];
            int16_t slope2 = sample_buffer[i + 1] - sample_buffer[i];
            if (slope1 >= 0 && slope2 < 0) {
                peaks[peaks_index] = sample_buffer[i];
                peaks_index++;
            }
        }

        if (nulls_index >= nulls_cnt && peaks_index >= peaks_cnt) {
            break;
        }
    }

    RingdownAnalysis analysis;
    analysis.status = RingdownAnalysisStatus::VALID;
    analysis.time = HAL_GetTick();

    if (nulls_index < nulls_cnt || peaks_index < peaks_cnt) {
        analysis.status = RingdownAnalysisStatus::NOT_ENOUGH_PEAKS;
        last_analysis = analysis;
        app::set_nozzle_present(indx_head::NozzlePresence::unknown);
        app::set_ringdown_decay(0);
        return; // invalid data, can't continue
    }

    // sometime the first peak is distorted.
    uint8_t skip_count = 0;
    while (skip_count < nulls_cnt - (avg_peaks + 1)) {
        if (peaks[skip_count] < peaks[skip_count + 1]) { // first peak cant be smaller than second
            skip_count++;
            continue;
        }
        if (peaks[skip_count] >= dcOffset * 2) { // first peak still under power, wait for decay oscillations
            skip_count++;
            continue;
        }
        break; // all check valid
    }

    float decay = 0;
    for (int i = skip_count; i < skip_count + avg_peaks; i++) {
        decay += (float)peaks[i + 1] / peaks[i];
    }
    decay = 1.0f - (decay / avg_peaks);
    analysis.decay = decay;
    // Below the present threshold the LC circuit has no/insufficient energy sink and would
    // burn the TVS/MOSFET if we kept driving it. Decay in the band between thresholds means
    // partial coupling (e.g. nozzle stuck halfway) — reported as `unknown` so the publish
    // call below skips it and the debouncer doesn't advance toward a misleading stable state.
    if (decay >= nozzle_present_decay_threshold) {
        analysis.nozzle_presence = indx_head::NozzlePresence::present;
    } else if (decay < nozzle_absent_decay_threshold) {
        analysis.nozzle_presence = indx_head::NozzlePresence::absent;
    } else {
        analysis.nozzle_presence = indx_head::NozzlePresence::unknown;
    }

    uint16_t interval = 0;
    for (int i = skip_count; i < skip_count + avg_peaks; i++) {
        interval += nulls[i + 1] - nulls[i];
    }

    interval = interval / avg_peaks; // interval in CPU cycles
    analysis.interval = interval;

    if (ringdown_analysis_sanity_check(analysis)) {
        app::set_nozzle_present(analysis.nozzle_presence);
    } else {
        app::set_nozzle_present(indx_head::NozzlePresence::unknown);
    }

    for (int power = 0; power < max_power; ++power) {
        const uint16_t period = (uint16_t)interval;
        const auto &intervals = intervalLUT[power];
        const uint16_t duration_pre = static_cast<int16_t>(period * intervals.factor_pre);
        const uint16_t duration_post = static_cast<int16_t>(period * intervals.factor_post);

        TimerConfig &tc = timer_config[power];
        tc.ARR = period + duration_post;
        tc.CCR1 = period - duration_pre + 1;
    }

    last_analysis = analysis;
    app::set_ringdown_decay(static_cast<int16_t>(analysis.decay * 1000.0f));
}

bool InductionHeater::ringdown_analysis_sanity_check(RingdownAnalysis &result) {
    // TODO - this is very benevolent check
    if (result.interval < 170) {
        result.status = RingdownAnalysisStatus::INTERVAL_TOO_SHORT;
        return false;
    }
    if (result.interval > 300) {
        result.status = RingdownAnalysisStatus::INTERVAL_TOO_LONG;
        return false;
    }

    // Consistency check: interval shouldn't change dramatically between measurements
    if (last_valid_interval > 0) {
        uint16_t diff = result.interval > last_valid_interval ? result.interval - last_valid_interval : last_valid_interval - result.interval;
        if (diff * 5 > last_valid_interval) { // deviation > 20% - INDX_TODO: 20% is too much, needs revision
            last_valid_interval = 0; // Really should we zero it on first bad measurement? This disables this check for next measurement, which could be bad as well.
            result.status = RingdownAnalysisStatus::INTERVAL_CHANGE_TOO_BIG;
            return false;
        }
    }

    last_valid_interval = result.interval;
    return true;
}

void InductionHeater::retare_analysis(void) {
    // TODO - is this even necesary? The offset seems to be always the same
    uint32_t sum = 0;
    for (int i = 0; i < SAMPLE_BUFFER_LEN; i++) {
        sum += sample_buffer[i];
    }
    dcOffset = sum / SAMPLE_BUFFER_LEN;
}

InductionHeater inductionHeater;
