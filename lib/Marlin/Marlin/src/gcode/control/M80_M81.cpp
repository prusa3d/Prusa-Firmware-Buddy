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

#include "../gcode.h"
#include "../../module/temperature.h"
#include "../../module/stepper.h"
#include "../../module/printcounter.h" // for print_job_timer

#include "../../inc/MarlinConfig.h"

#if HAS_LCD_MENU
  #include "../../lcd/ultralcd.h"
#endif

#if HAS_SUICIDE
  #include "../../Marlin.h"
#endif

#if HAS_POWER_SWITCH

  #if ENABLED(AUTO_POWER_CONTROL)
    #include "../../feature/power.h"
  #endif

  // Could be moved to a feature, but this is all the data
  bool powersupply_on;

  #if HAS_TRINAMIC
    #include "../../feature/tmc_util.h"
  #endif

  /** \addtogroup G-Codes
   * @{
   */

  /**
   *### M80: Turn on the Power Supply <a href="https://reprap.org/wiki/G-code#M80:_ATX_Power_On">M80: ATX Power On</a>
   *
   * Only MK3.5/S, MK3.9/S, MK4/S and iX
   *
   *#### Usage
   *
   *    M80 [ S ]
   *
   *#### Parameters
   *
   * - `S` - Report the current state and exit
   */
  void GcodeSuite::M80() {

    // S: Report the current power supply state and exit
    if (parser.seen('S')) {
      serialprintPGM(powersupply_on ? PSTR("PS:1\n") : PSTR("PS:0\n"));
      return;
    }

    PSU_ON();

    /**
     * If you have a switch on suicide pin, this is useful
     * if you want to start another print with suicide feature after
     * a print without suicide...
     */
    #if HAS_SUICIDE
      OUT_WRITE(SUICIDE_PIN, HIGH);
    #endif

    #if DISABLED(AUTO_POWER_CONTROL)
      delay(100); // Wait for power to settle
      restore_stepper_drivers();
    #endif

    #if HAS_LCD_MENU
      ui.reset_status();
    #endif
  }

#endif // HAS_POWER_SWITCH

/**
 *### M81: Turn off Power <a href="https://reprap.org/wiki/G-code#M81:_ATX_Power_Off">M81: ATX Power Off</a>
 *
 * This code should ALWAYS be available for FULL SHUTDOWN!
 *
 *#### Usage
 *
 *    M81
 *
 */
void GcodeSuite::M81() {
  thermalManager.disable_all_heaters();
  print_job_timer.stop();
  planner.finish_and_disable();

  #if FAN_COUNT > 0
    thermalManager.zero_fan_speeds();
    #if ENABLED(PROBING_FANS_OFF)
      thermalManager.fans_paused = false;
      ZERO(thermalManager.saved_fan_speed);
    #endif
  #endif

  safe_delay(1000); // Wait 1 second before switching off

  #if HAS_SUICIDE
    suicide();
  #elif HAS_POWER_SWITCH
    PSU_OFF();
  #endif

  #if HAS_LCD_MENU
    LCD_MESSAGEPGM_P(PSTR(MACHINE_NAME " " MSG_OFF "."));
  #endif
}

/** @}*/
