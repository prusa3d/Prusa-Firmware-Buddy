/**
 * Based on the implementation of the input shaper in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Dmitry Butyugin <dmbutyugin@google.com>
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Dmitry Butyugin <dmbutyugin@google.com>
 * and Kevin O'Connor <kevin@koconnor.net> for Klipper in used data structures, input shaper filters,
 * and some computations.
 *
 * We chose a different approach for implementing the input shaper that is less computationally demanding
 * and can be fully run on a single 32-bit MCU with identical results as the implementation in Klipper.
 */
#pragma once
#include <array>
#include "input_shaper_config.hpp"
#include "../precise_stepping/fwdecl.hpp"
#include "../../core/types.h"
#include "../../core/time_ticks.hpp"

#ifdef COREXY
constexpr const uint8_t INPUT_SHAPER_MAX_LOGICAL_AXES = 2;
#else
constexpr const uint8_t INPUT_SHAPER_MAX_LOGICAL_AXES = 1;
#endif

constexpr const uint8_t INPUT_SHAPER_MAX_LENGTH = 5;
constexpr const uint8_t INPUT_SHAPER_MAX_PULSES = INPUT_SHAPER_MAX_LENGTH * INPUT_SHAPER_MAX_LOGICAL_AXES;

constexpr const float INPUT_SHAPER_VELOCITY_EPSILON = 0.0001f;
constexpr const float INPUT_SHAPER_ACCELERATION_EPSILON = 0.1f;
/// 10 µs merge threshold for CoreXY pulse alignment.
constexpr const TimeTicks INPUT_SHAPER_PULSES_MIN_TIME_DIFF = TimeTicks::from_us(10);

namespace input_shaper {

struct Shaper {
    Shaper(const float par_a[], const float par_t[], int par_num_pulses)
        : num_pulses(par_num_pulses) {
        for (int i = 0; i < INPUT_SHAPER_MAX_PULSES; ++i) {
            if (i < num_pulses) {
                a[i] = par_a[i];
                t[i] = par_t[i];
            } else {
                a[i] = 0.;
                t[i] = 0.;
            }
        }
    }

    float a[INPUT_SHAPER_MAX_PULSES] = {};
    float t[INPUT_SHAPER_MAX_PULSES] = {};
    int num_pulses;
};

Shaper get(float damping_ratio, float shaper_freq, float vibration_reduction, input_shaper::Type type);
} // namespace input_shaper

typedef struct pulse_t {
    TimeTicks t;
    float a;
} pulse_t;

typedef struct micro_move_segment_t {
    float start_pos = 0.f;
    float start_v = 0.f;
    float half_accel = 0.f;
} micro_move_segment_t;

typedef struct input_shaper_pulses_t {
    uint8_t num_pulses;
    std::array<pulse_t, INPUT_SHAPER_MAX_PULSES> pulses;

    micro_move_segment_t calc_micro_move_segment(const std::array<const move_t *, INPUT_SHAPER_MAX_PULSES> &moves, TimeTicks nearest_next_change, uint8_t logical_axis) const;
} input_shaper_pulses_t;

typedef struct input_shaper_state_t {
    std::array<const input_shaper_pulses_t *, INPUT_SHAPER_MAX_LOGICAL_AXES> m_logical_axis_pulses = {};
    std::array<const move_t *, INPUT_SHAPER_MAX_PULSES> m_move = {};

    // The largest index corresponds to the pointer to the rightmost time point (on the time axis).
    // And index zero corresponds to the pointer to the leftmost time point (on the time axis).
    std::array<TimeTicks, INPUT_SHAPER_MAX_PULSES> m_next_change = {};

    uint8_t m_logical_axis_pulses_cnt = 0;
    uint8_t m_nearest_next_change_idx = 0;

    uint8_t m_physical_axis = 0;

    // Indicates if the current micro move segment is crossing zero velocity (needed change of a stepper motor direction).
    bool m_is_crossing_zero_velocity = false;

    bool step_dir = false;
    float start_v = 0.f;
    float half_accel = 0.f;
    float start_pos = 0.f;

    /// beginning of current micro move segment
    TimeTicks print_time;
    /// end of current micro move segment
    TimeTicks nearest_next_change;

    void set_nearest_next_change(TimeTicks new_nearest_next_change);

    uint8_t calc_nearest_next_change_idx() const;

    TimeTicks get_nearest_next_change() const {
        return m_next_change[m_nearest_next_change_idx];
    }

    const move_t *load_next_move_segment(uint8_t m_idx);

} input_shaper_state_t;

void create_null_input_shaper_pulses(input_shaper_pulses_t &is_pulses);

void create_zv_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio);

void create_zvd_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio);

void create_mzv_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio);

void create_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

void create_2hump_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

void create_3hump_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

class InputShaper {

public:
    static input_shaper_state_t is_state[3];
    static input_shaper_pulses_t logical_axis_pulses[3];

    InputShaper() = default;
};

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

void input_shaper_step_generator_init(const move_t &move, input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator);

void input_shaper_state_init(input_shaper_state_t &is_state, const move_t &move, uint8_t physical_axis);

bool input_shaper_state_update(input_shaper_state_t &is_state, uint8_t physical_axis);
