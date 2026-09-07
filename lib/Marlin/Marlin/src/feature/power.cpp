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
 * power.cpp - power control
 */

#include "../inc/MarlinConfig.h"

#if ENABLED(AUTO_POWER_CONTROL)

#include "power.h"
#include "../module/temperature.h"
#include "../module/stepper/indirection.h"
#include "../Marlin.h"

Power powerManager;

millis_t Power::lastPowerOn;

bool Power::is_power_needed() {
  #if ENABLED(AUTO_POWER_FANS)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (thermalManager.fan_speed[0]) return true;
  #endif

  // If any of the drivers or the bed are enabled...
  if (X_ENABLE_READ() == X_ENABLE_ON || Y_ENABLE_READ() == Y_ENABLE_ON
    #if POWER_IGNORE_Z
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      || Z_ENABLE_READ() == Z_ENABLE_ON
    #endif
    #if HAS_HEATED_BED
      || thermalManager.temp_bed.soft_pwm_amount > 0
    #endif
      #if HAS_Z2_ENABLE
        // #error dead code found by automatic analyses (see BFW-5461)
        || Z2_ENABLE_READ() == Z_ENABLE_ON
      #endif
      #if E_STEPPERS
        || E0_ENABLE_READ() == E_ENABLE_ON
        #if E_STEPPERS > 1
          // #error dead code found by automatic analyses (see BFW-5461)
          || E1_ENABLE_READ() == E_ENABLE_ON
          #if E_STEPPERS > 2
            // #error dead code found by automatic analyses (see BFW-5461)
            || E2_ENABLE_READ() == E_ENABLE_ON
            #if E_STEPPERS > 3
              // #error dead code found by automatic analyses (see BFW-5461)
              || E3_ENABLE_READ() == E_ENABLE_ON
              #if E_STEPPERS > 4
                // #error dead code found by automatic analyses (see BFW-5461)
                || E4_ENABLE_READ() == E_ENABLE_ON
                #if E_STEPPERS > 5
                  // #error dead code found by automatic analyses (see BFW-5461)
                  || E5_ENABLE_READ() == E_ENABLE_ON
                #endif // E_STEPPERS > 5
              #endif // E_STEPPERS > 4
            #endif // E_STEPPERS > 3
          #endif // E_STEPPERS > 2
        #endif // E_STEPPERS > 1
      #endif // E_STEPPERS
  ) return true;

  for (auto tool : PhysicalToolIndex::all()) {
    const auto &hotend = Hotend::for_tool(tool);
    if (hotend.nozzle_target_temp() > 0 || hotend.nozzle_heater_pwm() > PWM255(0)) {
      return true;
    }
  }

  #if HAS_HEATED_BED
    if (thermalManager.degTargetBed() > 0 || thermalManager.temp_bed.soft_pwm_amount > 0) return true;
  #endif

  return false;
}

void Power::check() {
  static millis_t nextPowerCheck = 0;
  millis_t ms = millis();
  if (ELAPSED(ms, nextPowerCheck)) {
    nextPowerCheck = ms + 2500UL;
    if (is_power_needed())
      power_on();
    else if (!lastPowerOn || ELAPSED(ms, lastPowerOn + (POWER_TIMEOUT) * 1000UL))
      power_off();
  }
}

void Power::power_on() {
  lastPowerOn = millis();
  if (!powersupply_on) {
    PSU_PIN_ON();
  }
}

void Power::power_off() {
  if (powersupply_on) PSU_PIN_OFF();
}

#endif // AUTO_POWER_CONTROL
