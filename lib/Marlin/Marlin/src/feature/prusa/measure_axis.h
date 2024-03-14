#pragma once

#include "config_features.h"
#if ENABLED(AXIS_MEASURE)

    #include "../../core/types.h"
    #include "../../module/planner.h"
    #include "../../module/prusa/homing_utils.hpp"
    #include "../tmc_util.h"
    #include <optional>

class Measure_axis {
public:
    enum state_t {
        INIT,
        WAIT,
        RAISE_Z,
        QUICK_HOME_XY,
        BACK_X,
        HOME_X,
        MEASURE_X,
        BACK_Y,
        HOME_Y,
        MEASURE_Y,
        FINISH,
        last_ = FINISH
    };

    Measure_axis(bool measure_x = true, bool measure_y = true, xy_bool_t invert_dir = { false, false },
        feedRate_t fr_mm_s = 0,
        float raise_z = Z_HOMING_HEIGHT,
        bool no_modifiers = true,
        bool default_acceleration = false,
        bool default_current = false);

    void loop();
    void start() { state_ = state_t(INIT + 1); }
    state_t state() { return state_; }
    xy_float_t length() {
        return axis_length;
    }
    #if ENABLED(SENSORLESS_HOMING)
    void set_sensitivity(xy_long_t sensitivity) { this->sensitivity = sensitivity; }
    void set_period(xy_long_t max_period) { this->max_period = max_period; }
    #endif

private:
    xy_feedrate_t fr;
    float raise_z;
    Motion_Parameters mp;
    el_current_xyz_t current;
    xy_float_t axis_length = { NAN, NAN };
    #if ENABLED(SENSORLESS_HOMING)
    sensorless_t stealth_states;
    #endif
    #if HAS_WORKSPACE_OFFSET
    workspace_xyz_t workspace;
    #endif
    #if ENABLED(SENSORLESS_HOMING)
    std::optional<xy_long_t> sensitivity;
    std::optional<xy_long_t> max_period;
    #endif

    state_t state_; //< state of measuring, don't change directly
    xy_bool_t invert_dir;
    bool do_x;
    bool do_y;
    bool no_modifiers;
    bool default_acceleration;
    bool default_current;
    bool leveling;

    void state_start(); //< triggered just after state change
    void state_finish(); //< triggered just before state change

    // changes state and calls hooks
    void state_change(state_t state) {
        state_finish();
        state_ = state;
        state_start();
    }

    // switches to the next state
    void state_next() {
        if (state_ >= last_) {
            return;
        }
        state_change(state_t(state_ + 1));
    }

    void sensorless_enable(AxisEnum axis);
    void sensorless_disable(AxisEnum axis);
    void quick_home_start();
    void quick_home_finish();
    void home_back(AxisEnum axis);
    void home_start(AxisEnum axis, bool invert = false);
    void home_finish(AxisEnum axis);
    void save_length(AxisEnum axis);
    void finish();
};

#endif
