/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * stepper.cpp - A singleton object to execute motion plans using stepper motors
 * Marlin Firmware
 *
 * Derived from Grbl
 * Copyright (c) 2009-2011 Simen Svale Skogsrud
 *
 * Grbl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Grbl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Timer calculations informed by the 'RepRap cartesian firmware' by Zack Smith
 * and Philipp Tiefenbacher.
 */

/**
 *         __________________________
 *        /|                        |\     _________________         ^
 *       / |                        | \   /|               |\        |
 *      /  |                        |  \ / |               | \       s
 *     /   |                        |   |  |               |  \      p
 *    /    |                        |   |  |               |   \     e
 *   +-----+------------------------+---+--+---------------+----+    e
 *   |               BLOCK 1            |      BLOCK 2          |    d
 *
 *                           time ----->
 *
 *  The trapezoid is the shape the speed curve over time. It starts at block->initial_rate, accelerates
 *  first block->accelerate_until step_events_completed, then keeps going at constant speed until
 *  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
 *  The slope of acceleration is calculated using v = u + at where t is the accumulated timer values of the steps so far.
 */

/**
 * Jerk controlled movements planner added Apr 2018 by Eduardo José Tagle.
 * Equations based on Synthethos TinyG2 sources, but the fixed-point
 * implementation is new, as we are running the ISR with a variable period.
 * Also implemented the Bézier velocity curve evaluation in ARM assembler,
 * to avoid impacting ISR speed.
 */

#include "stepper.h"

Stepper stepper; // Singleton

#include "planner.h"
#include <feature/precise_stepping/precise_stepping.hpp>
#include <timing_precise.hpp>
#include <bsod/bsod.h>

// public:

#if ENABLED(Z_TRIPLE_ENDSTOPS) || ENABLED(Z_STEPPER_AUTO_ALIGN)
                 // #error dead code found by automatic analyses (see BFW-5461)
bool Stepper::separate_multi_axis = false;
#endif

// private:

uint8_t Stepper::last_direction_bits; // = 0
uint8_t Stepper::axis_did_move; // = 0

xyz_steps_t Stepper::endstops_trigsteps;
abce_steps_t Stepper::count_position { 0 };
abce_steps_t Stepper::count_position_from_startup { 0 };
abce_steps_t Stepper::count_position_last_block { 0 };
xyze_int8_t Stepper::count_direction { 0 };

/**
 * Set the stepper direction of each axis
 *
 *   COREXY: X_AXIS=A_AXIS and Y_AXIS=B_AXIS
 *   COREXZ: X_AXIS=A_AXIS and Z_AXIS=C_AXIS
 *   COREYZ: Y_AXIS=B_AXIS and Z_AXIS=C_AXIS
 */
void Stepper::set_directions() {

#if MINIMUM_STEPPER_PRE_DIR_DELAY > 0
    delay_ns_precise<MINIMUM_STEPPER_PRE_DIR_DELAY>();
#endif

#define SET_STEP_DIR(A)                   \
    if (motor_direction(_AXIS(A))) {      \
        A##_APPLY_DIR(INVERT_##A##_DIR);  \
        count_direction[_AXIS(A)] = -1;   \
    } else {                              \
        A##_APPLY_DIR(!INVERT_##A##_DIR); \
        count_direction[_AXIS(A)] = 1;    \
    }

#if HAS_X_DIR
    SET_STEP_DIR(X); // A
#endif

#if HAS_Y_DIR
    SET_STEP_DIR(Y); // B
#endif

#if HAS_Z_DIR
    SET_STEP_DIR(Z); // C
#endif

    if (motor_direction(E_AXIS)) {
        REV_E_DIR(stepper_extruder);
        count_direction.e = -1;
    } else {
        NORM_E_DIR(stepper_extruder);
        count_direction.e = 1;
    }

// A small delay may be needed after changing direction
#if MINIMUM_STEPPER_POST_DIR_DELAY > 0
    delay_ns_precise<MINIMUM_STEPPER_POST_DIR_DELAY>();
#endif
}

// Return ratio of completed steps of current block (call within ISR context)
float Stepper::segment_progress() {
    const block_t *current_block = planner.get_current_processed_block();
    if (!current_block || !current_block->mstep_event_count) {
        return NAN;
    }

    xyze_msteps_t planned_msteps = current_block->msteps;
    abce_steps_t diff_steps = (count_position - count_position_last_block);
#if ENABLED(COREXY)
    xyze_msteps_t done_msteps = {
        (diff_steps[A_AXIS] + diff_steps[B_AXIS]) * PLANNER_STEPS_MULTIPLIER / 2,
        CORESIGN(diff_steps[A_AXIS] - diff_steps[B_AXIS]) * PLANNER_STEPS_MULTIPLIER / 2,
        diff_steps[C_AXIS] * PLANNER_STEPS_MULTIPLIER,
        diff_steps[E_AXIS] * PLANNER_STEPS_MULTIPLIER,
    };
#else
    xyze_msteps_t done_msteps = {
        diff_steps[X_AXIS] * PLANNER_STEPS_MULTIPLIER,
        diff_steps[Y_AXIS] * PLANNER_STEPS_MULTIPLIER,
        diff_steps[Z_AXIS] * PLANNER_STEPS_MULTIPLIER,
        diff_steps[E_AXIS] * PLANNER_STEPS_MULTIPLIER,
    };
#endif

    float planned;
    float done;

    if (planned_msteps.a || planned_msteps.b || planned_msteps.c) {
        // explicitly ignore extruder
        planned = (float)(planned_msteps.a + planned_msteps.b + planned_msteps.c);
        done = (float)(abs(done_msteps.a) + abs(done_msteps.b) + abs(done_msteps.c));
    } else {
        planned = (float)(current_block->mstep_event_count);
        done = (float)(abs(done_msteps.a) + abs(done_msteps.b) + abs(done_msteps.c) + abs(done_msteps.e));
    }

    return done / planned;
}

bool Stepper::is_axis_inverted(AxisEnum axis) {
    debug_assert(PreciseStepping::inverted_dirs_set);
    return !(PreciseStepping::inverted_dirs & (STEP_EVENT_FLAG_X_DIR << axis));
}

void Stepper::init() {

// Init Dir Pins
#if HAS_X_DIR
    X_DIR_INIT;
#endif
#if HAS_Y_DIR
    Y_DIR_INIT;
#endif
#if HAS_Z_DIR
    Z_DIR_INIT;
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS) && HAS_Z2_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    Z2_DIR_INIT;
    #endif
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS) && HAS_Z3_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    Z3_DIR_INIT;
    #endif
#endif
#if HAS_E0_DIR
    E0_DIR_INIT;
#endif
#if HAS_E1_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    E1_DIR_INIT;
#endif
#if HAS_E2_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    E2_DIR_INIT;
#endif
#if HAS_E3_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    E3_DIR_INIT;
#endif
#if HAS_E4_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    E4_DIR_INIT;
#endif
#if HAS_E5_DIR
    // #error dead code found by automatic analyses (see BFW-5461)
    E5_DIR_INIT;
#endif

// Init Enable Pins - steppers default to disabled.
#if HAS_X_ENABLE
    X_ENABLE_INIT;
    if (!X_ENABLE_ON) {
        X_ENABLE_WRITE(HIGH);
    }
#endif
#if DISABLED(XY_LINKED_ENABLE)
    #if HAS_Y_ENABLE
    Y_ENABLE_INIT;
    if (!Y_ENABLE_ON) {
        Y_ENABLE_WRITE(HIGH);
    }
    #endif
#endif
#if HAS_Z_ENABLE
    Z_ENABLE_INIT;
    if (!Z_ENABLE_ON) {
        Z_ENABLE_WRITE(HIGH);
    }
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS) && HAS_Z2_ENABLE
    // #error dead code found by automatic analyses (see BFW-5461)
    Z2_ENABLE_INIT;
    if (!Z_ENABLE_ON) {
        Z2_ENABLE_WRITE(HIGH);
    }
    #endif
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS) && HAS_Z3_ENABLE
    // #error dead code found by automatic analyses (see BFW-5461)
    Z3_ENABLE_INIT;
    if (!Z_ENABLE_ON) {
        Z3_ENABLE_WRITE(HIGH);
    }
    #endif
#endif
#if HAS_E0_ENABLE
    E0_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E0_ENABLE_WRITE(HIGH);
    }
#endif
#if HAS_E1_ENABLE
    E1_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E1_ENABLE_WRITE(HIGH);
    }
#endif
#if HAS_E2_ENABLE
    E2_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E2_ENABLE_WRITE(HIGH);
    }
#endif
#if HAS_E3_ENABLE
    E3_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E3_ENABLE_WRITE(HIGH);
    }
#endif
#if HAS_E4_ENABLE
    E4_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E4_ENABLE_WRITE(HIGH);
    }
#endif
#if HAS_E5_ENABLE
    // #error dead code found by automatic analyses (see BFW-5461)
    E5_ENABLE_INIT;
    if (!E_ENABLE_ON) {
        E5_ENABLE_WRITE(HIGH);
    }
#endif

#define _STEP_INIT(AXIS)           AXIS##_STEP_INIT
#define _WRITE_STEP(AXIS, HIGHLOW) AXIS##_STEP_WRITE(HIGHLOW)
#define _DISABLE(AXIS)             disable_##AXIS()
#define _INVERT_STEP_PIN(AXIS)     INVERT_##AXIS##_STEP_PIN

#define AXIS_INIT(AXIS, PIN) \
    _STEP_INIT(AXIS);        \
    _WRITE_STEP(AXIS, _INVERT_STEP_PIN(PIN))

#define E_AXIS_INIT(NUM) AXIS_INIT(E##NUM, E)

// Init Step Pins
#if HAS_X_STEP
    AXIS_INIT(X, X);
    #if ENABLED(XY_LINKED_ENABLE)
    _DISABLE(XY);
    #else
    _DISABLE(X);
    #endif
#endif

#if HAS_Y_STEP
    AXIS_INIT(Y, Y);
    #if ENABLED(XY_LINKED_ENABLE)
    _DISABLE(XY);
    #else
    _DISABLE(Y);
    #endif
#endif

#if HAS_Z_STEP
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS)
    // #error dead code found by automatic analyses (see BFW-5461)
    Z2_STEP_INIT;
    Z2_STEP_WRITE(INVERT_Z_STEP_PIN);
    #endif
    #if ENABLED(Z_TRIPLE_STEPPER_DRIVERS)
    // #error dead code found by automatic analyses (see BFW-5461)
    Z3_STEP_INIT;
    Z3_STEP_WRITE(INVERT_Z_STEP_PIN);
    #endif
    AXIS_INIT(Z, Z);
    _DISABLE(Z);
#endif

#if E_STEPPERS > 0 && HAS_E0_STEP
    E_AXIS_INIT(0);
#endif
#if E_STEPPERS > 1 && HAS_E1_STEP
    // #error dead code found by automatic analyses (see BFW-5461)
    E_AXIS_INIT(1);
#endif
#if E_STEPPERS > 2 && HAS_E2_STEP
    // #error dead code found by automatic analyses (see BFW-5461)
    E_AXIS_INIT(2);
#endif
#if E_STEPPERS > 3 && HAS_E3_STEP
    // #error dead code found by automatic analyses (see BFW-5461)
    E_AXIS_INIT(3);
#endif
#if E_STEPPERS > 4 && HAS_E4_STEP
    // #error dead code found by automatic analyses (see BFW-5461)
    E_AXIS_INIT(4);
#endif
#if E_STEPPERS > 5 && HAS_E5_STEP
    // #error dead code found by automatic analyses (see BFW-5461)
    E_AXIS_INIT(5);
#endif

    // Init direction bits for first moves
    last_direction_bits = 0
        | (INVERT_X_DIR ? _BV(X_AXIS) : 0)
        | (INVERT_Y_DIR ? _BV(Y_AXIS) : 0)
        | (INVERT_Z_DIR ? _BV(Z_AXIS) : 0);

    set_directions();
}

/**
 * Set the stepper positions directly in steps
 *
 * The input is based on the typical per-axis XYZ steps.
 * For CORE machines XYZ needs to be translated to ABC.
 *
 * This allows get_axis_position_mm to correctly
 * derive the current XYZ position later on.
 */
void Stepper::_set_position(const int32_t &x, const int32_t &y, const int32_t &z, const int32_t &e) {
#if CORE_IS_XY
    // corexy positioning
    // these equations follow the form of the dA and dB equations on http://www.corexy.com/theory.html
    abce_steps_t new_position = { x + y, CORESIGN(x - y), z, e };
#elif CORE_IS_XZ
    // #error dead code found by automatic analyses (see BFW-5461)
    // corexz planning
    abce_steps_t new_position = { x + z, y, CORESIGN(x - z), e };
#elif CORE_IS_YZ
    // #error dead code found by automatic analyses (see BFW-5461)
    // coreyz planning
    abce_steps_t new_position = { x, y + z, CORESIGN(y - z), e };
#else
    // default non-h-bot planning
    abce_steps_t new_position = { x, y, z, e };
#endif

    count_position = new_position;
}

// Signal endstops were triggered - This function can be called from
// an ISR context  (Temperature, Stepper or limits ISR).
void Stepper::endstop_triggered(const AxisEnum axis) {
    PreciseStepping::quick_stop();

#if IS_CORE
    ab_steps_t pos = { position(A_AXIS), position(B_AXIS) };
    endstops_trigsteps[A_AXIS] = pos.a + pos.b;
    endstops_trigsteps[B_AXIS] = CORESIGN(pos.a - pos.b);
#else // !IS_CORE
    endstops_trigsteps[A_AXIS] = position(A_AXIS);
    endstops_trigsteps[B_AXIS] = position(B_AXIS);
#endif
    endstops_trigsteps[C_AXIS] = position(C_AXIS);
}

void Stepper::report_positions() {
    // Note that the reported position is not atomic/synchronous for all axes
    // to avoid locking the step ISR
    const abc_steps_t pos = count_position.xyz();

#if CORE_IS_XY || CORE_IS_XZ
    SERIAL_ECHOPAIR(MSG_COUNT_A, pos.a, " B:", pos.b);
#else
    SERIAL_ECHOPAIR(MSG_COUNT_X, pos.x, " Y:", pos.y);
#endif
#if CORE_IS_XZ || CORE_IS_YZ
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOLNPAIR(" C:", pos.c);
#else
    SERIAL_ECHOLNPAIR(" Z:", pos.z);
#endif
}

#if ENABLED(BABYSTEPPING)

    #if ENABLED(SQUARE_WAVE_STEPPING) && (MINIMUM_STEPPER_PRE_DIR_DELAY > 0) && (MINIMUM_STEPPER_POST_DIR_DELAY > 0)
        #define STEP_PULSE_CYCLES 0 // minimum step time is ensured due to required direction change delays
    #elif MINIMUM_STEPPER_PULSE
        // #error dead code found by automatic analyses (see BFW-5461)
        #define STEP_PULSE_CYCLES ((MINIMUM_STEPPER_PULSE)*CYCLES_PER_MICROSECOND)
    #else
        // #error dead code found by automatic analyses (see BFW-5461)
        #define STEP_PULSE_CYCLES 0
    #endif

    #define CYCLES_EATEN_BABYSTEP 0
    #define EXTRA_CYCLES_BABYSTEP (STEP_PULSE_CYCLES - (CYCLES_EATEN_BABYSTEP))

    #define _ENABLE(AXIS)            enable_##AXIS()
    #define _READ_DIR(AXIS)          AXIS##_DIR_READ()
    #define _INVERT_DIR(AXIS)        is_axis_inverted(_AXIS(AXIS))
    #define _APPLY_DIR(AXIS, INVERT) AXIS##_APPLY_DIR(INVERT)

    #if EXTRA_CYCLES_BABYSTEP > 20
        // #error dead code found by automatic analyses (see BFW-5461)
        #define _SAVE_START const hal_timer_t pulse_start = HAL_timer_get_count(PULSE_TIMER_NUM)
        #define _PULSE_WAIT                                                                                                                      \
            while (EXTRA_CYCLES_BABYSTEP > (uint32_t)(HAL_timer_get_count(PULSE_TIMER_NUM) - pulse_start) * (PULSE_TIMER_PRESCALE)) { /* nada */ \
            }
    #else
        #define _SAVE_START NOOP
        #if EXTRA_CYCLES_BABYSTEP > 0
            // #error dead code found by automatic analyses (see BFW-5461)
            #define _PULSE_WAIT delay_ns_precise<EXTRA_CYCLES_BABYSTEP * NANOSECONDS_PER_CYCLE>()
        #elif STEP_PULSE_CYCLES > 0
            // #error dead code found by automatic analyses (see BFW-5461)
            #define _PULSE_WAIT NOOP
        #else
            #define _PULSE_WAIT delay_us_precise<4>()
        #endif
    #endif

    #define BABYSTEP_AXIS(AXIS, INVERT, DIR)                    \
        {                                                       \
            const uint8_t old_dir = _READ_DIR(AXIS);            \
            _ENABLE(AXIS);                                      \
            delay_ns_precise<MINIMUM_STEPPER_PRE_DIR_DELAY>();  \
            _APPLY_DIR(AXIS, _INVERT_DIR(AXIS) ^ DIR ^ INVERT); \
            delay_ns_precise<MINIMUM_STEPPER_POST_DIR_DELAY>(); \
            _SAVE_START;                                        \
            AXIS##_STEP_SET();                                  \
            _PULSE_WAIT;                                        \
            AXIS##_STEP_RESET();                                \
            delay_ns_precise<MINIMUM_STEPPER_PRE_DIR_DELAY>();  \
            _APPLY_DIR(AXIS, old_dir);                          \
            delay_ns_precise<MINIMUM_STEPPER_POST_DIR_DELAY>(); \
        }

// MUST ONLY BE CALLED BY AN ISR,
// No other ISR should ever interrupt this!
void Stepper::babystep(const AxisEnum axis, const bool direction) {
    buddy::InterruptDisabler _;
    debug_assert(!phase_stepping::is_enabled(axis));

    switch (axis) {

    #if ENABLED(BABYSTEP_XY)
        // #error dead code found by automatic analyses (see BFW-5461)

    case X_AXIS:
        #if CORE_IS_XY
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(X, false, direction);
        BABYSTEP_AXIS(Y, false, direction);
        #elif CORE_IS_XZ
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(X, false, direction);
        BABYSTEP_AXIS(Z, false, direction);
        #else
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(X, false, direction);
        #endif
        break;

    case Y_AXIS:
        #if CORE_IS_XY
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(X, false, direction);
        BABYSTEP_AXIS(Y, false, direction ^ (CORESIGN(1) < 0));
        #elif CORE_IS_YZ
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(Y, false, direction);
        BABYSTEP_AXIS(Z, false, direction ^ (CORESIGN(1) < 0));
        #else
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(Y, false, direction);
        #endif
        break;

    #endif

    case Z_AXIS: {

    #if CORE_IS_XZ
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(X, BABYSTEP_INVERT_Z, direction);
        BABYSTEP_AXIS(Z, BABYSTEP_INVERT_Z, direction ^ (CORESIGN(1) < 0));

    #elif CORE_IS_YZ
        // #error dead code found by automatic analyses (see BFW-5461)
        BABYSTEP_AXIS(Y, BABYSTEP_INVERT_Z, direction);
        BABYSTEP_AXIS(Z, BABYSTEP_INVERT_Z, direction ^ (CORESIGN(1) < 0));

    #else
        BABYSTEP_AXIS(Z, BABYSTEP_INVERT_Z, direction);
    #endif

    } break;

    default:
        break;
    }
}

#endif // BABYSTEPPING

#if HAS_DRIVER(TMC2130) || HAS_DRIVER(TMC2209)
    #include "config_store/store_c_api.h"
void Stepper::microstep_mode(const uint8_t driver, const uint8_t stepping) {
    switch (driver) {
    case 0:
    #if AXIS_IS_TMC(X)
        stepperX.microsteps(stepping);
    #endif
        break;
    case 1:
    #if AXIS_IS_TMC(Y)
        stepperY.microsteps(stepping);
    #endif
        break;
    case 2:
    #if AXIS_IS_TMC(Z)
        stepperZ.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperZ2.microsteps(steping);
    #endif
    #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperZ3.microsteps(stepping);
    #endif
        break;
    case 3:
    #if AXIS_IS_TMC(E0)
        stepperE0.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperE1.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperE2.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperE3.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperE4.microsteps(stepping);
    #endif
    #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        stepperE5.microsteps(stepping);
    #endif
        break;

    default:
        SERIAL_ERROR_MSG("Axis unavailable");
        break;
    }
}
void Stepper::microstep_readings() {
    char msg[7]; // max len of message is 256
    SERIAL_ECHOPGM("Microsteps are:\n");

    #if AXIS_IS_TMC(X)
    SERIAL_ECHOPGM("X:");
    snprintf(msg, 7, "%u\n", stepperX.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(Y)
    SERIAL_ECHOPGM("Y:");
    snprintf(msg, 7, "%u\n", stepperY.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(Z)
    SERIAL_ECHOPGM("Z:");
    snprintf(msg, 7, "%u\n", stepperZ.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("Z2:");
    snprintf(msg, 7, "%u\n", stepperZ2.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("Z3:");
    snprintf(msg, 7, "%u\n", stepperZ3.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E0)
    SERIAL_ECHOPGM("E0:");
    snprintf(msg, 7, "%u\n", stepperE0.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E1)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("E1:");
    snprintf(msg, 7, "%u\n", stepperE1.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E2)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("E2:");
    snprintf(msg, 7, "%u\n", stepperE2.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E3)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("E3:");
    snprintf(msg, 7, "%u\n", stepperE2.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E4)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("E4:");
    snprintf(msg, 7, "%u\n", stepperE3.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
    #if AXIS_IS_TMC(E5)
    // #error dead code found by automatic analyses (see BFW-5461)
    SERIAL_ECHOPGM("E5:");
    snprintf(msg, 7, "%u\n", stepperE5.microsteps());
    SERIAL_ECHOPGM(msg);
    #endif
}
#endif // HAS_DRIVER(TMC2130) || HAS_DRIVER(TMC2209)
