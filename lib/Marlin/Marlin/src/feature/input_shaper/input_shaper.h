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
#include <stdint.h>
#include <type_traits>
#include "../../core/types.h"

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct input_shaper_step_generator_t;

constexpr const uint8_t MAX_INPUT_SHAPER_PULSES = 5;

constexpr const double INPUT_SHAPER_VELOCITY_EPSILON = 0.0001;
constexpr const double INPUT_SHAPER_ACCELERATION_EPSILON = 0.01;

namespace input_shaper {
enum class Type : uint8_t {
    first = 0,
    zv = first,
    second,
    zvd = second,
    mzv,
    ei,
    ei_2hump,
    ei_3hump,
    last = ei_3hump,
};

inline Type &operator++(Type &type) {
    using IntType = typename std::underlying_type<Type>::type;
    type = static_cast<Type>(static_cast<IntType>(type) + 1);
    return type;
}

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

void set(bool seen_x, bool seen_y, float damping_ratio, float frequency, float vibration_reduction, input_shaper::Type type);

Shaper get(double damping_ratio, double shaper_freq, double vibration_reduction, input_shaper::Type type);
}

typedef struct input_shaper_pulses_t {
    int num_pulses;
    struct {
        double t;
        double a;
    } pulses[MAX_INPUT_SHAPER_PULSES];
} input_shaper_pulses_t;

typedef struct input_shaper_state_t {
    move_t *m[MAX_INPUT_SHAPER_PULSES];
    double next_change[MAX_INPUT_SHAPER_PULSES];
    // The largest index corresponds to the pointer to the rightmost time point (on the time axis).
    // And index zero corresponds to the pointer to the leftmost time point (on the time axis).

    double start_pos;
    double start_v;
    double half_accel;
    int step_dir;

    double nearest_next_change;
    double print_time;

    // Indicates that the current state leads to crossing zero velocity (change of stepper motor direction).
    bool zero_crossing_v;
} input_shaper_state_t;

input_shaper_pulses_t create_zv_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_zvd_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_mzv_input_shaper_pulses(double shaper_freq, double damping_ratio);

input_shaper_pulses_t create_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

input_shaper_pulses_t create_2hump_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

input_shaper_pulses_t create_3hump_ei_input_shaper_pulses(double shaper_freq, double damping_ratio, double vibration_reduction = 20.);

class InputShaper {

public:
    static input_shaper_state_t is_state_x;
    static input_shaper_pulses_t is_pulses_x;

    static input_shaper_state_t is_state_y;
    static input_shaper_pulses_t is_pulses_y;

    static float x_frequency;         // Default value is 0Hz - It means that the input shaper is disabled.
    static float y_frequency;         // Default value is 0Hz - It means that the input shaper is disabled.

    static input_shaper::Type x_type; // 0:ZV, 1:ZVD, 2:MZV, 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 0:ZV.
    static input_shaper::Type y_type; // 0:ZV, 1:ZVD, 2:MZV, 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 0:ZV.

    static bool enabled;

    InputShaper() = default;
};

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t *step_generator, step_generator_state_t &step_generator_state, double flush_time);
void input_shaper_state_init(input_shaper_state_t *is_state, const input_shaper_pulses_t *is_pulses, move_t *m, int axis_idx);
