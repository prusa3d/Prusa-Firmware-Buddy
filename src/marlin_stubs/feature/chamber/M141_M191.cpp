#include <marlin_stubs/PrusaGcodeSuite.hpp>

#include <feature/chamber/chamber.hpp>
#include <gcode/gcode_parser.hpp>
#include <common/temperature.hpp>
#include <module/planner.h>
#include <lcd/ultralcd.h> // Some marlin garbage dunno

using namespace buddy;

static void set_chamber_temperature(buddy::Temperature target, bool wait_for_heating, bool wait_for_cooling);
/**
 *### M141: Set chamber temperature <a href="https://reprap.org/wiki/G-code#M141:_Set_Chamber_Temperature_.28Fast.29">M141: Set Chamber Temperature (Fast)</a>
 *
 * #### Usage
 *
 *     M141 [ S ]
 *
 *#### Parameters
 *
 * - `S` - Target temperature, in degrees Celsius. 0 = no target temperature
 */

void PrusaGcodeSuite::M141() {
    using buddy::Temperature;

    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    if (const auto temp = p.option<Temperature>('S')) {
        set_chamber_temperature(*temp, false, false);
    }
}

/**
 *
 *### M191: Wait for chamber temperature <a href="https://reprap.org/wiki/G-code#M191:_Wait_for_chamber_temperature_to_reach_target_temp">M191: Wait for chamber temperature to reach target temp</a>
 *
 * #### Usage
 *
 *     M191 [ S | R | C ]
 *
 *#### Parameters
 *
 * - `S` - Target temperature in degrees Celsius. Wait only for heating.
 * - `R` - Target temperature in degrees Celsius. Wait for both cooling and heating.
 * - `C` - Target temperature in degrees Celsius. Wait only for cooling.
 */

void PrusaGcodeSuite::M191() {
    using buddy::Temperature;

    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    if (const auto opt = p.option_multikey<Temperature>({ 'S', 'R', 'C' })) {
        set_chamber_temperature(opt->first, (opt->second) != 'C', (opt->second) != 'S');
    }
}

static void set_chamber_temperature(buddy::Temperature target, bool wait_for_heating, bool wait_for_cooling) {
    using buddy::Temperature;

    if (!chamber().capabilities().temperature_control()) {
        SERIAL_ECHO("Chamber does not allow temperature control");
        return;
    }

    if (target == 0) {
        chamber().set_target_temperature({});
        return;
    }

    chamber().set_target_temperature(target);
    if (!wait_for_cooling && !wait_for_heating) {
        return;
    }

    uint32_t last_report_time = 0;

    while (true) {
        if (planner.draining()) {
            // We're aborting -> stop waiting
            break;
        }

        const auto current = chamber().current_temperature();
        static const Temperature tolerance = 3;

        const auto now = ticks_ms();
        if (ticks_diff(now, last_report_time) > 1000) {
            ui.set_status("Waiting for chamber temperature");
            last_report_time = now;
        }

        if (!current.has_value()) {
            // We don't know chamber temperature -> wait until we do

        } else if (std::abs(*current - target) <= tolerance) {
            // We're at the target -> done
            break;

        } else if (*current < target + tolerance && !wait_for_heating) {
            // We're cool and not waiting for heat -> done
            break;

        } else if (*current > target - tolerance && !wait_for_cooling) {
            // We're hot and not waiting for cooling ->done
            break;
        }

        idle(true);
    }
}
