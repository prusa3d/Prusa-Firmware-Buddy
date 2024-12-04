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

constexpr const uint8_t MAX_INPUT_SHAPER_PULSES = 5;

constexpr const double INPUT_SHAPER_VELOCITY_EPSILON = 0.0001;
constexpr const double INPUT_SHAPER_ACCELERATION_EPSILON = 0.1;

namespace input_shaper {

struct Shaper {
    Shaper(const float par_a[], const float par_t[], int par_num_pulses)
        : num_pulses(par_num_pulses) {
        for (int i = 0; i < MAX_INPUT_SHAPER_PULSES; ++i) {
            if (i < num_pulses) {
                a[i] = par_a[i];
                t[i] = par_t[i];
            } else {
                a[i] = 0.;
                t[i] = 0.;
            }
        }
    }

    float a[MAX_INPUT_SHAPER_PULSES] = {};
    float t[MAX_INPUT_SHAPER_PULSES] = {};
    int num_pulses;
};

Shaper get(float damping_ratio, float shaper_freq, float vibration_reduction, input_shaper::Type type);
} // namespace input_shaper

typedef struct pulse_t {
    double t;
    double a;
} pulse_t;

typedef struct input_shaper_pulses_t {
    uint8_t num_pulses;
    std::array<pulse_t, MAX_INPUT_SHAPER_PULSES> pulses;
} input_shaper_pulses_t;

typedef struct logical_axis_input_shaper_t {
    std::array<const move_t *, MAX_INPUT_SHAPER_PULSES> m_move;
    std::array<double, MAX_INPUT_SHAPER_PULSES> m_next_change;

    const input_shaper_pulses_t *m_pulses;

    // The largest index corresponds to the pointer to the rightmost time point (on the time axis).
    // And index zero corresponds to the pointer to the leftmost time point (on the time axis).
    double m_start_pos;
    double m_start_v;
    double m_half_accel;
    double m_print_time;
    uint8_t m_axis;
    uint8_t m_nearest_next_change_idx;

    void init(const move_t &move, uint8_t axis);

    bool update(const input_shaper_state_t &axis_is);

    void set_nearest_next_change(double new_nearest_next_change);

    uint8_t calc_nearest_next_change_idx() const;

    double calc_half_accel() const;

    double get_nearest_next_change() const {
        return m_next_change[m_nearest_next_change_idx];
    }

    const move_t *load_next_move_segment(uint8_t m_idx);
} logical_axis_input_shaper_t;

typedef struct input_shaper_state_t {
#ifdef COREXY
    std::array<logical_axis_input_shaper_t, 2> m_axis_shaper;
#else
    std::array<logical_axis_input_shaper_t, 1> m_axis_shaper;
#endif

    double start_pos;
    double start_v;
    double half_accel;

    double nearest_next_change;
    double print_time;

    bool step_dir;

    // Indicates if the current micro move segment is crossing zero velocity (needed change of stepper motor direction).
    bool is_crossing_zero_velocity;
} input_shaper_state_t;

input_shaper_pulses_t create_null_input_shaper_pulses();

input_shaper_pulses_t create_zv_input_shaper_pulses(float shaper_freq, float damping_ratio);

input_shaper_pulses_t create_zvd_input_shaper_pulses(float shaper_freq, float damping_ratio);

input_shaper_pulses_t create_mzv_input_shaper_pulses(float shaper_freq, float damping_ratio);

input_shaper_pulses_t create_ei_input_shaper_pulses(float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

input_shaper_pulses_t create_2hump_ei_input_shaper_pulses(float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

input_shaper_pulses_t create_3hump_ei_input_shaper_pulses(float shaper_freq, float damping_ratio, float vibration_reduction = 20.f);

class InputShaper {

public:
    static input_shaper_state_t is_state[3];
    static input_shaper_pulses_t logical_axis_pulses[3];

    InputShaper() = default;
};

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

void input_shaper_step_generator_init(const move_t &move, input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator);

void input_shaper_state_init(input_shaper_state_t &is_state, const move_t &move, uint8_t axis);

bool input_shaper_state_update(input_shaper_state_t &is_state, const int axis);
