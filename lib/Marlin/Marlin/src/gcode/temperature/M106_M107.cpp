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

#if FAN_COUNT > 0

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/temperature.h"
#include "fanctl.hpp"
#include <device/board.h>
#include <option/has_xbuddy_extension.h>

#if HAS_XBUDDY_EXTENSION()
  #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#if ENABLED(SINGLENOZZLE)
  #define _ALT_P active_extruder
  #define _CNT_P EXTRUDERS
#elif ENABLED(PRUSA_TOOLCHANGER)
  #define _ALT_P 0
  #define _CNT_P FAN_COUNT
#else
  #define _ALT_P _MIN(active_extruder, FAN_COUNT - 1)
  #define _CNT_P FAN_COUNT
#endif

static bool set_special_fan_speed(uint8_t fan, uint8_t speed) {
#if XL_ENCLOSURE_SUPPORT()
    static_assert(FAN_COUNT < 3, "Fan index 3 is reserved for Enclosure fan and should not be set by thermalManager");
    if (fan == 3) {
        Fans::enclosure().setPWM(speed);
        return true;
    }
#endif

#if HAS_XBUDDY_EXTENSION()
  static_assert(FAN_COUNT < 3, "Fan 3 is dedicated to extboard");
  if(fan == 3) {
    buddy::xbuddy_extension().set_fan1_fan2_pwm(speed);
    return true;
  }
#endif

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
 *
 *#### Parameters
 *
 * - `S` - Speed between 0-255
 * - `P` - Fan index, if more than one fan
 * - `A` - ???
 * - `T` - Restore/Use/Set Temporary Speed: (With EXTRA_FAN_SPEED enabled:)
 *   - `1` - Restore previous speed after T2
 *   - `2` - Use temporary speed set with T3-255
 *   - `3-255`- Set the speed for use with T2
 *Enclosure fan (index 3) don't support T parameter
 */
void GcodeSuite::M106() {
    const uint8_t p = parser.byteval('P', _ALT_P);

    if(set_special_fan_speed(p, std::clamp<uint16_t>(parser.ushortval('S', 255), 0, 255))) {
      return;
    }

    if (p < _CNT_P) {

    #if ENABLED(EXTRA_FAN_SPEED)
        const uint16_t t = parser.intval('T');
        if (t > 0) {
            return thermalManager.set_temp_fan_speed(p, t);
        }
    #endif
        uint16_t d = parser.seen('A') ? thermalManager.fan_speed[0] : 255;
        uint16_t s = parser.ushortval('S', d);
        NOMORE(s, 255U);
    #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
        if (GcodeSuite::fan_compatibility_mode == GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S) {
            s = (s * 7) / 10; // Converts speed to 70% of its values
        }
    #endif


        thermalManager.set_fan_speed(p, s);
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
 */
void GcodeSuite::M107() {
  const uint8_t p = parser.byteval('P', _ALT_P);

  if(set_special_fan_speed(p, 0)) {
    return;
  }

  thermalManager.set_fan_speed(p, 0);
}

/** @}*/

#endif // FAN_COUNT > 0
