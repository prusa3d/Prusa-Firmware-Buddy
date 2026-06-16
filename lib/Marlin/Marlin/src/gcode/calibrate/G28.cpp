/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"
#include "module/motion.h"

#include "../gcode.h"

#include "bsod.h"
#include "homing_reporter.hpp"

#include "../../module/endstops.h"
#include "../../module/planner.h"
#include "../../module/stepper.h" // for various

#if HAS_MULTI_HOTEND
  #include "../../module/tool_change.h"
#endif

#if HAS_LEVELING
  #include "../../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(BD_SENSOR)
  #include "../../feature/bedlevel/bdl/bdl.h"
#endif

#if ENABLED(SENSORLESS_HOMING)
  #include "../../feature/motordriver_util.h"
#endif

#if ENABLED(CRASH_RECOVERY)
  #include "../../feature/prusa/crash_recovery.hpp"
#endif

#if HAS_PRECISE_HOMING_COREXY()
  #include "../../module/prusa/homing_corexy.hpp"
#endif

#include "../../module/probe.h"

#if ENABLED(BLTOUCH)
  #include "../../feature/bltouch.h"
#endif

#include "../../lcd/ultralcd.h"

#if ENABLED(EXTENSIBLE_UI)
  //#include "../../lcd/extui/ui_api.h"
#elif ENABLED(DWIN_CREALITY_LCD)
  #include "../../lcd/e3v2/creality/dwin.h"
#elif ENABLED(DWIN_LCD_PROUI)
  #include "../../lcd/e3v2/proui/dwin.h"
#endif

#if ENABLED(LASER_FEATURE)
  #include "../../feature/spindle_laser.h"
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
  #include <module/prusa/toolchanger.h>
#endif

#if ENABLED(NOZZLE_LOAD_CELL)
#  include "../../feature/prusa/e-stall_detector.h"
#endif

#define DEBUG_OUT ENABLED(DEBUG_LEVELING_FEATURE)
#include "../../core/debug_out.h"

#include <feature/tmc_util.h> // for disabling Wave Table during homing

#include <feature/phase_stepping/phase_stepping.hpp> // for disabling phase stepping during homing
#include <feature/pressure_advance/pressure_advance_config.hpp> // for disabling PA during homing

#include <option/has_nozzle_cleaner.h>

#if ENABLED(DELTA) || ENABLED(SCARA) || ENABLED(AXEL_TPARA) || ENABLED(DUAL_X_CARRIAGE) || ENABLED(FOAMCUTTER_XYUV)
  #error These babies are no longer welcome here. The relevants ifdefs were removed.
#endif

#include <scope_guard.hpp>
#include <marlin_server.hpp>
#include <feature/print_status_message/print_status_message_guard.hpp>
#include <config_store/store_instance.hpp>
#include <buddy/unreachable.hpp>

#include <option/has_ceiling_clearance.h>
#if HAS_CEILING_CLEARANCE()
  #include <feature/ceiling_clearance/ceiling_clearance.hpp>
#endif

#if HAS_PRECISE_HOMING_COREXY()
bool corexy_refine_during_G28(float fr_mm_s, const G28Flags &flags);
#endif

#if ENABLED(QUICK_HOME)

  static void quick_home_xy() {

    // Pretend the current position is 0,0
    axes_home_level[X_AXIS] = AxisHomeLevel::not_homed;
    axes_home_level[Y_AXIS] = AxisHomeLevel::not_homed;
    current_position.set(0.0, 0.0);
    sync_plan_position();

    const int x_axis_home_dir = TOOL_X_HOME_DIR(active_extruder);

    const float speed_ratio = homing_feedrate(X_AXIS) / homing_feedrate(Y_AXIS);
    const float speed_ratio_inv = homing_feedrate(Y_AXIS) / homing_feedrate(X_AXIS);
    const float length_ratio = max_length(X_AXIS) / max_length(Y_AXIS);
    const bool length_r_less_than_speed_r = length_ratio < speed_ratio;

    const float mlx = length_r_less_than_speed_r ? (max_length(Y_AXIS) * speed_ratio) : (max_length(X_AXIS));
    const float mly = length_r_less_than_speed_r ? (max_length(Y_AXIS)) : (max_length(X_AXIS) * speed_ratio_inv);
    const float fr_mm_s = SQRT(sq(homing_feedrate(X_AXIS)) + sq(homing_feedrate(Y_AXIS)));

    #if ENABLED(SENSORLESS_HOMING)
      #if ENABLED(CRASH_RECOVERY)
        Crash_Temporary_Deactivate ctd;
      #endif // ENABLED(CRASH_RECOVERY)

      sensorless_t stealth_states {
        NUM_AXIS_LIST(
          TERN0(X_SENSORLESS, enable_crash_detection(X_AXIS)),
          TERN0(Y_SENSORLESS, enable_crash_detection(Y_AXIS)),
          false, false, false, false
        )
        , TERN0(X2_SENSORLESS, enable_crash_detection(X2_AXIS))
        , TERN0(Y2_SENSORLESS, enable_crash_detection(Y2_AXIS))
      };

      #if ENABLED(CRASH_RECOVERY)
        // Technically we should call end_sensorless_homing_per_axis() after
        // the move, but what follows is homing anyway, so it's not needed.
        crash_s.start_sensorless_homing_per_axis(X_AXIS);
        crash_s.start_sensorless_homing_per_axis(Y_AXIS);
      #endif
    #endif

    do_blocking_move_to_xy(1.5 * mlx * x_axis_home_dir, 1.5 * mly * home_dir(Y_AXIS), fr_mm_s);

    endstops.validate_homing_move();

    current_position.set(0.0, 0.0);
    sync_plan_position();

    #if ENABLED(SENSORLESS_HOMING)
      #if ANY(ENDSTOPS_ALWAYS_ON_DEFAULT, CRASH_RECOVERY)
        UNUSED(stealth_states);
      #else
        TERN_(X_SENSORLESS, disable_crash_detection(X_AXIS, stealth_states.x));
        TERN_(X2_SENSORLESS, disable_crash_detection(X2_AXIS, stealth_states.x2));
        TERN_(Y_SENSORLESS, disable_crash_detection(Y_AXIS, stealth_states.y));
        TERN_(Y2_SENSORLESS, disable_crash_detection(Y2_AXIS, stealth_states.y2));
      #endif
    #endif
  }

#endif // QUICK_HOME

#if ENABLED(Z_SAFE_HOMING)

  inline bool home_z_safely() {
    DEBUG_SECTION(log_G28, "home_z_safely", DEBUGGING(LEVELING));

    // Disallow Z homing if X or Y homing is needed
    if (homing_needed_error(_BV(X_AXIS) | _BV(Y_AXIS))) return false;

    sync_plan_position();

    /**
     * Move the Z probe (or just the nozzle) to the safe homing point
     * (Z is already at the right height)
     */
    constexpr xy_float_t safe_homing_xy = { Z_SAFE_HOMING_X_POINT, Z_SAFE_HOMING_Y_POINT };
    #if HAS_HOME_OFFSET
      xy_float_t okay_homing_xy = safe_homing_xy;
      okay_homing_xy -= home_offset;
    #else
      constexpr xy_float_t okay_homing_xy = safe_homing_xy;
    #endif

    xyze_pos_t dest_pos;
    dest_pos.set(okay_homing_xy, current_position.z);

    TERN_(HOMING_Z_WITH_PROBE, dest_pos -= probe_offset);
    TERN_(HAS_HOTEND_OFFSET, dest_pos -= hotend_currently_applied_offset);

    if (position_is_reachable(dest_pos)) {

      if (DEBUGGING(LEVELING)) DEBUG_POS("home_z_safely", dest_pos);

#if ENABLED(PRUSA_TOOLCHANGER)
      do_blocking_move_to_xy(dest_pos, PrusaToolChanger::limit_stealth_feedrate(XY_PROBE_FEEDRATE_MM_S));
#elif HAS_NOZZLE_CLEANER()
    // with nozzle cleaner (iX), move in Y first to avoid going over the cleaner
    do_blocking_move_to_xy(current_position.x, dest_pos.y);
    do_blocking_move_to_xy(dest_pos.x, dest_pos.y);
#else
      do_blocking_move_to_xy(dest_pos);
#endif

      if (!homeaxis(Z_AXIS)) {
        return false;
      }
    }
    else {
      LCD_MESSAGE(MSG_ZPROBE_OUT);
      SERIAL_ECHO_MSG(STR_ZPROBE_OUT_SER);
    }

    return true;
  }

  #if ENABLED(DETECT_PRINT_SHEET)
    /**
     * @brief Detect print sheet
     *
     * @param z_homing_height z clearance before moving to detect print sheet point
     * @retval true print sheet detected
     * @retval false print sheet not detected or move was interrupted
     */
    static bool detect_print_sheet(const float z_homing_height) {
      DEBUG_SECTION(log_G28, "detect_print_sheet", DEBUGGING(LEVELING));

      // Disallow detection if if X or Y or Z homing is needed
      if (homing_needed_error(_BV(X_AXIS) | _BV(Y_AXIS) | _BV(Z_AXIS))) return false;

      #if ENABLED(NOZZLE_LOAD_CELL) && HOMING_Z_WITH_PROBE
        // Enable loadcell high precision across the entire procedure to prime the noise filters
        auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell);
      #endif

      /**
       * Move the Z probe (or just the nozzle) to the sheet
       * detect point
       */
      do_z_clearance(z_homing_height);
      constexpr xy_float_t sheet_detect_xy = { DETECT_PRINT_SHEET_X_POINT, DETECT_PRINT_SHEET_Y_POINT };
      #if HAS_HOME_OFFSET
        xy_float_t okay_homing_xy = sheet_detect_xy;
        okay_homing_xy -= home_offset;
      #else
        constexpr xy_float_t okay_homing_xy = safe_homing_xy;
      #endif

      destination.set(okay_homing_xy, current_position.z);

      TERN_(HOMING_Z_WITH_PROBE, destination -= probe_offset);

      if (position_is_reachable(destination)) {

        if (DEBUGGING(LEVELING)) DEBUG_POS("detect_print_sheet", destination);

        do_blocking_move_to(destination);
        bool endstop_triggered;
        run_z_probe(0 - (Z_PROBE_LOW_POINT) + DETECT_PRINT_SHEET_Z_POINT, true, &endstop_triggered);
        if(!endstop_triggered) {
          return false;
        }
      }
      else {
        LCD_MESSAGE(MSG_ZPROBE_OUT);
        SERIAL_ECHO_MSG(STR_ZPROBE_OUT_SER);
      }

      return true;
    }
  #endif // DETECT_PRINT_SHEET

#endif // Z_SAFE_HOMING

#if HAS_TRINAMIC && defined(HAS_TMC_WAVETABLE)
static void reenable_wavetable(AxisEnum axis)
{
    tmc_enable_wavetable(axis == X_AXIS, axis == Y_AXIS, false);
}
#endif


/** \addtogroup G-Codes
 * @{
 */

/**
 *###G28: Home all axes according to settings <a href="https://reprap.org/wiki/G-code#G28:_Move_to_Origin_.28Home.29">G28: Move to Origin (Home)</a>
 *
 *If `HAS_PRECISE_HOMING()` is enabled, there are specific amount
 *of tries to home an X/Y axis. If it fails it runs re-calibration
 *
 *Home to all axes with no parameters.
 *
 *#### Usage
 *
 *    G28 [ X | Y | Z | I | N | O | P | R | S ]
 *
 *#### Parameters
 *
 * - `X` - Flag to go back to the X axis origin
 * - `Y` - Flag to go back to the Y axis origin
 * - `Z` - Flag to go back to the Z axis origin
 * - `I` - Imprecise: do not perform precise refinement
 * - `L` - Force leveling state ON (if possible) or OFF after homing (Requires `RESTORE_LEVELING_AFTER_G28` or `ENABLE_LEVELING_AFTER_G28`)
 * - `N` - No-change mode (do not change any motion setting such as feedrate)
 * - `O` - Home only if the position is not known and trusted
 * - `P` - Do not check print sheet presence
 * - `R` - <linear> Raise by n mm/inches before homing
 * - `S` - Simulated homing only in `MARLIN_DEV_MODE`
 * - `D` - Disallow homing self-calibration
 * - `C` - Force homing self-calibration
 */
void GcodeSuite::G28() {
#if ENABLED(NOZZLE_LOAD_CELL)
  BlockEStallDetection block_e_stall_detection;
#endif

  marlin_server::FSM_Holder fsm_holder(PhaseWait::homing);

  bool X = parser.seen('X');
  bool Y = parser.seen('Y');
  bool Z = parser.seen('Z');

  G28Flags flags;
  flags.only_if_needed = parser.boolval('O');
  flags.z_raise = parser.seenval('R') ? parser.value_linear_units() : Z_HOMING_HEIGHT;
  flags.no_change = parser.seen('N');
  flags.can_calibrate = !parser.seen('D');
  flags.force_calibrate = parser.seen('C');
  #if ENABLED(MARLIN_DEV_MODE)
    flags.simulate = parser.seen('S');
  #endif
  #if ENABLED(DETECT_PRINT_SHEET)
    flags.check_sheet = !parser.boolval('P');
  #endif
  flags.precise = !parser.seen('I'); // do not perform precise refinement

  // No axes were specified -> interpret as home all
  if(!X && !Y && !Z) {
    X = Y = Z = true;
  }

  #if ENABLED(CRASH_RECOVERY)
    const bool all_axes = (X && Y && Z);
    if (all_axes) {
      // Skip all recovery when homing all axes
      crash_s.set_gcode_replay_flags(Crash_s::RECOVER_NONE);
    } else if (X && Y) {
      // Skip the extra axis/position recovery if both XY are being rehomed
      crash_s.set_gcode_replay_flags(Crash_s::RECOVER_AXIS_STATE | Crash_s::RECOVER_Z_POSITION);
    }
  #endif

  G28_no_parser(X, Y, Z, flags);
}
/** @}*/

bool GcodeSuite::G28_no_parser(bool X, bool Y, bool Z, const G28Flags& flags) {
  struct AxisHomingRequirement {
    // !!! Keep before initial_level, someone might be not using a designated initializer and expect .expand({AxisHomeLevel::something}) to work.
    AxisHomeLevel required_level = AxisHomeLevel::not_homed;

    /// Whether the homing is optional or enforced
    bool force = false;

    /// Initial home level of the axis at homing start
    AxisHomeLevel initial_level = AxisHomeLevel::not_homed;

    void expand(const AxisHomingRequirement &other, bool condition = true) {
      if(condition) {
        required_level = std::max(required_level, other.required_level);
        force |= other.force;
      }
    }
  };
  std::array<AxisHomingRequirement, 3> requirements;

  /// \returns whether there should be any homing done for the specified axis during the G28
  const auto should_home_at_all = [&](AxisEnum axis) {
    const auto &r = requirements[axis];
    return r.force || (r.initial_level < r.required_level);
  };

  /// \returns whether the axis should be homed to (or above) the specified level during the G28
  /// In some cases, the G28 might only require precise refinement and not base homing,
  // at which point should_home_to_level(imprecise) would be false and should_home_to_level(full) would be true
  const auto should_home_to_level = [&](AxisEnum axis, AxisHomeLevel to_level = AxisHomeLevel::full) {
    const auto &r = requirements[axis];
    return (to_level <= r.required_level) && (r.force || r.initial_level < to_level);
  };

  // Setup basic requirements
  {
    const AxisHomingRequirement req {
      .required_level = (flags.precise ? AxisHomeLevel::full : AxisHomeLevel::imprecise),
      .force = !flags.only_if_needed,
    };

    for(uint8_t i = 0; i < axes_home_level.size(); i++) {
      requirements[i].initial_level = axes_home_level[i];
    }

    requirements[X_AXIS].expand(req, X);
    requirements[Y_AXIS].expand(req, Y);
    requirements[Z_AXIS].expand(req, Z);
  }

  // On Z_SAFE_HOMING, if we need to home Z, we need to have X and Y homed as well
  if(ENABLED(Z_SAFE_HOMING) && should_home_at_all(Z_AXIS)) {
    static constexpr AxisHomingRequirement req {
      .required_level = AxisHomeLevel::imprecise,
    };
    requirements[X_AXIS].expand(req);
    requirements[Y_AXIS].expand(req);
  }

  // On CODEPENDENT_XY_HOMING, we need to home both axes
  if constexpr(ENABLED(CODEPENDENT_XY_HOMING)) {
    requirements[X_AXIS].expand(requirements[Y_AXIS]);
    requirements[Y_AXIS] = requirements[X_AXIS];
  }

  // Precise COREXY homing is XY codependent, even if the imprecise homing isn't
  if(HAS_PRECISE_HOMING_COREXY() && (should_home_to_level(X_AXIS, AxisHomeLevel::full) || should_home_to_level(Y_AXIS, AxisHomeLevel::full))) {
    requirements[X_AXIS].expand(requirements[Y_AXIS]);
    requirements[Y_AXIS] = requirements[X_AXIS];
  }

  // Home (O)nly if position is unknown with respect to the required axes
  if (!should_home_at_all(X_AXIS) && !should_home_at_all(Y_AXIS) && !should_home_at_all(Z_AXIS)) {
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("> homing not needed, skip");
    return true;
  }


  #if ENABLED(MARLIN_DEV_MODE)
    if (flags.simulate) {
      planner.synchronize();
      if (planner.draining())
        return false;
      LOOP_NUM_AXES(a) set_axis_is_at_home((AxisEnum)a);
      sync_plan_position();
      SERIAL_ECHOLNPGM("Simulated Homing");
      report_current_position();
      return true;
    }
  #endif

  PrintStatusMessageGuard statusGuard;
  statusGuard.update<PrintStatusMessage::homing>({});
  HomingReporter reporter;

  #if HAS_CEILING_CLEARANCE()
  buddy::CeilingClearanceCheckDisabler ccd;
  #endif

  DEBUG_SECTION(log_G28, "G28", DEBUGGING(LEVELING));
  if (DEBUGGING(LEVELING)) log_machine_info();

  TERN_(BD_SENSOR, bdl.config_state = 0);

  /**
   * Set the laser power to false to stop the planner from processing the current power setting.
   */
  #if ENABLED(LASER_FEATURE)
    planner.laser_inline.status.isPowered = false;
  #endif

  #if ENABLED(FULL_REPORT_TO_HOST_FEATURE)
    const M_StateEnum old_grblstate = M_State_grbl;
    set_and_report_grblstate(M_HOMING);
  #endif

  TERN_(HAS_DWIN_E3V2_BASIC, DWIN_HomingStart());
  //TERN_(EXTENSIBLE_UI, ExtUI::onHomingStart());

  planner.synchronize();          // Wait for planner moves to finish!

  // If we are homing Z, assume that max_printed_z is zero (the Z homing couldn't be safe otherwise)
  if(Z && Z_HOME_DIR < 0) {
    planner.max_printed_z = 0;
  }

  /**
   * @brief Set to true when homing fails.
   * It is used to skip all motion until stepper currents
   * and variables are reverted to non-homing state.
   */
  bool failed = false;

  SET_SOFT_ENDSTOP_LOOSE(false);  // Reset a leftover 'loose' motion state

  // Disable the leveling matrix before homing
  #if CAN_SET_LEVELING_AFTER_G28
    const bool leveling_restore_state = parser.boolval('L', TERN1(RESTORE_LEVELING_AFTER_G28, planner.leveling_active));
  #endif

  // Cancel any prior G29 session
  TERN_(PROBE_MANUALLY, g29_in_progress = false);

  // Disable leveling before homing
  TERN_(HAS_LEVELING, set_bed_leveling_enabled(false));

  // Reset to the XY plane
  TERN_(CNC_WORKSPACE_PLANES, workspace_plane = PLANE_XY);

  #define HAS_CURRENT_HOME(N) (defined(N##_CURRENT_HOME) && N##_CURRENT_HOME != N##_CURRENT)
  #if HAS_CURRENT_HOME(X) || HAS_CURRENT_HOME(X2) || HAS_CURRENT_HOME(Y) || HAS_CURRENT_HOME(Y2) || HAS_CURRENT_HOME(I) || HAS_CURRENT_HOME(J) || HAS_CURRENT_HOME(K) || HAS_CURRENT_HOME(U) || HAS_CURRENT_HOME(V) || HAS_CURRENT_HOME(W)
    #define HAS_HOMING_CURRENT 1
  #endif

  #if HAS_HOMING_CURRENT
    auto debug_current = [](FSTR_P const s, const int16_t a, const int16_t b) {
      DEBUG_ECHOF(s); DEBUG_ECHOLNPGM(" current: ", a, " -> ", b);
    };
    #if HAS_CURRENT_HOME(X)
      const int16_t tmc_save_current_X = stepperX.getMilliamps();
      if(!flags.no_change) {
        stepperX.rms_current(X_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_X), tmc_save_current_X, X_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(X2)
      const int16_t tmc_save_current_X2 = stepperX2.getMilliamps();
      if(!no_change) {
        stepperX2.rms_current(X2_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_X2), tmc_save_current_X2, X2_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(Y)
      const int16_t tmc_save_current_Y = stepperY.getMilliamps();
      if(!flags.no_change) {
        stepperY.rms_current(Y_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_Y), tmc_save_current_Y, Y_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(Y2)
      const int16_t tmc_save_current_Y2 = stepperY2.getMilliamps();
      if(!no_change) {
        stepperY2.rms_current(Y2_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_Y2), tmc_save_current_Y2, Y2_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(I)
      const int16_t tmc_save_current_I = stepperI.getMilliamps();
      if(!no_change) {
        stepperI.rms_current(I_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_I), tmc_save_current_I, I_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(J)
      const int16_t tmc_save_current_J = stepperJ.getMilliamps();
      if(!no_change) {
        stepperJ.rms_current(J_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_J), tmc_save_current_J, J_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(K)
      const int16_t tmc_save_current_K = stepperK.getMilliamps();
      if(!no_change) {
        stepperK.rms_current(K_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_K), tmc_save_current_K, K_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(U)
      const int16_t tmc_save_current_U = stepperU.getMilliamps();
      if(!no_change) {
        stepperU.rms_current(U_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_U), tmc_save_current_U, U_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(V)
      const int16_t tmc_save_current_V = stepperV.getMilliamps();
      if(!no_change) {
        stepperV.rms_current(V_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_V), tmc_save_current_V, V_CURRENT_HOME);
      }
    #endif
    #if HAS_CURRENT_HOME(W)
      const int16_t tmc_save_current_W = stepperW.getMilliamps();
      if(!no_change) {
        stepperW.rms_current(W_CURRENT_HOME);
        if (DEBUGGING(LEVELING)) debug_current(F(STR_W), tmc_save_current_W, W_CURRENT_HOME);
      }
    #endif
    #if SENSORLESS_STALLGUARD_DELAY
      safe_delay(SENSORLESS_STALLGUARD_DELAY); // Short delay needed to settle
    #endif
  #endif

  #if ENABLED(XY_HOMING_STEALTCHCHOP)
    bool stealthChop_x = stepperX.stored.stealthChop_enabled;
    bool stealthChop_y = stepperY.stored.stealthChop_enabled;
    // Enable stealtchop on X and Y before homing
    stepperX.stored.stealthChop_enabled = true;
    stepperX.refresh_stepping_mode();
    stepperY.stored.stealthChop_enabled = true;
    stepperY.refresh_stepping_mode();
  #endif /*ENABLED(XY_HOMING_STEALTCHCHOP)*/

  #if ENABLED(IMPROVE_HOMING_RELIABILITY)
    Motion_Parameters saved_motion_state;
    saved_motion_state.save();
    ScopeGuard restore_motion_state_guard = [&] {
      saved_motion_state.load();
    };
    if (!flags.no_change) {
      // Reset default feedrate and acceleration limits during homing
      Motion_Parameters::reset(true);

      auto s = planner.user_settings;
      s.max_acceleration_mm_per_s2[X_AXIS] = XY_HOMING_ACCELERATION;
      s.max_acceleration_mm_per_s2[Y_AXIS] = XY_HOMING_ACCELERATION;
      s.travel_acceleration = XY_HOMING_ACCELERATION;
      #if HAS_CLASSIC_JERK
        s.max_jerk.set(XY_HOMING_JERK, XY_HOMING_JERK);
      #endif

      // use feedrates as given! These shouldn't be alterated by any other policy!
      planner.apply_settings(s, true);
    }
  #endif

  // Always home with tool 0 active (but not with PRUSA_TOOLCHANGER)
  #if HAS_MULTI_HOTEND && DISABLED(PRUSA_TOOLCHANGER)
    const uint8_t old_tool_index = active_extruder;
    // PARKING_EXTRUDER homing requires different handling of movement / solenoid activation, depending on the side of homing
    #if ENABLED(PARKING_EXTRUDER)
      const bool pe_final_change_must_unpark = parking_extruder_unpark_after_homing(old_tool_index, X_HOME_DIR + 1 == old_tool_index * 2);
    #endif
    tool_change(0, tool_return_t::no_return);
  #endif


  TERN_(HAS_DUPLICATION_MODE, set_duplication_enabled(false));

  // Disable phase stepping/PA just before homing XY. This will synchronize only if needed
  phase_stepping::EnsureSuitableForHoming phstep_disabler;
  pressure_advance::PressureAdvanceDisabler pa_disabler;

  // Homing feedrate
  float fr_mm_s = flags.no_change ? feedrate_mm_s : 0.0f;
  remember_feedrate_scaling_off();
  ScopeGuard resatore_feedrate_scaling = [] {
    restore_feedrate_and_scaling();
  };

  endstops.enable(true); // Enable endstops for next homing move

  TERN_(HOME_Z_FIRST, if (!failed && doZ) failed = !homeaxis(Z_AXIS));

  const bool seenR = !isnan(flags.z_raise);
  const float z_homing_height = seenR ? flags.z_raise : Z_HOMING_HEIGHT;

  if (!failed && z_homing_height && (seenR || should_home_at_all(X_AXIS) || should_home_at_all(Y_AXIS) || TERN0(Z_SAFE_HOMING, should_home_at_all(Z_AXIS)))) {
    // Raise Z before homing any other axes and z is not already high enough (never lower z)
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("Raise Z (before homing) by ", z_homing_height);
    const auto trigger_states = do_z_clearance(z_homing_height);

    // If we have the Z homed and trigger an endstop, that means that we have homed wrong.
    // Letting this continue could lead to collision with the print or with the bedsheet, so rather raise a redscreen.
    // BFW-5334
    if((trigger_states & (1 << EndstopEnum::Z_MAX)) && !should_home_at_all(Z_AXIS) && axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
      raise_redscreen(ErrCode::ERR_UNDEF, "Unexpected Z MAX endstop trigger", "G28");
    }
    TERN_(BLTOUCH, bltouch.init());
  }

  // Diagonal move first if both are homing
  #if ENABLED(QUICK_HOME)
    if (!failed && should_home_to_level(X_AXIS, AxisHomeLevel::imprecise) && should_home_to_level(Y_AXIS, AxisHomeLevel::imprecise))
      quick_home_xy();
  #endif

#if HAS_TRINAMIC && defined(HAS_TMC_WAVETABLE)
  // Only allow wavetable change if homing performs a backoff. This backoff is made in the way that it ends on stepper zero-position, so that re-enabling wavetable is safe.
  bool wavetable_off_X = false, wavetable_off_Y = false;
  #if HAS_PRECISE_HOMING_COREXY()
    #error "wavetable switching currently not compatible with HAS_PRECISE_HOMING_COREXY()"
  #endif
  #ifdef HOMING_BACKOFF_POST_MM
    constexpr xyz_float_t homing_backoff = HOMING_BACKOFF_POST_MM;
    wavetable_off_X = (homing_backoff[X] > 0.0f) && should_home_at_all(X_AXIS);
    wavetable_off_Y = (homing_backoff[Y] > 0.0f) && should_home_at_all(Y_AXIS);
  #endif
  void (*reenable_wt_X)(AxisEnum) = wavetable_off_X ? reenable_wavetable : NULL;
  void (*reenable_wt_Y)(AxisEnum) = wavetable_off_Y ? reenable_wavetable : NULL;
  // function pointers are useful because homing code doesn't need access to trinamic headres in order to re-enable wavetable during homing procedures

  // Turn off Wave Table for X and Y, which should improve homing
  // NOTE: change of Wave Table shall normally be done only when motors are guaranteed at zero-step. Here we are far enough from the print, so if the motors do something "wild" they should make no harm.
  // re-enabling wavetable back will take place during homing, when we are guaranteed at stepper zero
  if (!failed) {
    tmc_disable_wavetable(wavetable_off_X, wavetable_off_Y, false);
  }
#else
  void (*reenable_wt_X)(AxisEnum) = NULL;
  void (*reenable_wt_Y)(AxisEnum) = NULL;
#endif

  #if ENABLED(PRUSA_TOOLCHANGER)
  if (!failed && should_home_at_all(X_AXIS) && should_home_at_all(Y_AXIS)) {
    // Bump right edge to align toolchanger locking plates
    if (!prusa_toolchanger.align_locks()) {
      ui.status_printf_P(0, "Toolchanger lock alignment failed");
      homing_failed([]() { fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_X); }); // The alignment happens in X axis
      failed = true;
    }
  }

  // X position is unknown or near right edge
  if (!failed && (axes_need_homing(_BV(X_AXIS)) || current_position.x > X_MAX_POS - MOVE_BACK_BEFORE_HOMING_DISTANCE)) {
    do_homing_move(X_AXIS, -1 * MOVE_BACK_BEFORE_HOMING_DISTANCE); // Move a bit left to avoid unlocking the tool
  }
  #endif /*ENABLED(PRUSA_TOOLCHANGER)*/

  // Home Y (before X)
  if (ENABLED(HOME_Y_BEFORE_X) && !failed && should_home_to_level(Y_AXIS, AxisHomeLevel::imprecise)) {
    failed = !homeaxis(Y_AXIS, fr_mm_s, false, reenable_wt_Y, flags.can_calibrate);
  }

  // Home X
  if (!failed && should_home_to_level(X_AXIS, AxisHomeLevel::imprecise)) {
#ifdef HOMING_PREEMPTIVE_MOVE_Y
    do_blocking_move_to_y(HOMING_PREEMPTIVE_MOVE_Y);
#endif
    failed = !homeaxis(X_AXIS, fr_mm_s, false, reenable_wt_X, flags.can_calibrate);
  }

  // Home Y (after X)
  if (DISABLED(HOME_Y_BEFORE_X) && !failed && should_home_to_level(Y_AXIS, AxisHomeLevel::imprecise)) {
    failed = !homeaxis(Y_AXIS, fr_mm_s, false, reenable_wt_Y, flags.can_calibrate);
  }

  #if HAS_PRECISE_HOMING_COREXY()
    // absolute refinement requires both axes to be already probed
    if (!failed && (should_home_to_level(X_AXIS, AxisHomeLevel::full) || should_home_to_level(Y_AXIS, AxisHomeLevel::full))) {
      #if DISABLED(PRUSA_TOOLCHANGER)
        // skip refinement without data if we're not allowed to calibrate
        const bool do_refine = (corexy_home_is_calibrated() || flags.can_calibrate || flags.force_calibrate);
      #else
        // TODO[BFW-6527]: this is a temporary workaround until home calibration is enforced
        const bool do_refine = true;
      #endif

      if(do_refine) {
        failed |= !corexy_refine_during_G28(fr_mm_s, flags);

      }
    }
  #endif

  // Restore XY stealthChop / spreadCycle mode
  #if ENABLED(XY_HOMING_STEALTCHCHOP)
    // Enable stealtchop on X and Y before homing
    stepperX.stored.stealthChop_enabled = stealthChop_x;
    stepperX.refresh_stepping_mode();
    stepperY.stored.stealthChop_enabled = stealthChop_y;
    stepperY.refresh_stepping_mode();
  #endif /*ENABLED(XY_HOMING_STEALTCHCHOP)*/

  // Home Z last if homing towards the bed
  #if HAS_Z_AXIS && DISABLED(HOME_Z_FIRST)
    if (!failed && should_home_at_all(Z_AXIS)) {
      #if EITHER(Z_MULTI_ENDSTOPS, Z_STEPPER_AUTO_ALIGN)
        stepper.set_all_z_lock(false);
        stepper.set_separate_multi_axis(false);
      #endif

      #if ENABLED(PRUSA_TOOLCHANGER)
      if (active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        // When no tool is picked, make sure to pick one
        failed = !prusa_toolchanger.tool_change(0, tool_return_t::no_return, current_position, tool_change_lift_t::no_lift, false);
      }
      #endif

      if (!failed) {
      #if ENABLED(Z_SAFE_HOMING)
        failed = !home_z_safely();

        #if ENABLED(DETECT_PRINT_SHEET)
        if (!failed && flags.check_sheet) {
          PrintStatusMessageGuard status_guard;
          status_guard.update<PrintStatusMessage::detecting_steel_sheet>({});

          failed = [&] {
            // Do multiple attempts of detect print sheet
            // The point is that we want to prevent false failures caused by a dirty nozzle (cold filament left hanging out)
            // BFW-5028
            uint8_t attempt = 0;
            while(true) {
              // If detect_print_sheet, return success (failed -> false)
              if(detect_print_sheet(z_homing_height)) {
                return false;
              }

              // Fail straight away if draining
              if(planner.draining()) {
                return true;
              }

              bool ignore_fail = false;

              // Ran out of attempts -> report detect fail
              if(++attempt == 3) {
                // Report missing bed sheet
                static constexpr auto warning_type = WarningType::SteelSheetNotDetected;
                marlin_server::set_warning(warning_type);

                // Move the bed to the bottom to give space for the user to insert the sheet
                // Do this asynchronously so that we can process the response while moving
                current_position.z = DETECT_PRINT_SHEET_Z_AFTER_FAILURE;
                line_to_current_position(homing_feedrate(Z_AXIS));

                // Continue after the user puts the print sheet on
                const Response response = marlin_server::wait_for_response(warning_type_phase(warning_type));

                marlin_server::clear_warning(warning_type);

                // Wait for the movement to finish
                planner.synchronize();
                ignore_fail = (response == Response::Ignore);
                attempt = 0;
              }

              // Raise the Z again to prevent crashing into the sheet
              do_z_clearance(z_homing_height);

              // Return to the XY homing position over the printbed and try rehoming z
              if(!home_z_safely()) {
                // Fail straight away if z homing fails, only repeat if detect_print_sheet fails
                return true;
              }

              if(ignore_fail) {
                return false;
              }
            }
          }();
        }
        #endif
      #else
        failed = !homeaxis(Z_AXIS);
      #endif
      }

      if (!failed) {
        move_z_after_probing();
      }
    }
  #endif

  SECONDARY_AXIS_CODE(
    if (!failed && doI) failed = !homeaxis(I_AXIS),
    if (!failed && doJ) failed = !homeaxis(J_AXIS),
    if (!failed && doK) failed = !homeaxis(K_AXIS),
    if (!failed && doU) failed = !homeaxis(U_AXIS),
    if (!failed && doV) failed = !homeaxis(V_AXIS),
    if (!failed && doW) failed = !homeaxis(W_AXIS)
  );

  sync_plan_position();

  // clear any step fraction: we're at home
  PreciseStepping::reset_from_halt(false);

  endstops.not_homing();

  // Restore previous phase stepping state before we move again
  phstep_disabler.release();

  if (!failed) {
    // Move to a height where we can use the full xy-area
    TERN_(DELTA_HOME_TO_SAFE_ZONE, do_blocking_move_to_z(delta_clip_start_height));
  }

  TERN_(CAN_SET_LEVELING_AFTER_G28, if (leveling_restore_state) set_bed_leveling_enabled());

  // Restore the active tool after homing
  #if HAS_MULTI_HOTEND && DISABLED(PRUSA_TOOLCHANGER)
    tool_change(old_tool_index, TERN(PARKING_EXTRUDER, !pe_final_change_must_unpark, DISABLED(DUAL_X_CARRIAGE)));   // Do move if one of these
  #endif

  #if HAS_HOMING_CURRENT
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("Restore driver current...");
    #if HAS_CURRENT_HOME(X)
      stepperX.rms_current(tmc_save_current_X);
    #endif
    #if HAS_CURRENT_HOME(X2)
      stepperX2.rms_current(tmc_save_current_X2);
    #endif
    #if HAS_CURRENT_HOME(Y)
      stepperY.rms_current(tmc_save_current_Y);
    #endif
    #if HAS_CURRENT_HOME(Y2)
      stepperY2.rms_current(tmc_save_current_Y2);
    #endif
    #if HAS_CURRENT_HOME(I)
      stepperI.rms_current(tmc_save_current_I);
    #endif
    #if HAS_CURRENT_HOME(J)
      stepperJ.rms_current(tmc_save_current_J);
    #endif
    #if HAS_CURRENT_HOME(K)
      stepperK.rms_current(tmc_save_current_K);
    #endif
    #if HAS_CURRENT_HOME(U)
      stepperU.rms_current(tmc_save_current_U);
    #endif
    #if HAS_CURRENT_HOME(V)
      stepperV.rms_current(tmc_save_current_V);
    #endif
    #if HAS_CURRENT_HOME(W)
      stepperW.rms_current(tmc_save_current_W);
    #endif
    #if SENSORLESS_STALLGUARD_DELAY
      safe_delay(SENSORLESS_STALLGUARD_DELAY); // Short delay needed to settle
    #endif
  #endif // HAS_HOMING_CURRENT

  ui.refresh();

  TERN_(HAS_DWIN_E3V2_BASIC, DWIN_HomingDone());
  //TERN_(EXTENSIBLE_UI, ExtUI::onHomingDone());

  report_current_position();

  if (ENABLED(NANODLP_Z_SYNC) && (should_home_at_all(Z_AXIS) || ENABLED(NANODLP_ALL_AXIS)))
    SERIAL_ECHOLNPGM(STR_Z_MOVE_COMP);

  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(old_grblstate));

  return !failed;
}

#if HAS_PRECISE_HOMING_COREXY()
enum class RefineResult {
  success,
  hard_fail, ///< Failed hard, cannot recover
  refine_fail, ///< Failed to refine, but could potentially continue unrefined
  calibrate_from_menu, ///< Needs calibration but cannot do it en-situ. The user needs to abort/ignroe and do it from the calibrations menu.
};

RefineResult corexy_calibrate_homing_during_G28(float xy_mm_s, const G28Flags &flags) {
  // We cannot calibrate -> this are bad, abort
  if (!flags.can_calibrate) {
    return RefineResult::calibrate_from_menu;
  }

  Tristate calibration_approved = flags.force_calibrate ? Tristate::yes : config_store().auto_recalibrate_precise_homing.get();

  // Prompt the user that we would like to do the calibration (if the calibration was not triggered from gcode)
  if(calibration_approved == Tristate::other) {
    switch(marlin_server::prompt_warning(WarningType::HomingCalibrationNeeded, 60'000)) {

    case Response::Calibrate:
    case Response::_none: // In case of timeout
    default: // To stop the compiler from complaining
      calibration_approved = Tristate::yes;
      break;

    case Response::Skip:
      calibration_approved = Tristate::no;
      break;

    case Response::Always:
      calibration_approved = Tristate::yes;
      config_store().auto_recalibrate_precise_homing.set(Tristate::yes);
      break;

    case Response::Never:
      calibration_approved = Tristate::no;
      config_store().auto_recalibrate_precise_homing.set(Tristate::no);
      break;

    }
  }

  // Regardless of whether the calibration will run or not, reset homing instability history
  // In both cases, we want a clean slate so that the user is not bothered with "please recalibrate" right away
  config_store().precise_homing_instability_history.set(0);

  if(!calibration_approved) {
    // User decided to not do the calibration at his own risk -> consider the point refined
    return RefineResult::success;
  }

  PrintStatusMessageGuard status_guard;
  status_guard.update<PrintStatusMessage::recalibrating_home>({});

  /// Wait holder should be part of the homing procedure
  marlin_server::fsm_change(PhaseWait::homing_calibration);

#if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
  // Try calibrating motor sensitivity before calibration
  if (!corexy_sens_calibrate(xy_mm_s)) {
    return RefineResult::hard_fail;
  }

  // After calibrating sensitivity, we need to rehome
  if (!corexy_rehome_xy(xy_mm_s)) {
    return RefineResult::hard_fail;
  }
#endif

  if (!corexy_home_refine(xy_mm_s, CoreXYCalibrationMode::force)) {
    return RefineResult::refine_fail;
  }

  return RefineResult::success;
}

RefineResult corexy_refine_during_G28_once(float fr_mm_s, const G28Flags &flags) {
  // Do a quick move to the home position. Refinement can now be done separately to the imprecise homing and the head can be anywhere
  // The position taken from corexy_rehome_and_phase
  do_blocking_move_to_xy(
    (base_home_pos(X_AXIS) - XY_HOMING_ORIGIN_OFFSET * X_HOME_DIR),
    (base_home_pos(Y_AXIS) - XY_HOMING_ORIGIN_OFFSET * Y_HOME_DIR)
  );

  // Do not handle feedrate defaults again within precise homing: do it here
  fr_mm_s = fr_mm_s ?: homing_feedrate(A_AXIS);

  if (flags.force_calibrate || !corexy_home_is_calibrated()) {
    // Calibration is required, do not even attempt to naively refine
    return corexy_calibrate_homing_during_G28(fr_mm_s, flags);
  }

  // Retry the refinement a few times
  for (uint8_t retry = 0; retry < PRECISE_HOMING_COREXY_RETRIES; retry++) {
    if (planner.draining()) {
      return RefineResult::hard_fail;
    }

    if (corexy_home_refine(fr_mm_s, CoreXYCalibrationMode::disallow)) {
      // Successfully refined the home

      // Record whether the refinement found a stable home
      config_store().precise_homing_instability_history.transform([is_unstable = corexy_home_is_unstable()] (auto val) {
        return (val << 1) | is_unstable;
      });

      // If at least history_threshold out of last history_window refinements were unstable, trigger calibration
      static constexpr auto history_threshold = 6;
      static constexpr auto history_window = 10;

      // Check that we have the requested history window actually stored
      using History = decltype(config_store_ns::CurrentStore::precise_homing_instability_history)::value_type;
      static_assert(history_window <= sizeof(History) * 8);
      static constexpr History history_mask = ((1 << history_window) - 1);

      const auto recent_instability_count = std::popcount(static_cast<History>(config_store().precise_homing_instability_history.get() & history_mask));
      if(flags.can_calibrate && recent_instability_count >= history_threshold) {
        break; // Break out of the loop -> offer calibration
      }

      return RefineResult::success;
    }

    // instead of blindly retrying internally on the same location, move the gantry
    if (!corexy_rehome_xy(fr_mm_s)) {
      return RefineResult::hard_fail;
    }
  }

  // If we've exhausted retries, force calibration
  return corexy_calibrate_homing_during_G28(fr_mm_s, flags);
}

bool corexy_refine_during_G28(float fr_mm_s, const G28Flags &flags) {
  RefineResult result;
  while (true) {
    result = corexy_refine_during_G28_once(fr_mm_s, flags);
    switch(result) {

      case RefineResult::success:
        // Mark the axes as precisely homed
        axes_home_level[X_AXIS] = AxisHomeLevel::full;
        axes_home_level[Y_AXIS] = AxisHomeLevel::full;
        return true;

      case RefineResult::hard_fail:
        return false;

      case RefineResult::refine_fail:
      case RefineResult::calibrate_from_menu:
        break;
    }

    if(planner.draining()) {
      // We're quick stopping, do not repeat
      return false;
    }

    const auto warning_type = (result == RefineResult::calibrate_from_menu) ? WarningType::HomingCalibrationFromMenuNeeded : WarningType::HomingRefinementFailed;
    const auto prompt_result = marlin_server::prompt_warning(warning_type);
    switch(prompt_result) {

    case Response::Retry:
      continue;

    case Response::Abort:
      marlin_server::quick_stop();
      marlin_server::print_abort();
      return false;

    case Response::Ignore:
      // The user decided to ignore the problem at his own risk, consider the homing calibrated for now
      axes_home_level[X_AXIS] = AxisHomeLevel::full;
      axes_home_level[Y_AXIS] = AxisHomeLevel::full;   
      return true;

    default:
      BUDDY_UNREACHABLE();

    }
  }
}
#endif
