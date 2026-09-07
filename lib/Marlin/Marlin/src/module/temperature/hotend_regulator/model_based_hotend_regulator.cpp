/// @file
#include "model_based_hotend_regulator.hpp"

#include <module/temperature/steady_state_hotend.hpp>
#include <module/temperature.h>

static_assert(ENABLED(PIDTEMP), "Not supported anymore");
static_assert(DISABLED(PID_OPENLOOP), "Not supported anymore");

//! @brief Get model output hotend
//!
//! @param args contains target/current temperature, fan speed and other info relevant for this cycle
float ModelBasedHotendRegulator::get_model_output_hotend(const HotendRegulatorArgs &args) {
    if (args.target_temp > (target_temp + epsilon)) {
        if (state != Ramp::Up) {
            delay = transport_delay_cycles;
            expected_temp = target_temp;
            state = Ramp::Up;
        }
        //! Target for less than full power, so regulator can catch
        //! with generated temperature curve at minimum voltage
        //! (rated - 5%) = 22.8 V and maximum heater resistance
        //! of 15.1 Ohm. P = 22.8/15.1*22.8 = 34.43 W
        //! = 86% P(rated)
        constexpr float target_heater_pwm = PID_MAX * 0.8607f;
        const float temp_diff = deg_per_cycle * pid_max_inv
            * (target_heater_pwm - steady_state_hotend(target_temp, args.fan_speed * pid_max_inv));
        target_temp += temp_diff;
        if (delay > 1) {
            --delay;
        }
        expected_temp += temp_diff / delay;
        if (target_temp > args.target_temp) {
            target_temp = args.target_temp;
        }
        return target_heater_pwm;

    } else if (args.target_temp < (target_temp - epsilon)) {
        if (state != Ramp::Down) {
            delay = transport_delay_cycles;
            expected_temp = target_temp;
            state = Ramp::Down;
        }
        const float temp_diff = deg_per_cycle * pid_max_inv
            * steady_state_hotend(target_temp, args.fan_speed * pid_max_inv);
        target_temp -= temp_diff;
        if (delay > 1) {
            --delay;
        }
        expected_temp -= temp_diff / delay;
        if (target_temp < args.target_temp) {
            target_temp = args.target_temp;
        }
        return 0;

    } else {
        state = Ramp::None;
        target_temp = args.target_temp;
        const float remaining = target_temp - expected_temp;
        if (expected_temp > (target_temp + epsilon)) {
            float diff = remaining * transport_delay_cycles_inv;
            if (abs(diff) < epsilon) {
                diff = -epsilon;
            }
            expected_temp += diff;
        } else if (expected_temp < (target_temp - epsilon)) {
            float diff = remaining * transport_delay_cycles_inv;
            if (abs(diff) < epsilon) {
                diff = epsilon;
            }
            expected_temp += diff;
        } else {
            expected_temp = target_temp;
        }
        return steady_state_hotend(target_temp, args.fan_speed * pid_max_inv);
    }
}

HotendRegulatorResult ModelBasedHotendRegulator::get_pid_output_hotend(const HotendRegulatorArgs &args) {
    float pid_output;
    float feed_forward = 0;

    if (args.target_temp == 0 || args.reset_pid) {
        pid_output = 0;
        pid_reset = true;
    } else {
        if (pid_reset) {
            temp_iState = 0.0;
            work_pid.Kd = 0.0;
            target_temp = args.current_temp;
            expected_temp = args.current_temp;
            pid_reset = false;
        }
        feed_forward = get_model_output_hotend(args);

        const float pid_error = expected_temp - args.current_temp;
        work_pid.Kd = work_pid.Kd + PID_K2 * (args.pid.Kd * (pid_error - temp_dState) - work_pid.Kd);
        work_pid.Kp = args.pid.Kp * pid_error;

        pid_output = feed_forward + work_pid.Kp + work_pid.Kd + float(MIN_POWER);

#if ENABLED(PID_EXTRUSION_SCALING)
        work_pid.Kc = args.e_volume_delta * (args.target_temp - ambient_temp) * args.pid.Kc;
        pid_output += work_pid.Kc;
    #if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
        feed_forward += work_pid.Kc;
    #endif
#endif // PID_EXTRUSION_SCALING

        // Sum error only if it has effect on output value
        if (!((((pid_output + work_pid.Ki) < 0) && (pid_error < 0))
                || (((pid_output + work_pid.Ki) > PID_MAX) && (pid_error > 0)))) {
            temp_iState += pid_error;
        }
        work_pid.Ki = args.pid.Ki * temp_iState;
        pid_output += work_pid.Ki;

        temp_dState = pid_error;
        LIMIT(pid_output, 0, PID_MAX);
    }

#if ENABLED(PID_DEBUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (args.hotend_index == active_extruder) {
        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR(
            MSG_PID_DEBUG, args.hotend_index,
            MSG_PID_DEBUG_INPUT, args.current_temp,
            MSG_PID_DEBUG_OUTPUT, pid_output);
        SERIAL_ECHOPAIR(
            " target ", expected_temp,
            " fTerm ", feed_forward,
            MSG_PID_DEBUG_PTERM, work_pid.Kp,
            MSG_PID_DEBUG_ITERM, work_pid.Ki,
            MSG_PID_DEBUG_DTERM, work_pid.Kd
    #if ENABLED(PID_EXTRUSION_SCALING)
            // #error dead code found by automatic analyses (see BFW-5461)
            ,
            MSG_PID_DEBUG_CTERM, work_pid.Kc
    #endif
        );
        SERIAL_EOL();
    }
#endif // PID_DEBUG

    return HotendRegulatorResult {
        .pid_output = pid_output,
        .feed_forward = feed_forward,
    };
}
