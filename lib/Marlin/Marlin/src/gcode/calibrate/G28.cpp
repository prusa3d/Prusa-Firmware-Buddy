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

#ifdef MINDA_BROKEN_CABLE_DETECTION
    #include "minda_broken_cable_detection.h"
#else
static inline void MINDA_BROKEN_CABLE_DETECTION__BEGIN() {}
static inline void MINDA_BROKEN_CABLE_DETECTION__PRE_XYHOME() {}
static inline void MINDA_BROKEN_CABLE_DETECTION__POST_XYHOME() {}
static inline void MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_1() {}
static inline void MINDA_BROKEN_CABLE_DETECTION__END() {}
#endif   

#include "../gcode.h"

#include "../../module/endstops.h"
#include "../../module/planner.h"
#include "../../module/stepper.h" // for various
#include "../../module/precise_homing.h"

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

#define DEBUG_OUT ENABLED(DEBUG_LEVELING_FEATURE)
#include "../../core/debug_out.h"

#if ENABLED(QUICK_HOME)

  static void quick_home_xy() {

    // Pretend the current position is 0,0
    CBI(axis_known_position, X_AXIS);
    CBI(axis_known_position, Y_AXIS);
    current_position.set(0.0, 0.0);
    sync_plan_position();

    const int x_axis_home_dir =
      #if ENABLED(DUAL_X_CARRIAGE)
        x_home_dir(active_extruder)
      #else
        home_dir(X_AXIS)
      #endif
    ;

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
        stepperX.stall_sensitivity(crash_s.home_sensitivity[0]);
        stepperY.stall_sensitivity(crash_s.home_sensitivity[1]);
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

  inline void home_z_safely() {

    // Disallow Z homing if X or Y are unknown
    if (!TEST(axis_known_position, X_AXIS) || !TEST(axis_known_position, Y_AXIS)) {
      LCD_MESSAGEPGM(MSG_ERR_Z_HOMING);
      SERIAL_ECHO_MSG(MSG_ERR_Z_HOMING_SER);
      return;
    }

    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("home_z_safely >>>");

    sync_plan_position();

    /**
     * Move the Z probe (or just the nozzle) to the safe homing point
     * (Z is already at the right height)
     */
    destination.set(safe_homing_xy, current_position.z);

    #if HOMING_Z_WITH_PROBE
      destination -= probe_offset;
    #endif

    if (position_is_reachable(destination)) {

      if (DEBUGGING(LEVELING)) DEBUG_POS("home_z_safely", destination);

      // Free the active extruder for movement
      TERN_(DUAL_X_CARRIAGE, idex_set_parked(false));

      TERN_(SENSORLESS_HOMING, safe_delay(500)); // Short delay needed to settle

      do_blocking_move_to_xy(destination);
      homeaxis(Z_AXIS);
    }
    else {
      LCD_MESSAGEPGM(MSG_ZPROBE_OUT);
      SERIAL_ECHO_MSG(MSG_ZPROBE_OUT_SER);
    }

    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("<<< home_z_safely");
  }

#endif // Z_SAFE_HOMING

#if ENABLED(IMPROVE_HOMING_RELIABILITY)

  motion_state_t begin_slow_homing() {
    motion_state_t motion_state{0};
    motion_state.acceleration.set(planner.settings.max_acceleration_mm_per_s2[X_AXIS],
                                 planner.settings.max_acceleration_mm_per_s2[Y_AXIS]
                                 OPTARG(DELTA, planner.settings.max_acceleration_mm_per_s2[Z_AXIS])
                               );
    planner.settings.max_acceleration_mm_per_s2[X_AXIS] = XY_HOMING_ACCELERATION;
    planner.settings.max_acceleration_mm_per_s2[Y_AXIS] = XY_HOMING_ACCELERATION;
    TERN_(DELTA, planner.settings.max_acceleration_mm_per_s2[Z_AXIS] = 100);
    #if HAS_CLASSIC_JERK
      motion_state.jerk_state = planner.max_jerk;
      planner.max_jerk.set(XY_HOMING_JERK, XY_HOMING_JERK OPTARG(DELTA, 0));
    #endif
    planner.refresh_acceleration_rates();
    return motion_state;
  }

  void end_slow_homing(const motion_state_t &motion_state) {
    planner.settings.max_acceleration_mm_per_s2[X_AXIS] = motion_state.acceleration.x;
    planner.settings.max_acceleration_mm_per_s2[Y_AXIS] = motion_state.acceleration.y;
    TERN_(DELTA, planner.settings.max_acceleration_mm_per_s2[Z_AXIS] = motion_state.acceleration.z);
    TERN_(HAS_CLASSIC_JERK, planner.max_jerk = motion_state.jerk_state);
    planner.refresh_acceleration_rates();
  }

#endif // IMPROVE_HOMING_RELIABILITY

/**
 * G28: Home all axes according to settings
 * 
 * If PRECISE_HOMING is enabled, there are specific amount
 * of tries to home an X/Y axis. If it fails it runs re-calibration
 * (if it's not disabled by D).
 *
 * Parameters
 *
 *  None  Home to all axes with no parameters.
 *        With QUICK_HOME enabled XY will home together, then Z.
 *
 *  O   Home only if position is unknown
 *
 *  Rn  Raise by n mm/inches before homing
 *
 * Cartesian/SCARA parameters
 *
 *  X   Home to the X endstop
 *  Y   Home to the Y endstop
 *  Z   Home to the Z endstop
 *
 * PRECISE_HOMING only:
 * 
 *  D   Avoid home calibration
 * 
 */
void GcodeSuite::G28(const bool always_home_all) {
  bool S = false;
  #if ENABLED(MARLIN_DEV_MODE)
    S = parser.seen('S')
  #endif

  bool O = parser.boolval('O');
  bool X = parser.seen('X');
  bool Y = parser.seen('Y');
  bool Z = parser.seen('Z');
  float R = parser.seenval('R') ? parser.value_linear_units() : Z_HOMING_HEIGHT;

  G28_no_parser(always_home_all, O, R, S, X, Y, Z);
}

void GcodeSuite::G28_no_parser(bool always_home_all, bool O, float R, bool S, bool X, bool Y,bool Z) {
  MINDA_BROKEN_CABLE_DETECTION__BEGIN();
  if (DEBUGGING(LEVELING)) {
    DEBUG_ECHOLNPGM(">>> G28");
    log_machine_info();
  }

  /**
   * Set the laser power to false to stop the planner from processing the current power setting.
   */
  #if ENABLED(LASER_FEATURE)
    planner.laser_inline.status.isPowered = false;
  #endif

  #if ENABLED(DUAL_X_CARRIAGE)
    bool IDEX_saved_duplication_state = extruder_duplication_enabled;
    DualXMode IDEX_saved_mode = dual_x_carriage_mode;
  #endif

  #if ENABLED(MARLIN_DEV_MODE)
    if (S) {
      LOOP_XYZ(a) set_axis_is_at_home((AxisEnum)a);
      sync_plan_position();
      SERIAL_ECHOLNPGM("Simulated Homing");
      report_current_position();
      if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("<<< G28");
      return;
    }
  #endif

  if (!homing_needed() && O) {
    if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPGM("> homing not needed, skip\n<<< G28");
    return;
  }

  #if ENABLED(FULL_REPORT_TO_HOST_FEATURE)
    const M_StateEnum old_grblstate = M_State_grbl;
    set_and_report_grblstate(M_HOMING);
  #endif

  TERN_(HAS_DWIN_E3V2_BASIC, DWIN_HomingStart());
  //TERN_(EXTENSIBLE_UI, ExtUI::onHomingStart());

  // Wait for planner moves to finish!
  planner.synchronize();

  // Disable the leveling matrix before homing
  #if HAS_LEVELING

    // Cancel the active G29 session
    #if ENABLED(PROBE_MANUALLY)
      g29_in_progress = false;
    #endif

    #if ENABLED(RESTORE_LEVELING_AFTER_G28)
      const bool leveling_was_active = planner.leveling_active;
    #endif
    set_bed_leveling_enabled(false);
  #endif

  // Reset to the XY plane
  TERN_(CNC_WORKSPACE_PLANES, workspace_plane = PLANE_XY);

  #define HAS_CURRENT_HOME(N) (defined(N##_CURRENT_HOME) && N##_CURRENT_HOME != N##_CURRENT)
  #if HAS_CURRENT_HOME(X) || HAS_CURRENT_HOME(X2) || HAS_CURRENT_HOME(Y) || HAS_CURRENT_HOME(Y2) || (ENABLED(DELTA) && HAS_CURRENT_HOME(Z)) || HAS_CURRENT_HOME(I) || HAS_CURRENT_HOME(J) || HAS_CURRENT_HOME(K) || HAS_CURRENT_HOME(U) || HAS_CURRENT_HOME(V) || HAS_CURRENT_HOME(W)
    #define HAS_HOMING_CURRENT 1
  #endif

  #if HAS_HOMING_CURRENT
    auto debug_current = [](FSTR_P const s, const int16_t a, const int16_t b) {
      DEBUG_ECHOF(s); DEBUG_ECHOLNPGM(" current: ", a, " -> ", b);
    };
    #if HAS_CURRENT_HOME(X)
      const int16_t tmc_save_current_X = stepperX.getMilliamps();
      stepperX.rms_current(X_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_X), tmc_save_current_X, X_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(X2)
      const int16_t tmc_save_current_X2 = stepperX2.getMilliamps();
      stepperX2.rms_current(X2_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_X2), tmc_save_current_X2, X2_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(Y)
      const int16_t tmc_save_current_Y = stepperY.getMilliamps();
      stepperY.rms_current(Y_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_Y), tmc_save_current_Y, Y_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(Y2)
      const int16_t tmc_save_current_Y2 = stepperY2.getMilliamps();
      stepperY2.rms_current(Y2_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_Y2), tmc_save_current_Y2, Y2_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(Z) && ENABLED(DELTA)
      const int16_t tmc_save_current_Z = stepperZ.getMilliamps();
      stepperZ.rms_current(Z_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_Z), tmc_save_current_Z, Z_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(I)
      const int16_t tmc_save_current_I = stepperI.getMilliamps();
      stepperI.rms_current(I_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_I), tmc_save_current_I, I_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(J)
      const int16_t tmc_save_current_J = stepperJ.getMilliamps();
      stepperJ.rms_current(J_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_J), tmc_save_current_J, J_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(K)
      const int16_t tmc_save_current_K = stepperK.getMilliamps();
      stepperK.rms_current(K_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_K), tmc_save_current_K, K_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(U)
      const int16_t tmc_save_current_U = stepperU.getMilliamps();
      stepperU.rms_current(U_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_U), tmc_save_current_U, U_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(V)
      const int16_t tmc_save_current_V = stepperV.getMilliamps();
      stepperV.rms_current(V_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_V), tmc_save_current_V, V_CURRENT_HOME);
    #endif
    #if HAS_CURRENT_HOME(W)
      const int16_t tmc_save_current_W = stepperW.getMilliamps();
      stepperW.rms_current(W_CURRENT_HOME);
      if (DEBUGGING(LEVELING)) debug_current(F(STR_W), tmc_save_current_W, W_CURRENT_HOME);
    #endif
    #if SENSORLESS_STALLGUARD_DELAY
      safe_delay(SENSORLESS_STALLGUARD_DELAY); // Short delay needed to settle
    #endif
  #endif

  #if ENABLED(IMPROVE_HOMING_RELIABILITY)
    motion_state_t saved_motion_state = begin_slow_homing();
  #endif

  // Always home with tool 0 active
  #if HAS_MULTI_HOTEND
    #if DISABLED(DELTA) || ENABLED(DELTA_HOME_TO_SAFE_ZONE)
      const uint8_t old_tool_index = active_extruder;
    #endif
    // PARKING_EXTRUDER homing requires different handling of movement / solenoid activation, depending on the side of homing
    #if ENABLED(PARKING_EXTRUDER)
      const bool pe_final_change_must_unpark = parking_extruder_unpark_after_homing(old_tool_index, X_HOME_DIR + 1 == old_tool_index * 2);
    #endif
    tool_change(0, true);
  #endif

  TERN_(HAS_DUPLICATION_MODE, set_duplication_enabled(false));

  remember_feedrate_scaling_off();

  endstops.enable(true); // Enable endstops for next homing move

  #if ENABLED(DELTA)

    constexpr bool doZ = true; // for NANODLP_Z_SYNC if your DLP is on a DELTA

    home_delta();

    TERN_(IMPROVE_HOMING_RELIABILITY, end_slow_homing(saved_motion_state));

  #elif ENABLED(AXEL_TPARA)

    constexpr bool doZ = true; // for NANODLP_Z_SYNC if your DLP is on a TPARA

    home_TPARA();

  #else

    const bool homeX = X, homeY = Y, homeZ = Z,
               home_all = always_home_all || (homeX == homeY && homeX == homeZ),
               doX = home_all || homeX, doY = home_all || homeY, doZ = home_all || homeZ;

    destination = current_position;

    #if Z_HOME_DIR > 0  // If homing away from BED do Z first

      if (doZ) homeaxis(Z_AXIS);

    #endif

    const float z_homing_height = (
      #if ENABLED(UNKNOWN_Z_NO_RAISE)
        !TEST(axis_known_position, Z_AXIS) ? 0 :
      #endif
          isnan(R) ? Z_HOMING_HEIGHT : R
    );

    if (z_homing_height && (doX || doY)) {
      // Raise Z before homing any other axes and z is not already high enough (never lower z)
      destination.z = z_homing_height;
      if (destination.z > current_position.z) {
        if (DEBUGGING(LEVELING)) DEBUG_ECHOLNPAIR("Raise Z (before homing) to ", destination.z);
        do_blocking_move_to_z(destination.z);
      }
    }

    MINDA_BROKEN_CABLE_DETECTION__PRE_XYHOME();

    #if ENABLED(QUICK_HOME)

      if (doX && doY) quick_home_xy();

    #endif

    // Home Y (before X)
    #if ENABLED(HOME_Y_BEFORE_X)

      if (doY
        #if ENABLED(CODEPENDENT_XY_HOMING)
          || doX
        #endif        
        ) homeaxis(Y_AXIS
          #if ENABLED(PRECISE_HOMING)
            , 0.0f, false, !parser.seen('D')
          #endif
        );

    #endif

    // Home X
    if (doX
      #if ENABLED(CODEPENDENT_XY_HOMING) && DISABLED(HOME_Y_BEFORE_X)
        || doY
      #endif
    ) {

      #if ENABLED(DUAL_X_CARRIAGE)

        // Always home the 2nd (right) extruder first
        active_extruder = 1;
        homeaxis(X_AXIS);

        // Remember this extruder's position for later tool change
        inactive_extruder_x = current_position.x;

        // Home the 1st (left) extruder
        active_extruder = 0;
        homeaxis(X_AXIS);

        // Consider the active extruder to be in its "parked" position
        idex_set_parked();

      #else

        homeaxis(X_AXIS
          #if ENABLED(PRECISE_HOMING)
            , 0.0f, false, !parser.seen('D')
          #endif
        );

      #endif
    }

    #if BOTH(FOAMCUTTER_XYUV, HAS_I_AXIS)
      // Home I (after X)
      if (doI) homeaxis(I_AXIS);
    #endif

    // Home Y (after X)
    #if DISABLED(HOME_Y_BEFORE_X)
      if (doY) 
        homeaxis(Y_AXIS
          #if ENABLED(PRECISE_HOMING)
            , 0.0f, false, !parser.seen('D')
          #endif
        );
    #endif

    #if BOTH(FOAMCUTTER_XYUV, HAS_J_AXIS)
      // Home J (after Y)
      if (doJ) homeaxis(J_AXIS);
    #endif

    TERN_(IMPROVE_HOMING_RELIABILITY, end_slow_homing(saved_motion_state));

    #if ENABLED(FOAMCUTTER_XYUV)
      // skip homing of unused Z axis for foamcutters
      if (doZ) set_axis_is_at_home(Z_AXIS);
    #else
      // Home Z last if homing towards the bed
      #if Z_HOME_DIR < 0
        if (doZ) {
          MINDA_BROKEN_CABLE_DETECTION__POST_XYHOME();
          #if ENABLED(BLTOUCH)
            bltouch.init();
          #endif
          #if ENABLED(Z_SAFE_HOMING)
            home_z_safely();
          #else
            homeaxis(Z_AXIS);
          #endif

          #if HOMING_Z_WITH_PROBE && defined(Z_AFTER_PROBING)
            move_z_after_probing();
          #endif
          MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_1();
        } // doZ
      #endif

      SECONDARY_AXIS_CODE(
        if (doI) homeaxis(I_AXIS),
        if (doJ) homeaxis(J_AXIS),
        if (doK) homeaxis(K_AXIS),
        if (doU) homeaxis(U_AXIS),
        if (doV) homeaxis(V_AXIS),
        if (doW) homeaxis(W_AXIS)
      );
    #endif

    sync_plan_position();

  #endif

  /**
   * Preserve DXC mode across a G28 for IDEX printers in DXC_DUPLICATION_MODE.
   * This is important because it lets a user use the LCD Panel to set an IDEX Duplication mode, and
   * then print a standard GCode file that contains a single print that does a G28 and has no other
   * IDEX specific commands in it.
   */
  #if ENABLED(DUAL_X_CARRIAGE)

    if (idex_is_duplicating()) {

      TERN_(IMPROVE_HOMING_RELIABILITY, saved_motion_state = begin_slow_homing());

      // Always home the 2nd (right) extruder first
      active_extruder = 1;
      homeaxis(X_AXIS);

      // Remember this extruder's position for later tool change
      inactive_extruder_x = current_position.x;

      // Home the 1st (left) extruder
      active_extruder = 0;
      homeaxis(X_AXIS);

      // Consider the active extruder to be parked
      idex_set_parked();

      dual_x_carriage_mode = IDEX_saved_mode;
      set_duplication_enabled(IDEX_saved_duplication_state);

      TERN_(IMPROVE_HOMING_RELIABILITY, end_slow_homing(saved_motion_state));
    }

  #endif // DUAL_X_CARRIAGE

  endstops.not_homing();

  // Clear endstop state for polled stallGuard endstops
  #if ENABLED(SPI_ENDSTOPS)
    endstops.clear_endstop_state();
  #endif

  // Move to a height where we can use the full xy-area
  TERN_(DELTA_HOME_TO_SAFE_ZONE, do_blocking_move_to_z(delta_clip_start_height));

  #if HAS_LEVELING && ENABLED(RESTORE_LEVELING_AFTER_G28)
    set_bed_leveling_enabled(leveling_was_active);
  #endif

  restore_feedrate_and_scaling();

  // Restore the active tool after homing
  #if HAS_MULTI_HOTEND && (DISABLED(DELTA) || ENABLED(DELTA_HOME_TO_SAFE_ZONE))
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
    #if HAS_CURRENT_HOME(Z) && ENABLED(DELTA)
      stepperZ.rms_current(tmc_save_current_Z);
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

    MINDA_BROKEN_CABLE_DETECTION__END();
}
