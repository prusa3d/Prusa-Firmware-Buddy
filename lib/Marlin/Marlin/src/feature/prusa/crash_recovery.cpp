#include "inc/MarlinConfigPre.h"

#if ENABLED(CRASH_RECOVERY)

    #include "../../module/stepper.h"
    #include "crash_recovery.h"
    #include "bsod.h"
    #include "eeprom.h"
    #include "../../module/printcounter.h"
    #include "metric.h"

Crash_s &crash_s = Crash_s::instance();

Crash_s &Crash_s::instance() {
    static Crash_s s;
    return s;
}

Crash_s::Crash_s()
    : home_sensitivity { X_STALL_SENSITIVITY, Y_STALL_SENSITIVITY, Z_STALL_SENSITIVITY },
      m_axis_is_homing{false, false},
      m_enable_stealth{false, false} {
    reset();
    enabled = variant8_get_bool(eeprom_get_var(EEVAR_CRASH_ENABLED));
    max_period.x = variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_PERIOD_X));
    max_period.y = variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_PERIOD_Y));
    sensitivity.x = variant8_get_i16(eeprom_get_var(EEVAR_CRASH_SENS_X));
    sensitivity.y = variant8_get_i16(eeprom_get_var(EEVAR_CRASH_SENS_Y));
#if HAS_DRIVER(TMC2130)
    filter = variant8_get_bool(eeprom_get_var(EEVAR_CRASH_FILTER));
#endif
}

// Called from ISR
void Crash_s::stop_and_save() {
    // freeze motion first
    stepper.suspend();
    loop = true;

    // get the current live block
    const block_t *current_block = stepper.block();
    float e_position;

    if (!current_block && planner.movesplanned()) {
        // there's no live block, attempt to fetch the next block from the planner
        current_block = &planner.block_buffer[planner.block_buffer_tail];
    }

    if (current_block) {
        // recover the correct crash block
        const uint8_t crash_index = current_block - planner.block_buffer;
        const Crash_s::crash_block_t &crash_block = crash_s.crash_block[crash_index];

        // save current_block state
        sdpos = crash_block.sdpos;
        segments_finished = crash_block.segment_idx;
        inhibit_flags = crash_block.inhibit_flags;
        fr_mm_s = crash_block.fr_mm_s;
    #if ENABLED(LIN_ADVANCE)
        advance_mm = stepper.get_LA_steps() * Planner::mm_per_step[E_AXIS];
    #endif
        start_current_position = crash_block.start_current_position;

        // recover delta E position
        float d_e_steps = crash_block.e_steps * stepper.segment_progress();
        e_position = crash_block.e_position + d_e_steps * planner.mm_per_step[E_AXIS_N(active_extruder)];
    } else {
        // no block, get state from the queue & planner
        sdpos = queue.get_current_sdpos();
        segments_finished = 0;
        inhibit_flags = gcode_state.inhibit_flags;
        fr_mm_s = feedrate_mm_s;
    #if ENABLED(LIN_ADVANCE)
        advance_mm = 0;
    #endif
        start_current_position = current_position;
        e_position = current_position[E_AXIS];
    }

    // save planner state
    leveling_active = planner.leveling_active;
    crash_axis_known_position = axes_trusted;
    // TODO: this is incomplete, as some of the planner state is ahead of the stepper state
    //       marlin state is also not saved, notably: absolute/relative axis state
    //marlin_server.motion_param.save();

    // stop any movement: this will discard any planner state!
    planner.quick_stop();
    planner.reset_position();
    crash_position = planner.get_machine_position_mm();

    // Due to e_factor not being reflected in the physical positioning (allowing the stepper position
    // to drift), we cannot easily recover an absolute E position that makes sense. We thus work in
    // segment-relative offsets, requiring us to store extra state.
    crash_position[E_AXIS] = e_position;

    // update crash_current_position. WARNING: this is NOT intended to be fully reversible (doing so
    // would require keeping more state), it's only usable to abort or return to the same position.
    crash_current_position = planner.get_axis_positions_mm();

    #if HAS_POSITION_MODIFIERS
    planner.unapply_modifiers(crash_current_position
        #if HAS_LEVELING
        ,
        true
        #endif
    );
    #endif
}

void check_loop() {
    if (crash_s.loop) {
        // guard against incomplete buffer flushing: if we reach this point, the state machine is
        // incorrectly being handled recursively (double ungood)
        bsod("reentrant recovery");
    }
}

void Crash_s::resume_movement() {
    // at this point leveling should be off until the print is restored
    planner.leveling_active = false;

    // order is important here! set an approximate current position which is only good enough for
    // re-homing and guarantees no changes with a zero offset from current_position.
    current_position = crash_current_position;
    planner.set_position_mm(current_position);

    check_loop();
    planner.resume_queuing();
}

void Crash_s::restore_state() {
    if (inhibit_flags & INHIBIT_PARTIAL_REPLAY)
        segments_finished = 0;

    if (inhibit_flags & INHIBIT_XYZ_REPOSITIONING) {
        // also reset internal crash locations to current_position
        LOOP_NUM_AXES(i) {
            start_current_position[i] = current_position[i];
            crash_position[i] = current_position[i];
        }
    }

    // order is important here!
    planner.leveling_active = leveling_active;
    current_position = start_current_position;
    planner.set_position_mm(current_position);

    // restore additional queue parameters
    feedrate_mm_s = fr_mm_s;

    check_loop();
    planner.resume_queuing();
}

void Crash_s::set_state(state_t new_state) {
    if (state == new_state) {
        switch (state) {
        case IDLE:
            bsod("crash idle");
        case RECOVERY:
            bsod("crash recovery");
        case REPLAY:
            bsod("crash replay");
        case TRIGGERED_ISR:
        case TRIGGERED_AC_FAULT:
            bsod("crash double trigger");
        case PRINTING:
            bsod("crash printing");
        }
    }

    switch (new_state) {
    case IDLE:
        reset();
        break;

    case TRIGGERED_ISR:
        if (state != PRINTING || !is_active() || !is_enabled())
            bsod("crash not active");
        // FALLTHRU
    case TRIGGERED_AC_FAULT:
        // transition to AC_FAULT is _always_ possible from any state
        stop_and_save();
        break;

    case RECOVERY:
        // TODO: the following checks are too broad (should check for existing state)
        if (state != PRINTING && state != TRIGGERED_ISR && state != TRIGGERED_AC_FAULT)
            bsod("invalid recovery transition");
        resume_movement();
        break;

    case REPLAY:
        if (state != RECOVERY)
            bsod("invalid replay transition");
        activate();
        restore_state();
        break;

    case PRINTING:
        if (state != RECOVERY && state != IDLE && state != REPLAY)
            bsod("invalid printing transition");
        reset_repeated_crash();
        if (state != REPLAY)
            activate();
        break;
    }

    state = new_state;
}
/**
 * @brief Update sensitivity and threshold to drivers
 */
void Crash_s::update_machine() {
    if(!m_axis_is_homing[0])
    {
        if (enabled) {
            stepperX.stall_max_period(0);
            #if AXIS_DRIVER_TYPE_X(TMC2130)
                stepperX.sfilt(filter);
            #endif
            stepperX.stall_sensitivity(crash_s.sensitivity.x);
            stepperX.stall_max_period(crash_s.max_period.x);
        } else {
            tmc_disable_stallguard(stepperX, m_enable_stealth[0]);
        }
    }
    if(!m_axis_is_homing[1]) {
        if (enabled) {
            stepperY.stall_max_period(0);
            #if AXIS_DRIVER_TYPE_Y(TMC2130)
                stepperY.sfilt(filter);
            #endif
            stepperY.stall_sensitivity(crash_s.sensitivity.y);
            stepperY.stall_max_period(crash_s.max_period.y);
        } else {
            tmc_disable_stallguard(stepperY, m_enable_stealth[1]);
        }
    }
}

void Crash_s::enable(bool state) {
    if (state == enabled)
        return;
    enabled = state;
    eeprom_set_var(EEVAR_CRASH_ENABLED, variant8_bool(state));
    update_machine();
}

void Crash_s::set_sensitivity(xy_long_t sens) {
    if (sensitivity != sens) {
        sensitivity = sens;
        eeprom_set_var(EEVAR_CRASH_SENS_X, variant8_i16(sensitivity.x));
        eeprom_set_var(EEVAR_CRASH_SENS_Y, variant8_i16(sensitivity.y));
        update_machine();
    }
}

void Crash_s::reset_crash_counter() {
    counter_crash = xy_uint_t({ 0, 0 });
    counter_power_panic = 0;
    stats_saved = false;
}

void Crash_s::send_reports() {
    if (stats_saved)
        reset_crash_counter();

    if (axis_hit != X_AXIS && axis_hit != Y_AXIS)
        return;

    float speed = -1;
    if (axis_hit == X_AXIS) {
        speed = period_to_speed(X_MICROSTEPS, int(stepperX.TSTEP()), get_steps_per_unit_x());
    }
    if (axis_hit == Y_AXIS) {
        speed = period_to_speed(Y_MICROSTEPS, int(stepperY.TSTEP()), get_steps_per_unit_y());
    }

    static metric_t crash_metric = METRIC("crash", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    metric_record_custom(&crash_metric, ",axis=%c sens=%ii,period=%ui,speed=%.3f", axis_codes[axis_hit], sensitivity.pos[axis_hit], max_period.pos[axis_hit], (double)speed);
}

void Crash_s::set_max_period(xy_long_t mp) {
    if (max_period != mp) {
        max_period = mp;
        eeprom_set_var(EEVAR_CRASH_PERIOD_X, variant8_ui16(max_period.x));
        eeprom_set_var(EEVAR_CRASH_PERIOD_Y, variant8_ui16(max_period.y));
        update_machine();
    }
}

void Crash_s::write_stat_to_eeprom() {
    if (stats_saved)
        return;
    stats_saved = true;
    xy_uint_t total({ variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_COUNT_X_TOT)), variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_COUNT_Y_TOT)) });
    uint16_t power_panics = variant8_get_ui16(eeprom_get_var(EEVAR_POWER_COUNT_TOT));

    xy_long_t eevar = { EEVAR_CRASH_COUNT_X_TOT, EEVAR_CRASH_COUNT_Y_TOT };
    LOOP_XY(axis) {
        if (counter_crash.pos[axis] > 0) {
            total.pos[axis] += counter_crash.pos[axis];
            eeprom_set_var((enum eevar_id)eevar.pos[axis], variant8_ui16(total.pos[axis]));
            static metric_t crash_stat = METRIC("crash_stat", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
            metric_record_custom(&crash_stat, ",axis=%c last=%ui,total=%ui", axis_codes[axis], counter_crash.pos[axis], total.pos[axis]);
        }
    }
    power_panics += counter_power_panic;
    eeprom_set_var(EEVAR_POWER_COUNT_TOT, variant8_ui16(power_panics));

    reset_crash_counter();
}

uint32_t Crash_s::clean_history() {
    int valid = 0;
    for (auto &ts : crash_timestamps) {
        if (ts.has_value() && (print_job_timer.duration() - ts.value() <= CRASH_TIMER)) {
            ++valid;
        } else {
            ts = std::nullopt;
        }
    }
    return valid;
}

void Crash_s::reset_history() {
    for (auto &t : crash_timestamps)
        t = std::nullopt;
}

void Crash_s::count_crash() {
    if (axis_hit == X_AXIS || axis_hit == Y_AXIS)
        ++counter_crash.pos[axis_hit];

    uint32_t valid = clean_history();
    if (valid == crash_timestamps.size()) {
        repeated_crash = true;
        static metric_t crash_repeated = METRIC("crash_repeated", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
        metric_record_event(&crash_repeated);
    }
    crash_timestamps[crash_timestamps_idx++] = print_job_timer.duration();
    crash_timestamps_idx %= crash_timestamps.size();
}

void Crash_s::reset() {
    reset_history();
    repeated_crash = false;
    reset_crash_counter();
    segments_finished = 0;
    inhibit_flags = 0;
    state = IDLE;
    vars_locked = false;
    active = false;
    axis_hit = NO_AXIS_ENUM;
}

void Crash_s::start_sensorless_homing_per_axis(const AxisEnum axis) {
    if (axis < (sizeof(m_axis_is_homing) / sizeof(m_axis_is_homing[0]))) {
        m_axis_is_homing[axis] = true;
        if (X_AXIS == axis) {
            stepperX.stall_sensitivity(crash_s.home_sensitivity[0]);
        }
        else if (Y_AXIS == axis) {
            stepperY.stall_sensitivity(crash_s.home_sensitivity[1]);
        }
    }
}

/**
 */
void Crash_s::end_sensorless_homing_per_axis(const AxisEnum axis, const bool enable_stealth) {
    if (axis < (sizeof(m_axis_is_homing) / sizeof(m_axis_is_homing[0]))) {
        m_axis_is_homing[axis] = false;
        m_enable_stealth[axis] = enable_stealth;
        update_machine();
    }
}

    #if HAS_DRIVER(TMC2130)
        void Crash_s::set_filter(bool on) {
            if (filter == on)
                return;
            filter = on;
            eeprom_set_var(EEVAR_CRASH_FILTER, variant8_bool(on));
            update_machine();
        }
    #endif
#endif // ENABLED(CRASH_RECOVERY)
