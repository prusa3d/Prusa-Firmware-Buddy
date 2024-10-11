#include "../../inc/MarlinConfigPre.h"
#include "../../module/temperature.h"
#include "../../module/motion.h"
#include "../../feature/print_area.h"
#include "../../Marlin.h"
#include "../gcode.h"
#include "../queue.h"
#include <cstdio>
#include <stdarg.h>

#if ENABLED(GCODE_COMPATIBILITY_MK3)

static void run_gcode(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char gcode_buffer[64];
    vsnprintf(gcode_buffer, sizeof(gcode_buffer), fmt, args);
    gcode.process_subcommands_now(gcode_buffer);
    va_end(args);
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G80: Mesh Bed Leveling (MK3.5, MK3.9, MK4/S) <a href="https://reprap.org/wiki/G-code#G80:_Mesh-based_Z_probe">G80: Mesh-based Z probe</a>
 *
 * MK3.5, MK3.9 and MK4 Only
 *
 *#### Usage
 *
 *    G80
 */
void GcodeSuite::G80() {
    const int target_extruder =
    #if EXTRUDERS > 1
        active_extruder;
    #else
        0;
    #endif

    // calc target temperatures
    float print_temp = thermalManager.degTargetHotend(active_extruder);
    float mbl_temp;
    if (print_temp >= 250) {
        mbl_temp = print_temp - 25;
    } else if (print_temp > 170) {
        mbl_temp = 170;
    } else {
        mbl_temp = print_temp;
    }

    // set print area to MK3's bed size (mostly for XL)
    print_area.set_bounding_rect({ 0, 0, 250, 210 });

    // move to wait position
    run_gcode("G1 X32 Y-4 Z40");
    // set mbl temp
    run_gcode("M109 R%.0f T%i", mbl_temp, target_extruder);
    // cleanup nozzle
    run_gcode("G1 X32 Y-4 Z5");
    run_gcode("G29 P9 X0 Y-4 W32 H4");
    // small retraction
    run_gcode("G1 E-2 F2400");
    // turn off the motor
    run_gcode("M84 E");
    // fan off?
    // ...
    // MBL
    run_gcode("G29");
    // move away from the bed in Z
    run_gcode("G1 Z30");
    // set print temperature back
    run_gcode("M109 S%.0f T%i", print_temp, target_extruder);
    // deretraction
    run_gcode("G1 E2 F2400");


    // If running in MK3 compatibility mode, we need to move z axis down to print bed.
    // When running G80 in MK3 we moved the nozzle to the printbed after MBL.
    // We don't do that now and the newer slicer adds G1 instruction to move the nozzle down.
    if (gcode.gcode_compatibility_mode == GcodeCompatibilityMode::MK3) {
        run_gcode("G1 Z0.15"); // 0.15 is value of Z_MIN_POS https://github.com/prusa3d/Prusa-Firmware/blob/MK3/Firmware/variants/MK3S.h#L67
    }
}

/** @}*/

#endif
