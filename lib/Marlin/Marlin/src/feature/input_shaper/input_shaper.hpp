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
#include "input_shaper_config.hpp"
#include "../../core/types.h"

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct input_shaper_step_generator_t;

constexpr const uint8_t MAX_INPUT_SHAPER_PULSES = 5;

constexpr const double INPUT_SHAPER_VELOCITY_EPSILON = 0.0001;
constexpr const double INPUT_SHAPER_ACCELERATION_EPSILON = 0.01;

namespace input_shaper {

struct Shaper {
    Shaper(const double par_a[], const double par_t[], int par_num_pulses)
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

    double a[MAX_INPUT_SHAPER_PULSES];
    double t[MAX_INPUT_SHAPER_PULSES];
    int num_pulses;
};

Shaper get(double damping_ratio, double shaper_freq, double vibration_reduction, input_shaper::Type type);
}

typedef struct input_shaper_pulses_t {
    uint8_t num_pulses;
    struct {
        double t;
        double a;
    } pulses[MAX_INPUT_SHAPER_PULSES];
} input_shaper_pulses_t;

typedef struct input_shaper_state_t {
    const move_t *m[MAX_INPUT_SHAPER_PULSES];
    double next_change[MAX_INPUT_SHAPER_PULSES];
    // The largest index corresponds to the pointer to the rightmost time point (on the time axis).
    // And index zero corresponds to the pointer to the leftmost time point (on the time axis).

    double start_pos;
    double start_v;
    double half_accel;
    int step_dir;

    double nearest_next_change;
    double print_time;

    uint8_t nearest_next_change_idx;

    // Indicates if the current micro move segment is crossing zero velocity (needed change of stepper motor direction).
    bool is_crossing_zero_velocity;

    FORCE_INLINE const move_t *load_next_move_segment(uint8_t m_idx);
} input_shaper_state_t;

input_shaper_pulses_t create_null_input_shaper_pulses();

input_shaper_pulses_t create_zv_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_zvd_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_mzv_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

input_shaper_pulses_t create_2hump_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

input_shaper_pulses_t create_3hump_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

class InputShaper {

public:
    static input_shaper_state_t is_state[3];
    static input_shaper_pulses_t is_pulses[3];

    static bool enabled;

    InputShaper() = default;
};

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state, double flush_time);

void input_shaper_step_generator_init(const move_t &move, input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator);

void input_shaper_state_init(input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses, const move_t &move, uint8_t axis);

FORCE_INLINE std::pair<double, uint8_t> input_shaper_state_calc_nearest_next_change(const input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses);
