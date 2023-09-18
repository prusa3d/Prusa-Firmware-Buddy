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
 * G29.cpp - Unified Bed Leveling
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(AUTO_BED_LEVELING_UBL)

#include "../../gcode.h"
#include "../../../feature/bedlevel/bedlevel.h"
#include "../../../feature/prusa/e-stall_detector.h"

#if ENABLED(CRASH_RECOVERY)
    #include "../../../feature/prusa/crash_recovery.hpp"
#endif

void GcodeSuite::G29() {
    BlockEStallDetection block_e_stall_detection;
    #if ANY(CRASH_RECOVERY, POWER_PANIC)
      // G29 requires a full restart: inhibit partial replay
      crash_s.inhibit_gcode_replay();
    #endif

    ubl.G29();
}

#endif // AUTO_BED_LEVELING_UBL
