/// @file
#include "standard_hotend_regulator.hpp"

#include <module/temperature.h>
#include <module/temperature/steady_state_hotend.hpp>

static_assert(ENABLED(PIDTEMP), "Not supported anymore");
static_assert(DISABLED(PID_OPENLOOP), "Not supported anymore");

HotendRegulatorResult StandardHotendRegulator::get_pid_output_hotend(const HotendRegulatorArgs &args) {
    const float pid_error = args.target_temp - args.current_temp;

    float pid_output;
    float feed_forward = 0;

    if (args.target_temp == 0 || pid_error < -(PID_FUNCTIONAL_RANGE) || args.reset_pid) {
        pid_output = 0;
        pid_reset = true;
    } else if (pid_error > PID_FUNCTIONAL_RANGE) {
        pid_output = BANG_MAX;
        pid_reset = true;
    } else {
        if (pid_reset) {
            temp_iState = 0.0;
            work_pid.Kd = 0.0;
            temp_dState = pid_error;
            pid_reset = false;
        }
        work_pid.Kd = work_pid.Kd + PID_K2 * (args.pid.Kd * (pid_error - temp_dState) - work_pid.Kd);
        work_pid.Kp = args.pid.Kp * pid_error;
        pid_output = work_pid.Kp + float(MIN_POWER);

#if ENABLED(STEADY_STATE_HOTEND)
        static constexpr float pid_max_inv = 1.0f / PID_MAX;
        feed_forward = steady_state_hotend(args.target_temp, args.fan_speed * pid_max_inv);
        pid_output += feed_forward;
#endif

#if ENABLED(PID_EXTRUSION_SCALING)
        work_pid.Kc = args.e_volume_delta * (args.current_temp - ambient_temp) * args.pid.Kc;
        pid_output += work_pid.Kc;
#endif // PID_EXTRUSION_SCALING

        // Sum error only if it has effect on output value before D term is applied
        if (!((((pid_output + work_pid.Ki) < 0) && (pid_error < 0))
                || (((pid_output + work_pid.Ki) > PID_MAX) && (pid_error > 0)))) {
            temp_iState += pid_error;
        }
        work_pid.Ki = args.pid.Ki * temp_iState;
        pid_output += work_pid.Ki + work_pid.Kd;

        LIMIT(pid_output, 0, PID_MAX);
    }
    temp_dState = pid_error;

#if ENABLED(PID_DEBUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (args.hotend_index == active_extruder) {
        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR(
            MSG_PID_DEBUG, args.hotend_index,
            MSG_PID_DEBUG_INPUT, args.current_temp,
            MSG_PID_DEBUG_OUTPUT, pid_output);
        SERIAL_ECHOPAIR(
    #if ENABLED(STEADY_STATE_HOTEND)
            // #error dead code found by automatic analyses (see BFW-5461)
            " fTerm ", feed_forward,
    #endif
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
