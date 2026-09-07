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

#include "../../inc/MarlinConfig.h"

#include <option/has_dwarf.h>
#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#include <option/has_bed_fan.h>
#if HAS_BED_FAN()
    #include <feature/bed_fan/controller.hpp>
#endif

#include <option/has_cpu_fan.h>
#if HAS_CPU_FAN()
    #include <cpu_fan_controller.hpp>
#endif

#include <option/has_xl_can.h>
#if HAS_XL_CAN()
    #include <hw/xl/modular_bed_fan.hpp>
#endif

#include <option/has_dwarf.h>
#if HAS_DWARF()
    #include <puppies/Dwarf.hpp>
#endif

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/temperature.h"
#include "fanctl.hpp"
#include <device/board.h>
#include <utils/variant_utils.hpp>
#include <option/xbuddy_extension_variant.h>
#include <pwm_utils.hpp>
#include <algorithm_scale.hpp>

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

/**
 * @brief Set fans that are not controlled by Marlin
 * @param fan Fan index (0-based)
 * @param tool tool number (0-based)
 * @param speed Fan speed (0-255)
 * @param set_auto true to set auto control
 * @return false to let Marlin process this fan as well, true to eat this G-code
 */
static bool set_special_fan_speed(uint8_t fan, std::optional<PhysicalToolIndex> tool, uint8_t speed, bool set_auto) {
    [[maybe_unused]] const auto pwm_or_auto = set_auto ? PWM255OrAuto(pwm_auto) : PWM255OrAuto(speed);

    switch (fan) {
#if HAS_TOOLCHANGER()
    case 1:
    #if HAS_DWARF()
        // Heatbreak fan
        if (tool.has_value()) {
            if (buddy::puppies::dwarfs[*tool].is_enabled()) {
                if (set_auto) {
                    buddy::puppies::dwarfs[*tool].set_fan_auto(1);
                } else {
                    buddy::puppies::dwarfs[*tool].set_fan(1, speed);
                }
            }
        }
    #endif
        return true; // Eat this G-code, heatbreak fan is not controlled by Marlin
#endif

#if XL_ENCLOSURE_SUPPORT()
    case 3:
        Fans::enclosure().set_pwm(speed);
        return true;
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        using XBE = buddy::XBuddyExtension;

    case 3:
        buddy::xbuddy_extension().set_fan_target_pwm(XBE::Fan::cooling_fan_1, pwm_or_auto);
        return true;

    case 4:
        buddy::xbuddy_extension().set_fan_target_pwm(XBE::Fan::filtration_fan, pwm_or_auto);
        return true;
#endif // XBUDDY_EXTENSION_VARIANT_IS_STANDARD()

#if HAS_BED_FAN()
    case 5:
        if (set_auto) {
            constexpr float default_temp_threshold = -1.0f; // disabled
            bed_fan::controller().set_mode(bed_fan::Controller::AutomaticMode {
                .max_pwm = parser.byteval('A', 255),
                .bed_temp_threshold = parser.celsiusval('B', default_temp_threshold),
                .chamber_temp_threshold = parser.celsiusval('C', default_temp_threshold),
            });
        } else {
            bed_fan::controller().set_mode(bed_fan::Controller::ManualMode {
                .pwm = speed,
            });
        }
        return true;
#endif

#if HAS_INDX()
    case 6:
        // Dock fan on the xBuddy NEXTRUDER print-fan pin (same controller as
        // the C1 print fan).
        Fans::dock_fan().set_pwm(speed);
        return true;
#endif

#if HAS_CPU_FAN()
    case 7:
        // CPU fan on the XLS sandwich board; on plain XL the pin is
        // unconnected, so the override is harmless there.
        cpu_fan_controller::set_target_pwm(pwm_or_auto);
        return true;
#endif

#if HAS_XL_CAN()
    case 8:
        // Modular Bed cooling fan on the XL-CAN bridge (XLS only); applied by
        // the marlin_server loop, which no-ops when the bridge is absent.
        buddy::ModularBedFanControl::instance().set_pwm_override(pwm_or_auto);
        return true;
#endif

    default:
        break;
    }

    return false;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M106: Set Fan Speed <a href="https://reprap.org/wiki/G-code#M106:_Fan_On">M106: Fan On</a>
 *
 *#### Usage
 *
 *   M106 [ S | P | T ]
 *   M106 P5 [ S | A B C ]  // Bed fan specific usage
 *
 *#### Parameters
 *
 * - `S` - Speed between 0-255
 * - `P` - Fan index, if more than one fan
 *     - `0` - Print fan
 *     - `1` - Heatbreak fan
 *     - `2` - ...
 *     - `3` - Cooling fan (if supported) or Enclosure fan (XL)
 *     - `4` - Filtration fan (if supported)
 *     - `5` - Bed Fan (if supported)
 *     - `6` - Dock fan (INDX only)
 *     - `7` - CPU fan (XLS only)
 *     - `8` - Modular Bed cooling fan (XLS only)
 * - `R` - Set the to auto control (if supported by the fan)
 * - `T` - Select which tool if the same fan is on multiple tools, active_extruder if not specified
 * - `N` - Ramp function breakpoint PWM for chamber fan regulator (0-255, P3/P4 only). See description below.
 * - `G` - Proportional gain (ramp_slope) for chamber fan regulator (float, P3/P4 only). See description below.
 * - `A` - Maximum PWM for automatic bed fan control (0-255, P5 only)
 * - `B` - Bed temperature threshold for automatic control (°C, P5 only)
 * - `C` - Chamber temperature threshold for automatic control (°C, P5 only)
 *
 *#### XBuddyExtension Chamber Fan Auto Control Logic (fans 3 & 4 on CORE ONE printers)
 * For compatibility reasons, two chamber fan regulators are implemented. New algorithm is more oriented on airflow stability,
 * while legacy algorithm is more oriented on absolute temperature control. On print start and end, the regulator is switched
 * to legacy mode to keep compatibility with old gcodes. When M106 with parameters N or G is sent, the regulator is switched to new algorithm.
 *
 *##### New algorithm
 * - Chamber Fan control algorithm is ramp function with hysteresis on top of it
 * - If temperature is below target, ramp function output is ramp_breakpoint_pwm (Parameter N)
 *   The minimal PWM is to ensure good airflow to cool the extruded material fast enough, which is necessary even when the chamber is on the target temperature.
 *   This minimal required airflow is material specific, and thus it has been exposed to be made configurable via gcode.
 *
 * - If temperature is above target, ramp function output is proportional to the error with slope ramp_slope (Parameter G)
 * - Hysteresis is applied on top of the ramp function to avoid fan premature kick-start and reduce kick-starts frequency
 * - The PWM output is also modified based on the filtration backend to adjust for different fan configurations
 *
 *##### Legacy algorithm
 * - Legacy Fan control algorithm is PID regulator with only I component used.
 *
 * !! This comment is also doubled in the FanCooling::compute_auto_regulation_step. If you do changes here, update the other one, too.
 *
 *#### Bed Fan (P5) Examples
 * - `M106 P5 S128` - Manual control at PWM 128 (50%)
 * - `M106 P5 R A200 B10` - Auto control, max PWM 200, bed threshold 10°C
 * - `M106 P5 R A180 C5` - Auto control, max PWM 180, chamber threshold 5°C
 * - `M106 P5 R A255 B10 C5` - Auto control, chamber priority if both targets set
 *
 *#### Bed Fan Auto Control Logic
 * - If chamber threshold (C) set and chamber target > 0: use chamber control
 * - Else if bed threshold (B) set and bed target > 0: use bed control
 * - Temperature difference > threshold: maximum PWM (A parameter)
 * - Temperature difference 0-threshold: linear scaling
 *
 *#### CPU and Modular Bed fans (P7/P8, XLS only)
 * - Both fans normally run under automatic temperature control
 * - `S` sets a manual PWM that replaces the automatic control until `M106 P<n> R` or reboot
 * - `M107 P<n>` sets manual PWM 0 (fan truly off); use `R` to return to automatic control
 *
 *Enclosure fan (index 3) don't support T parameter
 */
void GcodeSuite::M106() {
    const uint8_t p = parser.byteval('P', 0);

    const bool auto_control = parser.seen('R');
    if (parser.seen('S') || parser.seen('A') || auto_control) {
        const uint8_t speed = parser.byteval('S', 255);

        const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
        if (set_special_fan_speed(p, tool, speed, auto_control)) {
            // Done in the function

        } else if (p == 0 /* print fan */) {
            uint16_t d = parser.seen('A') ? thermalManager.print_fan_speed : 255;
            uint16_t s = parser.ushortval('S', d);
            NOMORE(s, 255U);
#if HAS_GCODE_COMPATIBILITY()
            if (gcode.compatibility.mk4_compatibility_mode) {
                s = (s * 7) / 10; // Converts speed to 70% of its values
            }
            if (gcode.compatibility.xl_compatibility_mode && s != 0) {
                // Interpolate PWM values in case XL gcode is executed on XLS printer
                // 0 means off, so keep it off - no remapping
                // Remapping explained in BFW-8540
                // XLS fan has at 33% PWM the same airflow as GOM at 40% PWM
                // XLS fan has at 60% PWM the same airflow as Delta at 100% PWM (Delta has a bit higher airflow than GOM)
                // Clamping applied outside of 20% - 100% of the original value
                s = scale<uint32_t>(s, 20 * 255 / 100, 100 * 255 / 100, 33 * 255 / 100, 60 * 255 / 100);
            }
#endif

            thermalManager.set_print_fan_speed(s);
        }
    }

    switch (p) {

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    case 3:
    case 4:
        if (parser.seen('N')) {
            buddy::xbuddy_extension().set_chamber_regulator_ramp_breakpoint_pwm(std::clamp<uint16_t>(parser.ushortval('N'), 0, 255));
        }
        if (parser.seen('G')) {
            buddy::xbuddy_extension().set_chamber_regulator_ramp_slope(parser.floatval('G'));
        }
        break;
#endif
    }
}

/**
 *### M107: Fan Off <a href="https://reprap.org/wiki/G-code#M107:_Fan_Off">M107: Fan Off</a>
 *
 *#### Usage
 *
 *    M107 [ P ]
 *
 *#### Parameters
 *
 * - `P` - Fan index
 *     - `0` - Print fan
 *     - `1` - Heatbreak fan
 *     - `2` - ...
 *     - `3` - Cooling fan (if supported) or Enclosure fan (XL)
 *     - `4` - Filtration fan (if supported)
 *     - `5` - Bed Fan (if supported)
 *     - `6` - Dock fan (INDX only)
 *     - `7` - CPU fan (XLS only); off until `M106 P7 R` reverts to automatic control
 *     - `8` - Modular Bed cooling fan (XLS only); off until `M106 P8 R` reverts to automatic control
 * - `T` - Select which tool if there are multiple fans, one on each tool
 */
void GcodeSuite::M107() {
    const uint8_t p = parser.byteval('P', 0);

    const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
    if (set_special_fan_speed(p, tool, 0, false)) {
        return;
    }

    if (p == 0 /* print fan */) {
        thermalManager.set_print_fan_speed(0);
    }
}

/** @}*/
