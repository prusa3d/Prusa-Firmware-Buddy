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
 * probe.cpp
 */

#include "../inc/MarlinConfig.h"
#include <bsod/bsod.h>
#include <option/has_crash_detection.h>

#if HAS_BED_PROBE

#include "probe.h"

#ifdef EXTRA_PROBING_DBG
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "dbg.h"
#endif

#include "motion.h"
#include "temperature.h"
#include "endstops.h"
#include <module/planner.h>
#include <feature/pressure_advance/pressure_advance_config.hpp>
#include <feature/precise_stepping/precise_stepping.hpp>
#include <option/has_toolchanger.h>
#include <logging/log.hpp>

#include "../gcode/gcode.h"
#include "../lcd/ultralcd.h"

#include "../Marlin.h" // for disable_e_steppers, wait_for_user

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(MEASURE_BACKLASH_WHEN_PROBING)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../feature/backlash.h"
#endif

xyz_pos_t probe_offset; // Initialized by settings.load()

LOG_COMPONENT_DEF(Probe, logging::Severity::info);

#if ENABLED(NOZZLE_LOAD_CELL)
  #include "loadcell.hpp"
#endif
#if HAS_INDX()
  #include <puppies/INDX.hpp>
#endif

#if ENABLED(HOST_PROMPT_SUPPORT)
  #include "../feature/host_actions.h" // for PROMPT_USER_CONTINUE
#endif

#if ENABLED(SENSORLESS_PROBING)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../feature/motordriver_util.h"
#endif

#if QUIET_PROBING
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "stepper/indirection.h"
#endif

#if ENABLED(EXTENSIBLE_UI)
  #include "../lcd/extensible_ui/ui_api.h"
#endif

#include "metric.h"

#include <feature/print_status_message/print_status_message_guard.hpp>

#include <option/has_auto_retract.h>

#include <option/has_nozzle_thermal_compensation.h>
#if HAS_NOZZLE_THERMAL_COMPENSATION()
  #include <feature/nozzle_thermal_compensation/nozzle_thermal_compensation.hpp>
#endif
#include <option/has_indx.h>
#include <option/has_dwarf.h>
#include <mapi/motion.hpp>
#include <gcode/temperature/M104_M109.hpp>
#include <config_store/store_instance.hpp>
#include <marlin_vars.hpp>
#include <numbers>
#include <filament.hpp>
#include "printers.h"

bool probe_should_check_angle_after() {
#if PRINTER_IS_PRUSA_iX()
    // don't check angle_after for PA, it often causes MBL fails as the nozzle
    // is sticking to the PA-specific sheet and the pulling force fails the check
    return config_store().get_filament_type(active_extruder).parameters().base_preset != PresetFilamentType::PA;
#endif
  return true;
}

#if HAS_AUTO_RETRACT()
  #include <feature/auto_retract/auto_retract.hpp>
#endif

#if ENABLED(TOUCH_MI_PROBE)
  // #error dead code found by automatic analyses (see BFW-5461)

  // Move to the magnet to unlock the probe
  void run_deploy_moves_script() {
    #if TOUCH_MI_DEPLOY_XPOS > X_MAX_BED
      // #error dead code found by automatic analyses (see BFW-5461)
      TemporaryGlobalEndstopsState unlock_x(false);
    #endif
    #if TOUCH_MI_DEPLOY_YPOS > Y_MAX_BED
      // #error dead code found by automatic analyses (see BFW-5461)
      TemporaryGlobalEndstopsState unlock_y(false);
    #endif

    #if ENABLED(TOUCH_MI_MANUAL_DEPLOY)
      // #error dead code found by automatic analyses (see BFW-5461)

      const screenFunc_t prev_screen = ui.currentScreen;
      LCD_MESSAGEPGM(MSG_MANUAL_DEPLOY_TOUCHMI);
      ui.return_to_status();

      KEEPALIVE_STATE(PAUSED_FOR_USER);
      wait_for_user = true; // LCD click or M108 will clear this
      #if ENABLED(HOST_PROMPT_SUPPORT)
        // #error dead code found by automatic analyses (see BFW-5461)
        host_prompt_do(PROMPT_USER_CONTINUE, PSTR("Deploy TouchMI probe."), PSTR("Continue"));
      #endif
      while (wait_for_user) idle(true);
      ui.reset_status();
      ui.goto_screen(prev_screen);

    #elif defined(TOUCH_MI_DEPLOY_XPOS) && defined(TOUCH_MI_DEPLOY_YPOS)
      // #error dead code found by automatic analyses (see BFW-5461)
      do_blocking_move_to_xy(TOUCH_MI_DEPLOY_XPOS, TOUCH_MI_DEPLOY_YPOS);
    #elif defined(TOUCH_MI_DEPLOY_XPOS)
      // #error dead code found by automatic analyses (see BFW-5461)
      do_blocking_move_to_x(TOUCH_MI_DEPLOY_XPOS);
    #elif defined(TOUCH_MI_DEPLOY_YPOS)
      // #error dead code found by automatic analyses (see BFW-5461)
      do_blocking_move_to_y(TOUCH_MI_DEPLOY_YPOS);
    #endif
  }

  // Move down to the bed to stow the probe
  void run_stow_moves_script() {
    const xyz_pos_t oldpos = current_position.xyz();
    endstops.enable_z_probe(false);
    do_blocking_move_to_z(TOUCH_MI_RETRACT_Z, MMM_TO_MMS(HOMING_FEEDRATE_Z));
    do_blocking_move_to(oldpos, MMM_TO_MMS(HOMING_FEEDRATE_Z));
  }

#endif // TOUCH_MI_PROBE

#if QUIET_PROBING
  // #error dead code found by automatic analyses (see BFW-5461)
  void probing_pause(const bool p) {
    #if ENABLED(PROBING_STEPPERS_OFF)
      // #error dead code found by automatic analyses (see BFW-5461)
      disable_e_steppers();
      #if NONE(HOME_AFTER_DEACTIVATE)
        // #error dead code found by automatic analyses (see BFW-5461)
        disable_XY();
      #endif
    #endif
    if (p) safe_delay(
      #if DELAY_BEFORE_PROBING > 25
        // #error dead code found by automatic analyses (see BFW-5461)
        DELAY_BEFORE_PROBING
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        25
      #endif
    );
  }
#endif // QUIET_PROBING

/**
 * Raise Z to a minimum height to make room for a probe to move
 */
inline void do_probe_raise(const float z_raise) {
  float z_dest = z_raise;
  if (probe_offset.z < 0) z_dest -= probe_offset.z;
  #if HAS_HOTEND_OFFSET
  z_dest -= hotend_currently_applied_offset.z;
  #endif

  NOMORE(z_dest, Z_MAX_POS);

  if (z_dest > current_position.z)
    do_blocking_move_to_z(z_dest);
}

FORCE_INLINE void probe_specific_action(const bool deploy) {
  #if ENABLED(NOZZLE_LOAD_CELL)
    if (deploy) {
      // Disable E axis for probing to reduce noise on sensor
      disable_e_steppers();
    }
  #endif /*ENABLED(NOZZLE_LOAD_CELL)*/

  #if ENABLED(TOUCH_MI_PROBE)
    // #error dead code found by automatic analyses (see BFW-5461)

    deploy ? run_deploy_moves_script() : run_stow_moves_script();
  #endif
}

// returns false for ok and true for failure
bool set_probe_deployed(const bool deploy) {
  if (endstops.z_probe_enabled == deploy) return false;

  // Make room for probe to deploy (or stow)
  // Fix-mounted probe should only raise for deploy
  #if ENABLED(FIX_MOUNTED_PROBE)
    const bool deploy_stow_condition = deploy;
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    constexpr bool deploy_stow_condition = true;
  #endif

  // For beds that fall when Z is powered off only raise for trusted Z
  #if ENABLED(UNKNOWN_Z_NO_RAISE)
    // #error dead code found by automatic analyses (see BFW-5461)
    const bool unknown_condition = TEST(axis_known_position, Z_AXIS);
  #else
    constexpr float unknown_condition = true;
  #endif

  #if DISABLED(NOZZLE_LOAD_CELL)
    if (deploy_stow_condition && unknown_condition)
      do_probe_raise(_MAX(Z_CLEARANCE_BETWEEN_PROBES, Z_CLEARANCE_DEPLOY_PROBE));
  #else
    UNUSED(deploy_stow_condition);
    UNUSED(unknown_condition);
  #endif

  const xy_pos_t old_xy = current_position.xy();
  probe_specific_action(deploy);
  do_blocking_move_to(old_xy);
  #if DISABLED(NOZZLE_LOAD_CELL)
    endstops.enable_z_probe(deploy);
  #endif

  return false;
}

#ifdef Z_AFTER_PROBING
  // After probing move to a preferred Z position
  void move_z_after_probing() {
    float pos = Z_AFTER_PROBING;
    #if HAS_HOTEND_OFFSET
      pos -= hotend_currently_applied_offset.z;
    #endif
    if (current_position.z != pos) {
      do_blocking_move_to_z(pos);
      current_position.z = pos;
    }
  }
#endif

/**
 * @brief Used by run_z_probe to do a single Z probe move.
 *
 * @param  z        Z destination
 * @param  fr_mm_s  Feedrate in mm/s
 * @return true to indicate an error
 */

static bool do_probe_move(const float z, const feedRate_t fr_mm_s) {
  // Disable stealthChop if used. Enable diag1 pin on driver.
  #if ENABLED(SENSORLESS_PROBING)
    // #error dead code found by automatic analyses (see BFW-5461)
    sensorless_t stealth_states { false };
    stealth_states.z = enable_crash_detection(Z_AXIS);
    endstops.enable(true);
  #endif

  #if QUIET_PROBING
    // #error dead code found by automatic analyses (see BFW-5461)
    probing_pause(true);
  #endif

  #if ENABLED(NOZZLE_LOAD_CELL)
    endstops.enable_z_probe(true);
  #endif

  // Move down until the probe is triggered
  auto target = planner.get_machine_position_mm();
  target.z = z;
  planner.buffer_segment(target, fr_mm_s, PhysicalToolIndex::currently_selected());
  planner.synchronize();
  // Note: current_position is updated lower in this function

  #if ENABLED(NOZZLE_LOAD_CELL)
    endstops.enable_z_probe(false);
  #endif

  // Check to see if the probe was triggered
  const bool probe_triggered =
      TEST(endstops.trigger_state(),
        #if ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
          Z_MIN
        #else
          // #error dead code found by automatic analyses (see BFW-5461)
          Z_MIN_PROBE
        #endif
      )
  ;

  #if QUIET_PROBING
    // #error dead code found by automatic analyses (see BFW-5461)
    probing_pause(false);
  #endif

  // Re-enable stealthChop if used. Disable diag1 pin on driver.
  #if ENABLED(SENSORLESS_PROBING)
    // #error dead code found by automatic analyses (see BFW-5461)
    endstops.not_homing();
    #if DISABLED(ENDSTOPS_ALWAYS_ON_DEFAULT) && !HAS_CRASH_DETECTION()
      // #error dead code found by automatic analyses (see BFW-5461)
      disable_crash_detection(Z_AXIS, stealth_states.z);
    #endif
  #endif

  // Clear endstop flags
  endstops.hit_on_purpose();

  // Get Z where the steppers were interrupted
  set_current_from_steppers_for_axis(Z_AXIS);

  // Tell the planner where we actually are
  sync_plan_position();

  return !probe_triggered;
}

// those metrics are intentionally not static, as it is expected that they might be referenced
// from outside this file for early registration
METRIC_DEF(metric_probe_z, "probe_z", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
METRIC_DEF(metric_probe_z_diff, "probe_z_diff", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);

#if ENABLED(NOZZLE_LOAD_CELL)
static xy_pos_t offset_for_probe_try(int try_idx) {
  const float distance = 2.0;
  float radius = 0;
  int idx_offset = 0;

  do {
    float perimeter = 2 * radius * std::numbers::pi_v<float>;
    int tries_within_perimeter = static_cast<int>(perimeter / distance) + 1;
    if (try_idx < idx_offset + tries_within_perimeter) {
      int try_within_perimeter = try_idx - idx_offset;
      float goniom_dist = (static_cast<float>(try_within_perimeter) / static_cast<float>(tries_within_perimeter)) * 2 * std::numbers::pi_v<float>;
      return {std::cos(goniom_dist) * radius, std::sin(goniom_dist) * radius};
    } else {
      idx_offset += tries_within_perimeter;
      radius += distance;
    }
  } while (true);
}
#endif

#if ENABLED(NOZZLE_LOAD_CELL)

// After a tool change the loadcell stream may be momentarily absent or stale;
// don't start a probe move until fresh samples are arriving.
bool loadcell_wait_streaming(uint32_t per_attempt_timeout_us, uint8_t retries) {
    for (uint8_t attempt = 0; attempt <= retries; ++attempt) {
        if (loadcell.wait_for_fresh_sample(per_attempt_timeout_us)) {
            return true;
        }
        if (attempt < retries) {
            log_warning(Probe, "loadcell silent, re-arming (attempt %u/%u)",
                static_cast<unsigned>(attempt + 1), static_cast<unsigned>(retries));
  #if HAS_INDX()
            // Toggle the INDX loadcell enable through the puppy task.
            // Wait (bounded) for the "off" write to flush so the head sees a real off->on transition.
            buddy::puppies::indx.set_loadcell(false);
            const uint32_t rearm_start = ticks_us();
            while (buddy::puppies::indx.is_write_pending()
                   && ticks_diff(ticks_us(), rearm_start) <= static_cast<int32_t>(per_attempt_timeout_us)) {
                idle(true);
            }
            buddy::puppies::indx.set_loadcell(true);
  #elif HAS_TOOLCHANGER() && HAS_DWARF()
            // TODO: Dwarf loadcell-enable is a coil not flushed in the periodic refresh;
            // a proper re-arm needs extra plumbing — out of scope for now.
  #else
            // HX717 (local): always streaming, no enable gate — nothing to re-arm.
  #endif
        }
    }
    log_error(Probe, "loadcell did not resume streaming after %u retries",
        static_cast<unsigned>(retries));
    return false;
}

  static float loadcell_retare_for_analysis(millis_t tare_delay) {
    safe_delay(tare_delay);
    loadcell.WaitBarrier(); // Sync samples before tare
    float tare = loadcell.Tare();
    loadcell.analysis.Reset(); // Reset window to remove tare offset jump
    return tare;
  }

// Recover after a loadcell anomaly tripped the safety stop mid-probe: disarm, settle
// motion, re-establish streaming, and back off Z. Does not re-arm; callers `continue`
// to the loop top, which re-arms after the XY travel. The Z back-off lift runs unarmed
// (away from bed).
static bool recover_from_probe_safety_trip() {
  loadcell.disarm_probe_safety(); // disarm first so the ISR can't re-trip during the wait below
  planner.synchronize();          // drain the quick_stop (stop_pending cleared)
  if (!loadcell_wait_streaming())
    return false;
  do_blocking_move_to_z(current_position.z + Z_CLEARANCE_MULTI_PROBE, MMM_TO_MMS(Z_PROBE_SPEED_FAST));
  return true;
}

enum class ProbeRetry { retry, fail };

// Called whenever probe_should_abort() or probe_safety_did_trip() fires mid-probe.
// Returns 'retry' → caller should continue the probe loop; 'fail' → caller returns NAN.
static ProbeRetry handle_probe_safety_trip() {
  if (planner.draining())
    return ProbeRetry::fail; // cancelled: propagate
  if (!recover_from_probe_safety_trip())
    return ProbeRetry::fail; // unrecoverable loadcell
  return ProbeRetry::retry;
}
#endif

/**
 * @brief Probe at the current XY (possibly more than once) to find the bed Z.
 *
 * @details Used by probe_at_point to get the bed Z height at the current XY.
 *          Leaves current_position.z at the height where the probe triggered.
 *
 * @param params Struct containing parameters for the probing operation:
 * @param params.expected_trigger_z do not probe lower than expected_trigger_z + Z_PROBE_LOW_POINT [mm]
 * @param params.single_only
 * @param[out] params.endstop_triggered
 *  - true endstop was triggered earlier than expected_trigger_z was reached
 *  - false endstop was not reached
 * @param params.is_nozzle_clean If true, skip any nozzle cleaning routines before probing.
 *  when the analysis could still compute a Z for them.
 *
 * @return The Z position of the bed at the current XY or NAN on error.
 */
float run_z_probe(const RunZProbeParams& params) {
  if (DEBUGGING(LEVELING)) {
    SERIAL_ECHOLNPAIR(">>> run_z_probe x=", current_position.x, ", y=", current_position.y, ", z=", current_position.z);
  }

  // Stop the probe before it goes too low to prevent damage.
  // If Z isn't known then probe to -10mm.
  float z_probe_low_point = params.expected_trigger_z + Z_PROBE_LOW_POINT;
  if (params.endstop_triggered)
    *params.endstop_triggered = true;

  // We expect PA delays to be already avoided here
  debug_assert(pressure_advance::PressureAdvanceDisabler::is_active());

  #if ENABLED(NOZZLE_LOAD_CELL)
    if (!loadcell_wait_streaming()) {
      return NAN;
    }
    auto H = loadcell.CreateLoadAboveErrEnforcer();
    // Arm for the reference tare + descent; disarms on every return path.
    auto safetyArmer = Loadcell::ProbeSafetyArmer(loadcell);
    auto reference_tare = loadcell_retare_for_analysis(Z_FIRST_PROBE_DELAY); ///< Use this value as reference for following tares
    const auto max_tare_offset = std::abs(loadcell.GetThreshold()); ///< Maximal valid offset from reference_tare
    if (loadcell.probe_should_abort())
      return NAN;
  #endif

  // Double-probing does a fast probe followed by a slow probe
  #if TOTAL_PROBING == 2
    // #error dead code found by automatic analyses (see BFW-5461)

    // Do a first probe at the fast speed
    if (do_probe_move(z_probe_low_point, MMM_TO_MMS(Z_PROBE_SPEED_FAST))) {
      if(params.endstop_triggered)
        *params.endstop_triggered = false;
      if (planner.draining() || PreciseStepping::stopping())
        return NAN;

      #if ENABLED(HALT_ON_PROBING_ERROR)
        // #error dead code found by automatic analyses (see BFW-5461)
        kill("PROBING ERROR", "Could not reach the bed, FAST Probe fail!");
      #endif
      return NAN;
    }

    const float first_probe_z = current_position.z;

    // Raise to give the probe clearance
    do_blocking_move_to_z(current_position.z + Z_CLEARANCE_MULTI_PROBE, MMM_TO_MMS(Z_PROBE_SPEED_FAST));

  #elif Z_PROBE_SPEED_FAST != Z_PROBE_SPEED_SLOW

    // If the nozzle is well over the travel height then
    // move down quickly before doing the slow probe
    const float z = params.expected_trigger_z + Z_CLEARANCE_DEPLOY_PROBE + 5.0f + (probe_offset.z < 0 ? -probe_offset.z : 0) - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.z);
    if (current_position.z > z) {
      // Probe down fast. If the probe never triggered, raise for probe clearance
      if (!do_probe_move(z, MMM_TO_MMS(Z_PROBE_SPEED_FAST))) {
        do_blocking_move_to_z(current_position.z + Z_CLEARANCE_BETWEEN_PROBES, MMM_TO_MMS(Z_PROBE_SPEED_FAST), Segmented::yes);
      }
    }
  #endif

  #ifdef EXTRA_PROBING
    float probes[TOTAL_PROBING];
  #elif ENABLED(NOZZLE_LOAD_CELL)
    xy_pos_t center_pos = current_position.xy();
    int probe_idx = 0;
    uint8_t success_count = 0;
    float z_sum = 0.0f;
  #endif

  #if TOTAL_PROBING > 2 && DISABLED(NOZZLE_LOAD_CELL)
    float probes_total = 0;
  #endif

  #if TOTAL_PROBING > 2
    for (uint8_t p = 0; p < TOTAL_PROBING; p++)
  #endif
    {
      idle(false); // Avoid watchdog reset in case of no move while probing
      #if ENABLED(NOZZLE_LOAD_CELL)
        auto center_offset = offset_for_probe_try(probe_idx++);
        // XY travel must not be quick-stoppable; re-arm after for the tare + descent.
        loadcell.disarm_probe_safety();
        do_blocking_move_to_xy(center_pos + center_offset, MMM_TO_MMS(Z_PROBE_SPEED_FAST));
        loadcell.arm_probe_safety();

        // re-tare the loadcell
        auto offset = loadcell_retare_for_analysis(Z_FIRST_PROBE_DELAY) - reference_tare;

        SERIAL_ECHO_START();
        SERIAL_ECHOLNPAIR_F("Re-tared with offset ", offset);

        // If tare value is suspicious, lift very high and try tare again
        if (std::abs(offset) > max_tare_offset) {
          do_blocking_move_to_z(current_position.z + Z_AFTER_PROBING, MMM_TO_MMS(Z_PROBE_SPEED_FAST), Segmented::yes);
          reference_tare = loadcell_retare_for_analysis(Z_FIRST_PROBE_DELAY);

          SERIAL_ECHO_START();
          SERIAL_ECHOLNPAIR_F("Lifted and took new reference tare ", reference_tare);
        }

        SERIAL_ECHO_START();
        SERIAL_ECHOLNPAIR("Starting probe at ", center_pos.x, " ", center_pos.y);

        METRIC_DEF(probe_start, "probe_start", METRIC_VALUE_EVENT, 0, METRIC_ENABLED);
        metric_record_event(&probe_start);

        // Sync enough samples before moving downwards to ensure the pre-compression line can be fit
        // when the Z move is very short
        loadcell.WaitBarrier(static_cast<uint32_t>(ticks_us() + loadcell.analysis.analysisLookback * 1e6f));

        // A re-tare or the sync above tripped the safety stop.
        if (loadcell.probe_should_abort()) {
          if (handle_probe_safety_trip() == ProbeRetry::fail)
            return NAN;
          continue;
        }
      #endif

      // Probe downward slowly to find the bed
      if (do_probe_move(z_probe_low_point, MMM_TO_MMS(Z_PROBE_SPEED_SLOW))) {
        if(params.endstop_triggered)
          *params.endstop_triggered = false;
        if (planner.draining())
          return NAN; // cancelled
        #if ENABLED(NOZZLE_LOAD_CELL)
          if (loadcell.probe_safety_did_trip()) { // anomaly mid-descent: recover and retry
            if (handle_probe_safety_trip() == ProbeRetry::fail)
              return NAN;
            continue;
          }
        #endif
        // genuine: bed not reached
        #if ENABLED(HALT_ON_PROBING_ERROR)
          // #error dead code found by automatic analyses (see BFW-5461)
          kill("PROBING ERROR", "Could not reach the bed, SLOW Probe fail!");
        #endif
        return NAN;
      }

      #if ENABLED(NOZZLE_LOAD_CELL)
        // The analysis profile *expects* a delay after touchdown. Compensate for the
        // synchronization and first move delay to wait precisely the amount requested.
        uint32_t move_fwd_end = PreciseStepping::get_time_of_last_block_us();
        uint32_t elapsed_us = ticks_diff(ticks_us(), move_fwd_end);
        uint32_t start_delay_us = PreciseStepping::get_first_move_delay_us();
        uint32_t precomp_ms = (elapsed_us + start_delay_us) / 1000;
        debug_assert(precomp_ms <= Loadcell::TOUCHDOWN_DELAY_MS); // we handle underflow, but catch it on debug
        millis_t delay_ms = std::min<uint32_t>(Loadcell::TOUCHDOWN_DELAY_MS - precomp_ms, Loadcell::TOUCHDOWN_DELAY_MS);
        safe_delay(delay_ms);

        // Return slowly back. Ensure this move is not optimized even when small
        float move_back = 0.09f;

        auto target = current_machine_position();
        target.z += move_back;
        planner.buffer_line(target, MMM_TO_MMS(Z_PROBE_SPEED_BACK_MOVE), PhysicalToolIndex::currently_selected(), { .raw_block = true });
        set_current_position(to_native_pos(target));

        planner.synchronize();
        if (loadcell.probe_should_abort()) {
          if (handle_probe_safety_trip() == ProbeRetry::fail)
            return NAN;
          continue;
        }
        uint32_t move_back_end = PreciseStepping::get_time_of_last_block_us();
      #endif

      #if ENABLED(MEASURE_BACKLASH_WHEN_PROBING)
        // #error dead code found by automatic analyses (see BFW-5461)
        backlash.measure_with_probe();
      #endif

      #if DISABLED(NOZZLE_LOAD_CELL)
        const float z = current_position.z;
      #endif


      #if EXTRA_PROBING
        // Insert Z measurement into probes[]. Keep it sorted ascending.
        for (uint8_t i = 0; i <= p; i++) {                            // Iterate the saved Zs to insert the new Z
          if (i == p || probes[i] > z) {                              // Last index or new Z is smaller than this Z
            for (int8_t m = p; --m >= i;) probes[m + 1] = probes[m];  // Shift items down after the insertion point
            probes[i] = z;                                            // Insert the new Z measurement
            break;                                                    // Only one to insert. Done!
          }
        }
      #elif ENABLED(NOZZLE_LOAD_CELL)
        // wait until the analysis' window fully includes the move-back period
        uint32_t window_end = move_back_end + static_cast<uint32_t>((loadcell.analysis.analysisLookahead + loadcell.analysis.loadDelay) * 1000000.f);
        loadcell.WaitBarrier(window_end);

        if (loadcell.probe_should_abort()) {
          if (handle_probe_safety_trip() == ProbeRetry::fail)
            return NAN;
          continue;
        }

        METRIC_DEF(analysis_result, "probe_analysis", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
        auto result = loadcell.analysis.Analyse(params.check_angle_after ? *params.check_angle_after : probe_should_check_angle_after());

        if (result.has_value()) {
          z_sum += result->z_coordinate;
          success_count++;
          metric_record_custom(&analysis_result, " ok=%i,desc=\"all-good\"", true);
          SERIAL_ECHOLNPAIR("Probe ", success_count, "/", params.required_successes, " classified as clean and OK, Z: ", result->z_coordinate);
          if (success_count >= params.required_successes)
            break;

        } else {
          const auto &err = result.error();
          metric_record_custom(&analysis_result, " ok=%i,desc=\"%s\",arg=%f", false, err.description, err.arg);
          SERIAL_ECHO_START();
          SERIAL_ECHOPAIR("Probe classified as NOK (", err.description);
          SERIAL_ECHOPAIR(", ", err.arg);
          SERIAL_ECHOLN(")");
        }
      #elif TOTAL_PROBING > 2
        // #error dead code found by automatic analyses (see BFW-5461)
        probes_total += z;
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        UNUSED(z);
      #endif

      if (params.single_only)
        break;

      #if TOTAL_PROBING > 2
        // Small Z raise after all but the last probe
        if (p < TOTAL_PROBING - 1) {
          do_blocking_move_to_z(current_position.z + Z_CLEARANCE_MULTI_PROBE, MMM_TO_MMS(Z_PROBE_SPEED_FAST));
        }
      #endif
    }

  #if ENABLED(NOZZLE_LOAD_CELL)

    const float measured_z = success_count >= params.required_successes ? z_sum / success_count : NAN;

  #elif TOTAL_PROBING > 2

    #if EXTRA_PROBING
      // Take the center value (or average the two middle values) as the median
      static constexpr int PHALF = (TOTAL_PROBING - 1) / 2;
      const float middle = probes[PHALF],
                  median = ((TOTAL_PROBING) & 1) ? middle : (middle + probes[PHALF + 1]) * 0.5f;

      // Remove values farthest from the median
      uint8_t min_avg_idx = 0, max_avg_idx = TOTAL_PROBING - 1;
      for (uint8_t i = EXTRA_PROBING; i--;)
        if (ABS(probes[max_avg_idx] - median) > ABS(probes[min_avg_idx] - median))
          max_avg_idx--; else min_avg_idx++;

      // Return the average value of all remaining probes.
      for (uint8_t i = min_avg_idx; i <= max_avg_idx; i++)
        probes_total += probes[i];

    #endif

    const float measured_z = probes_total * RECIPROCAL(MULTIPLE_PROBING);

  #elif TOTAL_PROBING == 2
    // #error dead code found by automatic analyses (see BFW-5461)

    const float z2 = current_position.z;

    // Return a weighted average of the fast and slow probes
    const float measured_z = (z2 * 3.0 + first_probe_z * 2.0) * 0.2;

  #else
    // #error dead code found by automatic analyses (see BFW-5461)

    // Return the single probe result
    const float measured_z = current_position.z;

  #endif

  return measured_z;
}


#if ENABLED(NOZZLE_LOAD_CELL) && ENABLED(PROBE_CLEANUP_SUPPORT)

#if HAS_AUTO_RETRACT()
void prepare_for_nozzle_cleaning() {
  // Do not retract on unknown retracted_distance
  const auto retracted_distance = buddy::auto_retract().retracted_distance().value_or(10);
  const bool do_not_autoretract = FilamentType::for_tool_heuristic(VirtualToolIndex::currently_selected()).parameters().is_flexible;
  // Do not auto retract filaments marked is_flexible, they might get tangled in the extruder (BFW-6953)
  // Skip if retracted distance is known and filament is out of the nozzle
  if (!do_not_autoretract && retracted_distance < 5.0f) {
    buddy::auto_retract().ensure_retracted_no_ramming();
  }
}
#endif

/**
 * @brief Probe within a given rectangle in order to cleanup loadcell-based probe.
 */
bool cleanup_probe(const xy_pos_t &rect_min, const xy_pos_t &rect_max) {
  GcodeSuite::G28_no_parser(true, true, true,
    {
        .only_if_needed = true,
        .precise = false
    });

  float radius = 1.0f;
  bool probe_deployed = false;
  const int required_clean_cnt = 3;
  int consecutive_clean_cnt = 0;

  // Disable PA to reduce filter delay during probe analysis
  pressure_advance::PressureAdvanceDisabler pa_disabler;

  // Enable loadcell high precision across the entire sequence to prime the noise filters
  auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell);

  // set acceleration to known value
  auto saved_acceleration = planner.user_settings.travel_acceleration;
  {
    auto s = planner.user_settings;
    s.travel_acceleration = PROBE_CLEANUP_TRAVEL_ACCELERATION;
    planner.apply_settings(s);
  }

  #if HAS_AUTO_RETRACT()
  if (config_store().pre_nozzle_cleaning_retraction_enable.get()) {
    // If retracted distance is known, we should retract to avoid nozzle cleaning fail often occurring with the filament in the nozzle
    prepare_for_nozzle_cleaning();
  }
  #endif // HAS_AUTO_RETRACT()
  
  PrintStatusMessageGuard pmg;
  pmg.update<PrintStatusMessage::nozzle_cleaning>({});

  bool should_continue = true;
  for (float y = rect_min.y + radius; (y + radius) <= rect_max.y && should_continue; y += 2 * radius) {
    for (float x = rect_max.x - radius; (x - radius) >= rect_min.x && should_continue; x -= 2 * radius) {
      // move above the probe point
      xyz_pos_t pos = { x, y, static_cast<float>(PROBE_CLEANUP_CLEARANCE - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.z))};
      do_blocking_move_to(pos);

      if(probe_deployed == false) {
        // first attempt: deploy probe
        if (DEPLOY_PROBE()) {
          SERIAL_ECHOLNPGM("failed to deploy probe");
          should_continue = false;
          break;
        }
      }
      probe_deployed = true;

      // probe
      float result = run_z_probe({
        .expected_trigger_z = 0.f,
        .single_only = true,
        .endstop_triggered = nullptr,
        .check_angle_after = false,
      });
      if (planner.draining()) {
        should_continue = false;
        break;
      }
      if (!std::isnan(result)) {
        consecutive_clean_cnt += 1;
      } else {
        consecutive_clean_cnt = 0;
      }

      // exit in case the probe was successfull
      if (consecutive_clean_cnt >= required_clean_cnt) {
        should_continue = false;
        break;
      }
    }
  }

  // restore acceleration
  {
    auto s = planner.user_settings;
    s.travel_acceleration = saved_acceleration;
    planner.apply_settings(s);
  }

  if (probe_deployed) {
    STOW_PROBE();
  }
  
  return consecutive_clean_cnt >= required_clean_cnt;
}
#endif


float probe_here(float expected_trigger_z, uint8_t max_attempts)
{
  float res = NAN;
  DEPLOY_PROBE();
  for(uint8_t i = 0; i < max_attempts; i++){
    res = run_z_probe({ .expected_trigger_z = expected_trigger_z, .single_only = true }) + probe_offset.z + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.z);
    if (!std::isnan(res))
      break;

    // When run_z_probe fails in single_only mode, it does not lift the head, we gotta do it ourselves
    do_blocking_move_to_z( current_position.z + Z_CLEARANCE_MULTI_PROBE);
  }
  STOW_PROBE();

  return res;
}

/**
 * - Move to the given XY
 * - Deploy the probe, if not already deployed
 * - Probe the bed, get the Z position
 * - Depending on the 'stow' flag
 *   - Stow the probe, or
 *   - Raise to the BETWEEN height
 * - Return the probed Z position
 */
float probe_at_point(const xy_pos_t &pos, const ProbePtRaise raise_after/*=PROBE_PT_NONE*/, const uint8_t verbose_level/*=0*/, const bool probe_relative/*=true*/, const uint8_t required_successes/*=1*/, const ApplyToolCorrections apply_tool_corrections/*=ApplyToolCorrections::yes*/) {
  xyz_pos_t npos = xyz_pos_t(pos);
  if (probe_relative) {
    if (!position_is_reachable_by_probe(npos.xy())) {
      #if ENABLED(HALT_ON_PROBING_ERROR)
        // #error dead code found by automatic analyses (see BFW-5461)
        kill("PROBING ERROR", "Could not reach the bed, XY position not within machine coordinates!");
      #endif
      return NAN; // The given position is in terms of the probe
    }
    npos -= probe_offset; // Get the nozzle position
  }
  else if (!position_is_reachable(npos.xy())) {
    #if ENABLED(HALT_ON_PROBING_ERROR)
      // #error dead code found by automatic analyses (see BFW-5461)
      kill("PROBING ERROR", "Could not reach the bed, XY position not within machine coordinates!");
    #endif
    return NAN; // The given position is in terms of the nozzle
  }
  #if HAS_HOTEND_OFFSET
  // now offset the probing possition by nozzle offset, to probe where nozzle actually is in desired position
  npos -= hotend_currently_applied_offset;
  #endif

  npos.z =
      current_position.z
  ;

  const float old_feedrate_mm_s = feedrate_mm_s;
  feedrate_mm_s = XY_PROBE_FEEDRATE_MM_S;

  // Move the probe to the starting XYZ
  do_blocking_move_to(npos, MMM_TO_MMS(XY_PROBE_SPEED));

  // Disable PA to reduce filter delay during probe analysis
  pressure_advance::PressureAdvanceDisabler pa_disabler;

  #if ENABLED(NOZZLE_LOAD_CELL)
    // HighPrecision needs to be enabled with some time margin to prime the filters.
    // If it hasn't been already we're being called in single-probe mode, enable it temporarily.
    bool enableHighPrecision = !loadcell.IsHighPrecisionEnabled();
    if (enableHighPrecision) SERIAL_ECHO_MSG("probe: enabling high-precision in single-probe mode");
    auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell, enableHighPrecision);
  #endif

  float measured_z = NAN;
  if (!DEPLOY_PROBE()) {
    measured_z = run_z_probe({ .expected_trigger_z = 0.f, .required_successes = required_successes});
    const float move_away_from = std::isnan(measured_z) ? current_position.z : measured_z;

    measured_z += probe_offset.z;

    if (apply_tool_corrections == ApplyToolCorrections::yes) {
      #if HAS_HOTEND_OFFSET
      #if !HAS_TOOLCHANGER() && !HAS_INDX()
        #error not implemented
      #endif
      // measured Z is in probe's logical coordinate space, shift it to printers native coordinate space
      measured_z += hotend_currently_applied_offset.z;
      #endif

      #if HAS_NOZZLE_THERMAL_COMPENSATION()
      // Express the result in the nozzle's length at the reference temperature; see
      // nozzle_thermal_compensation.hpp
      measured_z -= buddy::nozzle_thermal_compensation::current_elongation_vs_reference_mm();
      #endif
    }

    const bool big_raise = raise_after == PROBE_PT_BIG_RAISE;
    if (big_raise || raise_after == PROBE_PT_RAISE) {
      plan_park_move_to(current_position.x, current_position.y, move_away_from + (big_raise ? 25 : Z_CLEARANCE_BETWEEN_PROBES), MMM_TO_MMS(XY_PROBE_SPEED), MMM_TO_MMS(Z_PROBE_SPEED_FAST), Segmented::no);
    } else if (raise_after == PROBE_PT_STOW)
      if (STOW_PROBE()) measured_z = NAN;
  }

  const auto logical = xyz_pos_t(pos).asLogical();

  if (verbose_level > 2) {
    SERIAL_ECHOPAIR_F("Bed X: ", logical.x, 3);
    SERIAL_ECHOPAIR_F(" Y: ", logical.y, 3);
    SERIAL_ECHOLNPAIR_F(" Z: ", measured_z, 3);
  }

  {
      int logical_x = static_cast<int>(logical.x);
      int logical_y = static_cast<int>(logical.y);
      metric_record_custom(&metric_probe_z, " x=%i,y=%i,v=%.3f", logical_x, logical_y, (double)measured_z);
  }

  feedrate_mm_s = old_feedrate_mm_s;

  if (isnan(measured_z)) {
    STOW_PROBE();
    #if ENABLED(HALT_ON_PROBING_ERROR)
      // #error dead code found by automatic analyses (see BFW-5461)
      kill("PROBING ERROR", "Could not reach the bed, endstop was not triggered!");
    #endif
  }

  return measured_z;
}

#endif // HAS_BED_PROBE

#if HAS_LEVELING && HAS_BED_PROBE
 float probe_min_x() {
    return (X_MIN_POS) + probe_offset.x + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.x);
  }
  float probe_max_x() {
    return (X_MAX_POS) + probe_offset.x + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.x);
  }
  float probe_min_y() {
    return (Y_MIN_POS) + probe_offset.y + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.y);
  }
  float probe_max_y() {
    #ifdef PROBE_MAX_Y
      return (PROBE_MAX_Y) + probe_offset.y + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.y);
    #else
      return (Y_MAX_POS) + probe_offset.y + TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.y);
    #endif
  }
#endif
