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
#pragma once

/**
 * probe.h - Move, deploy, enable, etc.
 */

#include "../inc/MarlinConfig.h"

#if HAS_BED_PROBE

  constexpr xyz_pos_t nozzle_to_probe_offset = NOZZLE_TO_PROBE_OFFSET;

  extern xyz_pos_t probe_offset;

  bool set_probe_deployed(const bool deploy);
  #ifdef Z_AFTER_PROBING
    void move_z_after_probing();
  #endif
  enum ProbePtRaise : unsigned char {
    PROBE_PT_NONE,  // No raise or stow after run_z_probe
    PROBE_PT_STOW,  // Do a complete stow after run_z_probe
    PROBE_PT_RAISE, // Raise to "between" clearance after run_z_probe
    PROBE_PT_BIG_RAISE  // Raise to big clearance after run_z_probe
  };
  float run_z_probe(float expected_trigger_z, bool single_only = false, bool *endstop_triggered = nullptr);
  float probe_here(float z_down_limit);
  float probe_at_point(const float &rx, const float &ry, const ProbePtRaise raise_after=PROBE_PT_NONE, const uint8_t verbose_level=0, const bool probe_relative=true);
  inline float probe_at_point(const xy_pos_t &pos, const ProbePtRaise raise_after=PROBE_PT_NONE, const uint8_t verbose_level=0, const bool probe_relative=true) {
    return probe_at_point(pos.x, pos.y, raise_after, verbose_level, probe_relative);
  }
  #if ENABLED(NOZZLE_LOAD_CELL) && ENABLED(PROBE_CLEANUP_SUPPORT)
    void cleanup_probe(const xy_pos_t &rect_min, const xy_pos_t &rect_max);
  #endif
  #define DEPLOY_PROBE() set_probe_deployed(true)
  #define STOW_PROBE() set_probe_deployed(false)
  #if HAS_HEATED_BED && ENABLED(WAIT_FOR_BED_HEATER)
    extern const char msg_wait_for_bed_heating[25];
  #endif

#else

  constexpr xyz_pos_t probe_offset{0};

  #define DEPLOY_PROBE()
  #define STOW_PROBE()

#endif

#if HAS_LEVELING && (HAS_BED_PROBE || ENABLED(PROBE_MANUALLY))
  float probe_min_x();
  float probe_max_x();
  float probe_min_y();
  float probe_max_y();
#else
  inline float probe_min_x() { return 0; };
  inline float probe_max_x() { return 0; };
  inline float probe_min_y() { return 0; };
  inline float probe_max_y() { return 0; };
#endif

#if HAS_Z_SERVO_PROBE
  void servo_probe_init();
#endif

#if QUIET_PROBING
  void probing_pause(const bool p);
#endif
