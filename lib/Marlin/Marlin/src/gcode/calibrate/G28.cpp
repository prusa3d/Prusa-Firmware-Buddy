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
  #include "../../feature/tmc_util.h"
#endif

#if ENABLED(CRASH_RECOVERY)
  #include "../../feature/prusa/crash_recovery.hpp"
#endif

#if ENABLED(PRECISE_HOMING_COREXY)
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

#include "../../../../../../src/common/trinamic.h" // for disabling Wave Table during homing
#include <feature/phase_stepping/phase_stepping.hpp> // for disabling phase stepping during homing

#include <option/has_nozzle_cleaner.h>

#if ENABLED(DELTA) || ENABLED(SCARA) || ENABLED(AXEL_TPARA) || ENABLED(DUAL_X_CARRIAGE) || ENABLED(FOAMCUTTER_XYUV)
  #error These babies are no longer welcome here. The relevants ifdefs were removed.
#endif

#include <scope_guard.hpp>
#include <marlin_server.hpp>

#if ENABLED(QUICK_HOME)

  static void quick_home_xy() {

    // Pretend the current position is 0,0
    CBI(axis_known_position, X_AXIS);
    CBI(axis_known_position, Y_AXIS);
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
          TERN0(X_SENSORLESS, tmc_enable_stallguard(stepperX)),
          TERN0(Y_SENSORLESS, tmc_enable_stallguard(stepperY)),
          false, false, false, false
        )
        , TERN0(X2_SENSORLESS, tmc_enable_stallguard(stepperX2))
        , TERN0(Y2_SENSORLESS, tmc_enable_stallguard(stepperY2))
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
        TERN_(X_SENSORLESS, tmc_disable_stallguard(stepperX, stealth_states.x));
        TERN_(X2_SENSORLESS, tmc_disable_stallguard(stepperX2, stealth_states.x2));
        TERN_(Y_SENSORLESS, tmc_disable_stallguard(stepperY, stealth_states.y));
        TERN_(Y2_SENSORLESS, tmc_disable_stallguard(stepperY2, stealth_states.y2));
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

    destination.set(okay_homing_xy, current_position.z);

    TERN_(HOMING_Z_WITH_PROBE, destination -= probe_offset);
    TERN_(HAS_HOTEND_OFFSET, destination -= hotend_currently_applied_offset);

    if (position_is_reachable(destination)) {

      if (DEBUGGING(LEVELING)) DEBUG_POS("home_z_safely", destination);

      TERN_(SENSORLESS_HOMING, safe_delay(500)); // Short delay needed to settle

#if ENABLED(PRUSA_TOOLCHANGER)
      do_blocking_move_to_xy(destination, PrusaToolChanger::limit_stealth_feedrate(XY_PROBE_FEEDRATE_MM_S));
#elif HAS_NOZZLE_CLEANER()
    // with nozzle cleaner (iX), move in Y first to avoid going over the cleaner
    do_blocking_move_to_xy(current_position.x, destination.y);
    do_blocking_move_to_xy(destination.x, destination.y);
#else
      do_blocking_move_to_xy(destination);
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
    static bool detect_print_sheet(const_float_t z_homing_height) {
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

        TERN_(SENSORLESS_HOMING, safe_delay(500)); // Short delay needed to settle

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

static void reenable_wavetable(AxisEnum axis)
{
    tmc_enable_wavetable(axis == X_AXIS, axis == Y_AXIS, false);
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *###G28: Home all axes according to settings <a href="https://reprap.org/wiki/G-code#G28:_Move_to_Origin_.28Home.29">G28: Move to Origin (Home)</a>
 *
 *If `PRECISE_HOMING` is enabled, there are specific amount
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
  #if ENABLED(MARLIN_DEV_MODE)
    flags.simulate = parser.seen('S')
  #endif
  #if ENABLED(DETECT_PRINT_SHEET)
    flags.check_sheet = !parser.boolval('P');
  #endif
  #if ENABLED(PRECISE_HOMING_COREXY)
    flags.precise = !parser.seen('I'); // imprecise: do not perform precise refinement
  #endif

  G28_no_parser(X, Y, Z, flags);
}
/** @}*/

bool GcodeSuite::G28_no_parser(bool only_if_needed, float z_raise, bool simulate_homing, bool X, bool Y, bool Z
  , bool no_change OPTARG(PRECISE_HOMING_COREXY, bool precise) OPTARG(DETECT_PRINT_SHEET, bool check_sheet)) {

  G28Flags flags;
  flags.only_if_needed = only_if_needed;
  flags.z_raise = z_raise;
  flags.no_change = no_change;
  #if ENABLED(MARLIN_DEV_MODE)
    flags.simulate = simulate;
  #endif
  #if ENABLED(PRECISE_HOMING_COREXY)
    flags.precise = precise;
  #endif
  #if ENABLED(DETECT_PRINT_SHEET)
    flags.check_sheet = check_sheet;
  #endif

  return G28_no_parser(X, Y, Z, flags);
}

bool GcodeSuite::G28_no_parser(bool X, bool Y, bool Z, const G28Flags& flags) {
  HomingReporter reporter;

  DEBUG_SECTION(log_G28, "G28", DEBUGGING(LEVELING));
  if (DEBUGGING(LEVELING)) log_machine_info();

  TERN_(BD_SENSOR, bdl.config_state = 0);

  /**
   * Set the laser power to false to stop the planner from processing the current power setting.
   */
  #if ENABLED(LASER_FEATURE)
    planner.laser_inline.status.isPowered = false;
  #endif

  #if ENABLED(MARLIN_DEV_MODE)
    if (S) {
      LOOP_NUM_AXES(a) set_axis_is_at_home((AxisEnum)a);
      sync_plan_position();
      SERIAL_ECHOLNPGM("Simulated Homing");
      report_current_position();
      return;
    }
  #endif

  // Home (O)nly if position is unknown with respect to the required axes
  uint8_t required_axis_bits = 0;
  if(X) SBI(required_axis_bits, X_AXIS);
  if(Y) SBI(required_axis_bits, Y_AXIS);
  if(Z) SBI(required_axis_bits, Z_AXIS);
  if(!X && !Y && !Z) {
    // None specified -> need all
    SBI(required_axis_bits, X_AXIS);
    SBI(required_axis_bits, Y_AXIS);
    SBI(required_axis_bits, Z_AXIS);
  }
  if (!axes_should_home(required_axis_bits) && flags.only_if_needed) {
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("> homing not needed, skip");
    return true;
  }

  #if ENABLED(FULL_REPORT_TO_HOST_FEATURE)
    const M_StateEnum old_grblstate = M_State_grbl;
    set_and_report_grblstate(M_HOMING);
  #endif

  TERN_(HAS_DWIN_E3V2_BASIC, DWIN_HomingStart());
  //TERN_(EXTENSIBLE_UI, ExtUI::onHomingStart());

  planner.synchronize();          // Wait for planner moves to finish!

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

  // Count this command as movement / activity
  reset_stepper_timeout();

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
      Motion_Parameters::reset();

      auto s = planner.user_settings;
      s.max_acceleration_mm_per_s2[X_AXIS] = XY_HOMING_ACCELERATION;
      s.max_acceleration_mm_per_s2[Y_AXIS] = XY_HOMING_ACCELERATION;
      s.travel_acceleration = XY_HOMING_ACCELERATION;
      #if HAS_CLASSIC_JERK
        s.max_jerk.set(XY_HOMING_JERK, XY_HOMING_JERK);
      #endif
      planner.apply_settings(s);
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

  // Disable phase stepping just before homing XY. This will synchronize only if needed
  phase_stepping::EnsureSuitableForHoming phstep_disabler;

  // Homing feedrate
  float fr_mm_s = flags.no_change ? feedrate_mm_s : 0.0f;
  remember_feedrate_scaling_off();
  ScopeGuard resatore_feedrate_scaling = [] {
    restore_feedrate_and_scaling();
  };

  endstops.enable(true); // Enable endstops for next homing move

  #define _UNSAFE(A) (homeZ && TERN0(Z_SAFE_HOMING, axes_should_home(_BV(A##_AXIS))))

  const bool homeZ = TERN0(HAS_Z_AXIS, Z),
              NUM_AXIS_LIST(              // Other axes should be homed before Z safe-homing
                needX = _UNSAFE(X), needY = _UNSAFE(Y), needZ = false, // UNUSED
                needI = _UNSAFE(I), needJ = _UNSAFE(J), needK = _UNSAFE(K),
                needU = _UNSAFE(U), needV = _UNSAFE(V), needW = _UNSAFE(W)
              ),
              NUM_AXIS_LIST(              // Home each axis if needed or flagged
                homeX = needX || X,
                homeY = needY || Y,
                homeZZ = homeZ,
                homeI = needI || parser.seen_test(AXIS4_NAME), homeJ = needJ || parser.seen_test(AXIS5_NAME),
                homeK = needK || parser.seen_test(AXIS6_NAME), homeU = needU || parser.seen_test(AXIS7_NAME),
                homeV = needV || parser.seen_test(AXIS8_NAME), homeW = needW || parser.seen_test(AXIS9_NAME)
              ),
              home_all = NUM_AXIS_GANG(   // Home-all if all or none are flagged
                  homeX == homeX, && homeY == homeX, && homeZ == homeX,
                && homeI == homeX, && homeJ == homeX, && homeK == homeX,
                && homeU == homeX, && homeV == homeX, && homeW == homeX
              ),
              NUM_AXIS_LIST(
                doX = home_all || homeX, doY = home_all || homeY, doZ = home_all || homeZ,
                doI = home_all || homeI, doJ = home_all || homeJ, doK = home_all || homeK,
                doU = home_all || homeU, doV = home_all || homeV, doW = home_all || homeW
              );

  #if HAS_Z_AXIS
    UNUSED(needZ); UNUSED(homeZZ);
  #else
    constexpr bool doZ = false;
  #endif

  TERN_(HOME_Z_FIRST, if (!failed && doZ) failed = !homeaxis(Z_AXIS));

  const bool seenR = !isnan(flags.z_raise);
  const float z_homing_height = seenR ? flags.z_raise : Z_HOMING_HEIGHT;

  if (!failed && z_homing_height && (seenR || NUM_AXIS_GANG(doX, || doY, || TERN0(Z_SAFE_HOMING, doZ), || doI, || doJ, || doK, || doU, || doV, || doW))) {
    // Raise Z before homing any other axes and z is not already high enough (never lower z)
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("Raise Z (before homing) by ", z_homing_height);
    const auto trigger_states = do_z_clearance(z_homing_height);

    // If we have the Z homed and trigger an endstop, that means that we have homed wrong.
    // Letting this continue could lead to collision with the print or with the bedsheet, so rather raise a redscreen.
    // BFW-5334
    if((trigger_states & (1 << EndstopEnum::Z_MAX)) && !doZ && !axes_need_homing(_BV(Z_AXIS))) {
      raise_redscreen(ErrCode::ERR_UNDEF, "Unexpected Z MAX endstop trigger", "G28");
    }
    TERN_(BLTOUCH, bltouch.init());
  }

  // Diagonal move first if both are homing
  TERN_(QUICK_HOME, if (!failed && doX && doY) quick_home_xy());

  // Only allow wavetable change if homing performs a backoff. This backoff is made in the way that it ends on stepper zero-position, so that re-enabling wavetable is safe.
  bool wavetable_off_X = false, wavetable_off_Y = false;
  #if defined(HOMING_BACKOFF_POST_MM) && defined(HAS_TMC_WAVETABLE)
    constexpr xyz_float_t homing_backoff = HOMING_BACKOFF_POST_MM;
    wavetable_off_X = (homing_backoff[X] > 0.0f) && doX;
    wavetable_off_Y = (homing_backoff[Y] > 0.0f) && doY;
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

  #if ENABLED(PRUSA_TOOLCHANGER)
  if (!failed && doX && doY) {
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
  if (ENABLED(HOME_Y_BEFORE_X) && !failed && (doY || TERN0(CODEPENDENT_XY_HOMING, doX))) {
    failed = !homeaxis(Y_AXIS, fr_mm_s, false, reenable_wt_Y, flags.can_calibrate);
  }

  // Home X
  if (!failed && (doX || (doY && ENABLED(CODEPENDENT_XY_HOMING) && DISABLED(HOME_Y_BEFORE_X)))) {
    failed = !homeaxis(X_AXIS, fr_mm_s, false, reenable_wt_X, flags.can_calibrate);
  }

  // Home Y (after X)
  if (DISABLED(HOME_Y_BEFORE_X) && !failed && doY) {
    failed = !homeaxis(Y_AXIS, fr_mm_s, false, reenable_wt_Y, flags.can_calibrate);
  }

  #if ENABLED(PRECISE_HOMING_COREXY)
    // absolute refinement requires both axes to be already probed
    if (!failed && ( doX || ENABLED(CODEPENDENT_XY_HOMING)) && doY && flags.precise) {
      failed = !refine_corexy_origin(flags.can_calibrate);
      if (failed && !planner.draining()) {
        homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); });
      }
    }
  #endif

  // Home Z last if homing towards the bed
  #if HAS_Z_AXIS && DISABLED(HOME_Z_FIRST)
    if (!failed && doZ) {
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
        if (TERN1(POWER_LOSS_RECOVERY, !parser.seen_test('H'))) {
          failed = !home_z_safely();

          #if ENABLED(DETECT_PRINT_SHEET)
          if (!failed && flags.check_sheet) {
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

                bool ignore_fail = false;

                // Ran out of attempts -> report detect fail
                if(++attempt == 3) {
                  // Report missing bed sheet
                  static constexpr auto warning_type = WarningType::SteelSheetNotDetected;
                  static constexpr auto warning_phase = PhasesWarning::SteelSheetNotDetected;
                  marlin_server::set_warning(warning_type, warning_phase);

                  // Move the bed to the bottom to give space for the user to insert the sheet
                  // Do this asynchronously so that we can process the response while moving
                  current_position.z = DETECT_PRINT_SHEET_Z_AFTER_FAILURE;
                  line_to_current_position(homing_feedrate(Z_AXIS));

                  // Continue after the user puts the print sheet on
                  const Response response = marlin_server::wait_for_response(warning_phase);

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

        } else {
          failed = !homeaxis(Z_AXIS);
        }
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
    // Clear endstop state for polled stallGuard endstops
    TERN_(SPI_ENDSTOPS, endstops.clear_endstop_state());

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

  if (ENABLED(NANODLP_Z_SYNC) && (doZ || ENABLED(NANODLP_ALL_AXIS)))
    SERIAL_ECHOLNPGM(STR_Z_MOVE_COMP);

  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(old_grblstate));

  return !failed;
}
