/**
 * @file M123.cpp
 */

#include "M123.hpp"
#include "PrusaGcodeSuite.hpp"
#include "fanctl.hpp"
#include "../../lib/Marlin/Marlin/src/module/motion.h"

uint32_t M123::fan_auto_report_delay = 0;

/** \addtogroup G-Codes
 * @{
 */

/**
 * M123: Print fan speed on serial port. Without any parameters means print out fan telemetry to serial port once.
 *
 * ## Parameters
 *
 * - `S` - [value] Set fan auto report delay
 */

void PrusaGcodeSuite::M123() {
    if (parser.seen('S')) {
        M123::fan_auto_report_delay = parser.byteval('S');
    } else {
        M123::print_fan_speed();
    }
}

void M123::print_fan_speed() {
    char buffer[50];
    snprintf(buffer, sizeof buffer, "E0:%d RPM PRN1:%d RPM E0@:%u PRN1@:%u\n", Fans::heat_break(active_extruder).getActualRPM(), Fans::print(active_extruder).getActualRPM(), Fans::heat_break(active_extruder).getPWM(), Fans::print(active_extruder).getPWM());
    SERIAL_ECHO(buffer);
    SERIAL_EOL();
}

/** @}*/
