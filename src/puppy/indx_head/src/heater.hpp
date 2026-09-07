/// @file
#pragma once

#include <array>
#include <cstdint>
#include <fpm/fixed.hpp>
#include <indx_head/nozzle_presence.hpp>

enum class RingdownAnalysisStatus {
    VALID = 0,
    NOT_ENOUGH_PEAKS,
    INTERVAL_TOO_SHORT,
    INTERVAL_TOO_LONG,
    INTERVAL_CHANGE_TOO_BIG,
};

struct RingdownAnalysis {
    RingdownAnalysisStatus status;
    uint16_t interval; // in CPU cycles
    float decay;
    indx_head::NozzlePresence nozzle_presence;
    uint32_t time; // in sys_tick
};

class InductionHeater {
public:
    InductionHeater(void);

    void ramp_isr();

    static constexpr uint8_t avg_peaks = 3; // number of ring-down cycles to average
    static constexpr uint16_t max_power = 14;
    static constexpr uint16_t limited_max_power = 10; // ~46W
    // Decay-to-presence thresholds. Reference measurements: ~0.096 with a fully seated nozzle,
    // ~0.029 without a nozzle. Values strictly between the thresholds indicate partial coupling
    // (e.g. nozzle stuck halfway) and are reported as `unknown` so the debouncer never settles
    // on them. Only `present` is safe to heat — below the present threshold the LC circuit has
    // no/insufficient energy sink and would burn the TVS/MOSFET at full power.
    static constexpr float nozzle_present_decay_threshold = 0.085f;
    static constexpr float nozzle_absent_decay_threshold = 0.04f;

    void heater_control(int32_t target_centideg, int32_t current_centideg);

    uint8_t current_pwm() const;

private:
    RingdownAnalysis last_analysis;
    uint16_t dcOffset;
    uint16_t current_power;
    uint16_t last_valid_interval;
    volatile uint16_t ramp_current_power;
    volatile uint16_t ramp_target_power;

    /// Precomputed timer configuration. This depends on the measured oscillation period.
    struct TimerConfig {
        uint16_t ARR; // auto-reload register
        uint16_t CCR1; // capture/compare register 1
    };

    // !!! INDEX timer_config[0] ~ intervalLUT[0] ~ (current_power = 1), current_power 0 is not in this table
    std::array<TimerConfig, max_power> timer_config;

    /// Apply precomputed timer configuration for given power.
    void apply_timer_config(uint16_t power);

    /// Stop timer.
    void stop_timer();

    void ringdown_analysis(void);
    bool ringdown_analysis_sanity_check(RingdownAnalysis &result);
    void retare_analysis(void);

    /// Return true if we should measure LC response and perform ringdown analysis.
    /// We need to do this periodically, because parameters drift and we also need
    /// to check nozzle presence.
    bool should_measure() const;

    /// Measure LC response and perform ringdown analysis.
    void measure();

    void update(uint16_t target_power);

    // Heater control
    enum class HeaterControlMode {
        HEATER_CONTROL_OFF,
        HEATER_CONTROL_TURBO, // max_power until target - 10°C
        HEATER_CONTROL_LIMITED, // limited_max_power until overshoot past target
        HEATER_CONTROL_PID, // PID clamped to limited_max_power
    };

    HeaterControlMode heater_control_mode = HeaterControlMode::HEATER_CONTROL_OFF;
    fpm::fixed_16_16 pid_i_state = static_cast<fpm::fixed_16_16>(0);
    int32_t last_centideg = 0;
    uint16_t background_pwr_i = 0;

    uint16_t pid_calculate_pwr_i(int32_t target_centideg, int32_t current_centideg);
};

extern InductionHeater inductionHeater;
