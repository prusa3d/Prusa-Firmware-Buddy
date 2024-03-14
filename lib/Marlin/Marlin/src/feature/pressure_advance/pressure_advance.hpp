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
#include "pressure_advance_config.hpp"
#include "../../core/types.h"

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct pressure_advance_step_generator_t;

// #define PRESSURE_ADVANCE_SIMPLE_WINDOW_FILTER // Use one value filter instead of the Bartlett window.

constexpr const uint16_t PRESSURE_ADVANCE_MAX_FILTER_LENGTH = 41;

// The minimum difference of two consecutive position samples for which the step time is calculated
// by interpolation between them. For differences smaller than this value, the step time will be
// calculated by rounding to the earlier time of those samples.
constexpr const float PRESSURE_ADVANCE_MIN_POSITION_DIFF = 0.00001f;

typedef struct pressure_advance_buffer_t {
    float data[PRESSURE_ADVANCE_MAX_FILTER_LENGTH] = {};
    uint16_t length = 0;
    uint16_t start_idx = 0;
    // How many samples of position in the buffer have the same value.
    // Used for skipping pressure advance computations for moves when the extruder isn't active.
    uint16_t same_samples_cnt = 0;
} pressure_advance_buffer_t;

typedef struct pressure_advance_window_filter_t {
    float window[PRESSURE_ADVANCE_MAX_FILTER_LENGTH];
    uint16_t length;
} pressure_advance_window_filter_t;

struct pressure_advance_params_t {
    float pressure_advance_value;
    float sampling_rate_float;
    double sampling_rate;
    double filter_total_time;
    double filter_delay;

    pressure_advance_window_filter_t filter;
};

typedef struct pressure_advance_state_t {
    pressure_advance_buffer_t buffer;
    const move_t *current_move;

    // Index of next position sample within current move segment.
    // It resets whenever the current move segment is processed.
    uint32_t local_sample_idx;

    // Index of next position sample over the whole print time.
    // This value only increases and isn't reset during the pressure advance computations.
    // So, with a sampling rate of 1000 samples per second, the maximum print is 49.7 hours without resetting this value.
    uint32_t total_sample_idx;

    // Index of the last position sample over the whole print time from the current move.
    uint32_t current_move_last_total_sample_idx;

    // Amount of time that was left after subtracting the current move segment end time (from the time of the next sample) after that was fully processed.
    // This amount of time has to be added to time computed from local_sample_idx or total_sample_idx.
    float local_sample_time_left;

    // Two successive positions after applying pressure advance FIR filter.
    // Between those two positions, interpolation of the requested position is done.
    float prev_position;
    float next_position;

    float start_v;
    float half_accel;
    float start_pos;
    bool step_dir;

#ifndef NDEBUG
    // This variable indicates if the actual position in steps has to be inside range: (previous_interpolated_position, current_interpolated_position).
    // This is just for detecting numerical issues related to manipulating the actual position.
    bool position_has_to_be_in_range;
#endif
} pressure_advance_state_t;

pressure_advance_window_filter_t create_normalized_bartlett_window_filter(uint16_t filter_length);

pressure_advance_window_filter_t create_simple_window_filter(uint16_t filter_length);

pressure_advance_params_t create_pressure_advance_params(const pressure_advance::Config &config);

class PressureAdvance {

public:
    static pressure_advance_params_t pressure_advance_params;
    static pressure_advance_state_t pressure_advance_state;

    PressureAdvance() = default;
};

step_event_info_t pressure_advance_step_generator_next_step_event(pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

void pressure_advance_step_generator_init(const move_t &move, pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

void pressure_advance_state_init(pressure_advance_step_generator_t &step_generator, const pressure_advance_params_t &params, const move_t &move, uint8_t axis);
