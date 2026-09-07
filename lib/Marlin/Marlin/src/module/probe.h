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

  /// Whether probe_at_point() folds the tool-specific terms - the active tool's hotend offset and
  /// its nozzle length - into its result. Say no when those terms are what you are measuring.
  enum class ApplyToolCorrections : bool { no = false,
      yes = true };

  bool probe_should_check_angle_after();

  struct RunZProbeParams {
    float expected_trigger_z;
    bool single_only = false;
    bool *endstop_triggered = nullptr;
    uint8_t required_successes = 1;
    std::optional<bool> check_angle_after = std::nullopt;
  };

  float run_z_probe(const RunZProbeParams& params);
  /// Probe straight down from the current position, retrying at the SAME XY on
  /// failure (lifting between tries). Unlike run_z_probe's spiral search, this
  /// does not move XY between attempts.
  /// @param expected_trigger_z  Expected Z of the surface (down limit / hint).
  /// @param max_attempts        Max probe attempts before giving up.
  /// @returns probed Z (with offsets applied), or NAN if all attempts failed.
  float probe_here(float expected_trigger_z, uint8_t max_attempts);
  float probe_at_point(const xy_pos_t &pos, const ProbePtRaise raise_after=PROBE_PT_NONE, const uint8_t verbose_level=0, const bool probe_relative=true, const uint8_t required_successes=1, const ApplyToolCorrections apply_tool_corrections=ApplyToolCorrections::yes);
  #if ENABLED(NOZZLE_LOAD_CELL) && ENABLED(PROBE_CLEANUP_SUPPORT)
    bool cleanup_probe(const xy_pos_t &rect_min, const xy_pos_t &rect_max);
  #endif

  #if ENABLED(NOZZLE_LOAD_CELL)
    /// Ensure the loadcell is streaming fresh data. Waits; on timeout,
    /// re-arms the active loadcell source and retries. @return false if the stream never resumed.
    bool loadcell_wait_streaming(uint32_t per_attempt_timeout_us = 500'000, uint8_t retries = 3);
  #endif
  #define DEPLOY_PROBE() set_probe_deployed(true)
  #define STOW_PROBE() set_probe_deployed(false)

#else

  constexpr xyz_pos_t probe_offset{0};

  #define DEPLOY_PROBE()
  #define STOW_PROBE()

#endif

#if HAS_LEVELING && HAS_BED_PROBE
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

#if QUIET_PROBING
  // #error dead code found by automatic analyses (see BFW-5461)
  void probing_pause(const bool p);
#endif
