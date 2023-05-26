/**
 * Based on the implementation of the pressure advance in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 *
 * We implement the pressure advance using FIR filter that is less computationally demanding
 * and can be fully run on a single 32-bit MCU with nearly identical results as the implementation in Klipper.
 */
#pragma once
#include <cstdint>
#include "../../core/types.h"

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct pressure_advance_step_generator_t;

constexpr const uint16_t MAX_PRESSURE_ADVANCE_FILTER_LENGTH = 41;

// Used for rounding the requested distance to interpolated positions.
constexpr const double PRESSURE_ADVANCE_POSITION_ROUNDING = 0.00001;

typedef struct pressure_advance_buffer_t {
    double data[MAX_PRESSURE_ADVANCE_FILTER_LENGTH] = {};
    uint16_t start_idx = 0;
    // How many samples of position in the buffer have the same value.
    // Used for skipping pressure advance computations for moves when the extruder isn't active.
    uint16_t same_samples_cnt = 0;
} pressure_advance_buffer_t;

typedef struct pressure_advance_window_filter_t {
    double window[MAX_PRESSURE_ADVANCE_FILTER_LENGTH];
    uint16_t length;
} pressure_advance_window_filter_t;

typedef struct pressure_advance_params_t {
    double pressure_advance_value;
    double half_smooth_time;
    double sampling_rate;

    pressure_advance_window_filter_t filter;
} pressure_advance_params_t;

typedef struct pressure_advance_state_t {
    pressure_advance_buffer_t buffer;
    uint16_t buffer_length;

    double current_start_position;
    double current_move_time;
    move_t *current_move;
    uint32_t current_sample_idx;

    double previous_interpolated_position;
    double current_interpolated_position;

    bool step_dir;
    double offset;

    double start_print_time;

    double start_v;
    double half_accel;

    double current_pa_position;
    double previous_pa_position_ratio;
} pressure_advance_state_t;

pressure_advance_window_filter_t create_normalized_bartlett_window_filter(uint16_t filter_length);

pressure_advance_window_filter_t create_simple_window_filter(uint16_t filter_length);

pressure_advance_params_t create_pressure_advance_params(double pressure_advance_value, double pressure_advance_smooth_time, double sampling_rate, uint16_t filter_length);

class PressureAdvance {

public:
    static pressure_advance_params_t pressure_advance_params;
    static pressure_advance_state_t pressure_advance_state;

    PressureAdvance() = default;
};

step_event_info_t pressure_advance_step_generator_next_step_event(pressure_advance_step_generator_t *step_generator, step_generator_state_t &step_generator_state, double flush_time);
void pressure_advance_state_init(pressure_advance_state_t &state, const pressure_advance_params_t &params, move_t &move);
