#include "axes.hpp"
#include <array>
#include <module/planner.h>
#include <config_store/store_instance.hpp>

struct AxisMotorParams {
    int steps_per_rev = 0;
    float pos_to_phase = 0;
    float pos_to_steps = 0;
    float pos_to_msteps = 0;
    float mm_to_rev = 0;
    float rev_to_mm = 0;
    int phase_per_ustep = 0;

    static AxisMotorParams make_for_motor(int steps_per_rev, float steps_per_unit, int microsteps) {
        return {
            .steps_per_rev = steps_per_rev,
            .pos_to_phase = 256.f * steps_per_unit / microsteps,
            .pos_to_steps = steps_per_unit,
            .pos_to_msteps = steps_per_unit / PLANNER_STEPS_MULTIPLIER,
            .mm_to_rev = 1.f / (steps_per_rev * float(microsteps) / steps_per_unit),
            .rev_to_mm = steps_per_rev * float(microsteps) / steps_per_unit,
            .phase_per_ustep = 256 / microsteps
        };
    };
};

static std::array<AxisMotorParams, phase_stepping::opts::SUPPORTED_AXIS_COUNT> axis_motor_params;

void phase_stepping::initialize_axis_motor_params() {
    int xy_steps_per_rev = get_has_400step_xy_motors() ? 400 : 200;

    axis_motor_params[0] = AxisMotorParams::make_for_motor(xy_steps_per_rev, get_steps_per_unit_x(), get_microsteps_x());
    axis_motor_params[1] = AxisMotorParams::make_for_motor(xy_steps_per_rev, get_steps_per_unit_y(), get_microsteps_y());
    if (opts::SUPPORTED_AXIS_COUNT > 2) {
        axis_motor_params[2] = AxisMotorParams::make_for_motor(200, get_steps_per_unit_z(), get_microsteps_z());
    }
    if (opts::SUPPORTED_AXIS_COUNT > 3) {
        axis_motor_params[3] = AxisMotorParams::make_for_motor(200, get_steps_per_unit_e(), get_microsteps_e());
    }
}

void phase_stepping::update_axis_motor_params() {
    auto &s = planner.settings;
    std::array<AxisMotorParams, phase_stepping::opts::SUPPORTED_AXIS_COUNT> new_axis_motor_params;

    int xy_steps_per_rev = get_has_400step_xy_motors() ? 400 : 200;

    new_axis_motor_params[0] = AxisMotorParams::make_for_motor(xy_steps_per_rev, s.axis_steps_per_mm[X_AXIS], get_microsteps_x());
    new_axis_motor_params[1] = AxisMotorParams::make_for_motor(xy_steps_per_rev, s.axis_steps_per_mm[Y_AXIS], get_microsteps_y());
    if (opts::SUPPORTED_AXIS_COUNT > 2) {
        new_axis_motor_params[2] = AxisMotorParams::make_for_motor(200, s.axis_steps_per_mm[Z_AXIS], get_microsteps_z());
    }
    if (opts::SUPPORTED_AXIS_COUNT > 3) {
        new_axis_motor_params[3] = AxisMotorParams::make_for_motor(200, s.axis_steps_per_mm[E_AXIS], get_microsteps_e());
    }

    {
        // If the ISR triggers while we're updating this, we get Undefined Behavior, so we need to disable interrupts.
        buddy::InterruptDisabler _;
        axis_motor_params = new_axis_motor_params;
    }
}

int phase_stepping::get_motor_steps(AxisEnum axis) {
    return axis_motor_params[static_cast<int>(axis)].steps_per_rev;
}

int32_t phase_stepping::pos_to_phase(AxisEnum axis, float position) {
    return normalize_motor_phase(position * axis_motor_params[axis].pos_to_phase);
}

int32_t phase_stepping::pos_to_steps(AxisEnum axis, float position) {
    return position * axis_motor_params[static_cast<int>(axis)].pos_to_steps;
}

int32_t phase_stepping::pos_to_msteps(AxisEnum axis, float position) {
    return position * axis_motor_params[static_cast<int>(axis)].pos_to_msteps;
}

float phase_stepping::mm_to_rev(AxisEnum axis, float mm) {
    return mm * axis_motor_params[static_cast<int>(axis)].mm_to_rev;
}

float phase_stepping::rev_to_mm(AxisEnum axis, float revs) {
    return revs * axis_motor_params[static_cast<int>(axis)].rev_to_mm;
}

int phase_stepping::phase_per_ustep(AxisEnum axis) {
    return axis_motor_params[static_cast<int>(axis)].phase_per_ustep;
}
