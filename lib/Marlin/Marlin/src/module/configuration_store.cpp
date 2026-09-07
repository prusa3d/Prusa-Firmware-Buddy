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
 * configuration_store.cpp
 *
 * Settings and EEPROM storage
 *
 * IMPORTANT:  Whenever there are changes made to the variables stored in EEPROM
 * in the functions below, also increment the version number. This makes sure that
 * the default values are used whenever there is a change to the data, to prevent
 * wrong data being written to the variables.
 *
 * ALSO: Variables in the Store and Retrieve sections must be in the same order.
 *       If a feature is disabled, some data must still be written that, when read,
 *       either sets a Sane Default, or results in No Change to the existing value.
 *
 */

// Check the integrity of data offsets.
// Can be disabled for production build.
//#define DEBUG_EEPROM_READWRITE

#include "configuration_store.h"

#include <option/has_pause.h>
#include <option/has_planner.h>
#if HAS_PLANNER()
  #include "endstops.h"
  #include "planner.h"
  #include "stepper.h"
#endif

#include "temperature.h"
#include "../lcd/ultralcd.h"
#include "../core/language.h"
#include "../gcode/gcode.h"
#include "../Marlin.h"
#include <feature/motordriver_util.h>

#if ENABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    #include "config_store/store_c_api.h"
#endif // USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES

#include "probe.h"

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(EXTENSIBLE_UI)
  #include "../lcd/extensible_ui/ui_api.h"
#endif

#include "../feature/pause.h"

#if ENABLED(BACKLASH_COMPENSATION)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../feature/backlash.h"
#endif

#if EXTRUDERS > 1
  #include "tool_change.h"
  void M217_report();
#endif

#if HAS_TRINAMIC
  #include "stepper/indirection.h"
#endif

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
  #include <option/has_burst_stepping.h>
  void M970_report(bool eeprom);
#endif

// Limit an index to an array size
#define ALIM(I,ARR) _MIN(I, COUNT(ARR) - 1)

// Defaults for reset / fill in on load
static const uint32_t   _DMA[] PROGMEM = DEFAULT_MAX_ACCELERATION;
#if ENABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
// Only called by reset_motion() (compiled only when HAS_PLANNER, i.e. master boards - all have the config store)
// Storeless boards (puppies) are planner-less, so the caller is compiled out there.
static float get_steps_per_unit(size_t index) {
    switch (index) {
    case 0:
      return get_steps_per_unit_x();
    case 1:
      return get_steps_per_unit_y();
    case 2:
      return get_steps_per_unit_z();
    }
    //if index is bigger than max index, use max index - default marlin behavior
    return get_steps_per_unit_e();
}
#endif // USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
static const feedRate_t _DMF[] PROGMEM = DEFAULT_MAX_FEEDRATE;

MarlinSettings settings;

/**
 * Post-process after Retrieve or Reset
 */

#if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
  float new_z_fade_height;
#endif

void MarlinSettings::postprocess() {
  #if HAS_PLANNER()
    xyze_pos_t oldpos = current_position;

    // steps per s2 needs to be updated to agree with units per s2
    planner.refresh_acceleration_rates();

    #if DISABLED(NO_VOLUMETRICS)
      planner.calculate_volumetric_multipliers();
    #elif EXTRUDERS
      // #error dead code found by automatic analyses (see BFW-5461)
      for (auto tool : VirtualToolIndex::all())
        planner.refresh_e_factor(tool);
    #endif

    // Software endstops depend on home_offset
    LOOP_XYZ(i) {
      update_workspace_offset((AxisEnum)i);
      update_software_endstops((AxisEnum)i);
    }

    #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
      set_z_fade_height(new_z_fade_height, false); // false = no report
    #endif

    #if HAS_LINEAR_E_JERK
      // #error dead code found by automatic analyses (see BFW-5461)
      planner.recalculate_max_e_jerk();
    #endif

    // Refresh mm_per_step, mm_per_half_step and mm_per_mstep with the reciprocal of axis_steps_per_mm and axis_msteps_per_mm
    // and init stepper.count[], planner.position[] with current_position
    planner.refresh_positioning();

    // Various factors can change the current position
    if (oldpos != current_position)
      report_current_position();
  #endif /* HAS_PLANNER() */
}

bool MarlinSettings::save() {
  // EEPROM disabled
  return false;
}

/**
 * @brief Resets motion parameters only to defaults (speed, accel., etc.)
 * @param no_limits When true, do not apply any mode limit
 */
void MarlinSettings::reset_motion(const bool no_limits) {
  #if HAS_PLANNER()
    auto s = planner.user_settings;

    LOOP_XYZE_N(i) {
      s.max_acceleration_mm_per_s2[i] = pgm_read_dword(&_DMA[ALIM(i, _DMA)]);
      s.axis_steps_per_mm[i]          = get_steps_per_unit(i);
      s.axis_msteps_per_mm[i]         = get_steps_per_unit(i) * PLANNER_STEPS_MULTIPLIER;
      s.max_feedrate_mm_s[i]          = pgm_read_float(&_DMF[ALIM(i, _DMF)]);
    }

    s.min_segment_time_us = DEFAULT_MINSEGMENTTIME;
    s.acceleration = DEFAULT_ACCELERATION;
    s.retract_acceleration = DEFAULT_RETRACT_ACCELERATION;
    s.travel_acceleration = DEFAULT_TRAVEL_ACCELERATION;
    s.min_feedrate_mm_s = feedRate_t(DEFAULT_MINIMUMFEEDRATE);
    s.min_travel_feedrate_mm_s = feedRate_t(DEFAULT_MINTRAVELFEEDRATE);

    #if HAS_CLASSIC_JERK
      #ifndef DEFAULT_XJERK
        // #error dead code found by automatic analyses (see BFW-5461)
        #define DEFAULT_XJERK 0
      #endif
      #ifndef DEFAULT_YJERK
        // #error dead code found by automatic analyses (see BFW-5461)
        #define DEFAULT_YJERK 0
      #endif
      #ifndef DEFAULT_ZJERK
        // #error dead code found by automatic analyses (see BFW-5461)
        #define DEFAULT_ZJERK 0
      #endif
      s.max_jerk.set(DEFAULT_XJERK, DEFAULT_YJERK, DEFAULT_ZJERK);
      #if HAS_CLASSIC_E_JERK
        s.max_jerk.e = DEFAULT_EJERK;
      #endif
    #endif

    #if DISABLED(CLASSIC_JERK)
      planner.junction_deviation_mm = float(JUNCTION_DEVIATION_MM);
    #endif

    planner.apply_settings(s, no_limits);
  #endif /* HAS_PLANNER() */
}

/**
 * M502 - Reset Configuration
 */
void MarlinSettings::reset() {
  #if HAS_PLANNER()
    reset_motion();
  #endif

  #if HAS_HOME_OFFSET
    home_offset = {};
  #endif

  #if HAS_HOTEND_OFFSET
    reset_hotend_offsets();
  #endif

  //
  // Tool-change Settings
  //

  #if EXTRUDERS > 1
    toolchange_settings.z_raise = TOOLCHANGE_ZRAISE;
  #endif

  #if ENABLED(BACKLASH_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    backlash.correction = (BACKLASH_CORRECTION) * 255;
    constexpr xyz_float_t tmp = BACKLASH_DISTANCE_MM;
    backlash.distance_mm = tmp;
    #ifdef BACKLASH_SMOOTHING_MM
      // #error dead code found by automatic analyses (see BFW-5461)
      backlash.smoothing_mm = BACKLASH_SMOOTHING_MM;
    #endif
  #endif

  //
  // Global Leveling
  //

  #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
    new_z_fade_height = 0.0;
  #endif

  #if HAS_LEVELING
    reset_bed_level();
  #endif

  #if HAS_BED_PROBE
    #ifndef NOZZLE_TO_PROBE_OFFSET
      // #error dead code found by automatic analyses (see BFW-5461)
      #define NOZZLE_TO_PROBE_OFFSET { 0, 0, 0 }
    #endif
    constexpr float dpo[XYZ] = NOZZLE_TO_PROBE_OFFSET;
    static_assert(COUNT(dpo) == 3, "NOZZLE_TO_PROBE_OFFSET must contain offsets for X, Y, and Z.");
    LOOP_XYZ(a) probe_offset[a] = dpo[a];
  #endif

  //
  // Endstop Adjustments
  //

    #if ENABLED(Z_TRIPLE_ENDSTOPS)
      // #error dead code found by automatic analyses (see BFW-5461)
      endstops.z2_endstop_adj = (
        #ifdef Z_TRIPLE_ENDSTOPS_ADJUSTMENT2
          // #error dead code found by automatic analyses (see BFW-5461)
          Z_TRIPLE_ENDSTOPS_ADJUSTMENT2
        #else
          // #error dead code found by automatic analyses (see BFW-5461)
          0
        #endif
      );
      endstops.z3_endstop_adj = (
        #ifdef Z_TRIPLE_ENDSTOPS_ADJUSTMENT3
          // #error dead code found by automatic analyses (see BFW-5461)
          Z_TRIPLE_ENDSTOPS_ADJUSTMENT3
        #else
          // #error dead code found by automatic analyses (see BFW-5461)
          0
        #endif
      );
    #endif

  //
  // Hotend PID
  //

  #if ENABLED(PIDTEMP)
    for (auto tool : PhysicalToolIndex::all()) {
      Hotend::for_tool(tool).set_nozzle_pid_config(HotendPIDConfig{});
    }
  #endif

  //
  // PID Extrusion Scaling
  //

  //
  // Heated Bed PID
  //

  #if ENABLED(PIDTEMPBED)
    thermalManager.temp_bed.pid.Kp = DEFAULT_bedKp;
    thermalManager.temp_bed.pid.Ki = scalePID_i(DEFAULT_bedKi);
    thermalManager.temp_bed.pid.Kd = scalePID_d(DEFAULT_bedKd);
  #endif

  //
  // Volumetric & Filament Size
  //

  #if DISABLED(NO_VOLUMETRICS)

    parser.volumetric_enabled =
      #if ENABLED(VOLUMETRIC_DEFAULT_ON)
        // #error dead code found by automatic analyses (see BFW-5461)
        true
      #else
        false
      #endif
    ;
    for (auto tool : VirtualToolIndex::all())
      planner.filament_size[tool] = DEFAULT_NOMINAL_FILAMENT_DIA;

  #endif

  #if HAS_PLANNER()
    endstops.enable_globally(
      #if ENABLED(ENDSTOPS_ALWAYS_ON_DEFAULT)
        // #error dead code found by automatic analyses (see BFW-5461)
        true
      #else
        false
      #endif
    );
  #endif /* HAS_PLANNER() */

  reset_stepper_drivers();
  
  //
  // Advanced Pause filament load & unload lengths
  //

  #if HAS_PAUSE()
    for (uint8_t e = 0; e < EXTRUDERS; e++) {
      fc_settings[e].unload_length = FILAMENT_CHANGE_UNLOAD_LENGTH;
      fc_settings[e].load_length = FILAMENT_CHANGE_FAST_LOAD_LENGTH;
    }
  #endif

  postprocess();
}

#if DISABLED(DISABLE_M503)

  #define CONFIG_ECHO_START()       do{ if (!forReplay) SERIAL_ECHO_START(); }while(0)
  #define CONFIG_ECHO_MSG(STR)      do{ CONFIG_ECHO_START(); SERIAL_ECHOLNPGM(STR); }while(0)
  #define CONFIG_ECHO_HEADING(STR)  do{ if (!forReplay) { CONFIG_ECHO_START(); SERIAL_ECHOLNPGM(STR); } }while(0)

  #if HAS_TRINAMIC
    inline void say_M906(const bool forReplay) { CONFIG_ECHO_START(); SERIAL_ECHOPGM("  M906"); }
    #if HAS_STEALTHCHOP
      void say_M569(const bool forReplay, const char * const etc=nullptr, const bool newLine = false) {
        CONFIG_ECHO_START();
        SERIAL_ECHOPGM("  M569 S1");
        if (etc) {
          SERIAL_CHAR(' ');
          serialprintPGM(etc);
        }
        if (newLine) SERIAL_EOL();
      }
    #endif
    #if ENABLED(HYBRID_THRESHOLD)
      inline void say_M913(const bool forReplay) { CONFIG_ECHO_START(); SERIAL_ECHOPGM("  M913"); }
    #endif
    #if USE_SENSORLESS
      inline void say_M914() { SERIAL_ECHOPGM("  M914"); }
    #endif
  #endif

  #if HAS_PAUSE()
    inline void say_M603(const bool forReplay) { CONFIG_ECHO_START(); SERIAL_ECHOPGM("  M603 "); }
  #endif

  inline void say_units(const bool colon) {
    serialprintPGM(
      PSTR(" (mm)")
    );
    if (colon) SERIAL_ECHOLNPGM(":");
  }

  void report_M92(const bool echo=true, const int8_t e=-1);

  /**
   * M503 - Report current settings in RAM
   *
   * Unless specifically disabled, M503 is available even without EEPROM
   */
  void MarlinSettings::report(const bool forReplay) {
    /**
     * Announce current units, in case inches are being displayed
     */
    CONFIG_ECHO_START();
      SERIAL_ECHOPGM("  G21    ; Units in mm");
      say_units(false);
    SERIAL_EOL();

    #if DISABLED(NO_VOLUMETRICS)

      /**
       * Volumetric extrusion M200
       */
      if (!forReplay) {
        CONFIG_ECHO_START();
        SERIAL_ECHOPGM("Filament settings:");
        if (parser.volumetric_enabled)
          SERIAL_EOL();
        else
          SERIAL_ECHOLNPGM(" Disabled");
      }

      for(auto tool : VirtualToolIndex::all()) {
        CONFIG_ECHO_START();
        SERIAL_ECHOLNPAIR("  M200 T", tool.to_raw(), " D", LINEAR_UNIT(planner.filament_size[tool]));
      }

      if (!parser.volumetric_enabled)
        CONFIG_ECHO_MSG("  M200 D0");

    #endif // !NO_VOLUMETRICS

    #if HAS_PLANNER()
      CONFIG_ECHO_HEADING("Steps per unit:");
      report_M92(!forReplay);

      CONFIG_ECHO_HEADING("Maximum feedrates (units/s):");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
          "  M203 X", LINEAR_UNIT(planner.settings.max_feedrate_mm_s[X_AXIS])
        , " Y", LINEAR_UNIT(planner.settings.max_feedrate_mm_s[Y_AXIS])
        , " Z", LINEAR_UNIT(planner.settings.max_feedrate_mm_s[Z_AXIS])
        #if DISABLED(DISTINCT_E_FACTORS)
          , " E", VOLUMETRIC_UNIT(planner.settings.max_feedrate_mm_s[E_AXIS])
        #endif
      );
      #if ENABLED(DISTINCT_E_FACTORS)
        // #error dead code found by automatic analyses (see BFW-5461)
        CONFIG_ECHO_START();
        for (uint8_t i = 0; i < E_STEPPERS; i++) {
          SERIAL_ECHOLNPAIR(
              "  M203 T", (int)i
            , " E", VOLUMETRIC_UNIT(planner.settings.max_feedrate_mm_s[E_AXIS_N(i)])
          );
        }
      #endif

      CONFIG_ECHO_HEADING("Maximum Acceleration (units/s2):");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
          "  M201 X", LINEAR_UNIT(planner.settings.max_acceleration_mm_per_s2[X_AXIS])
        , " Y", LINEAR_UNIT(planner.settings.max_acceleration_mm_per_s2[Y_AXIS])
        , " Z", LINEAR_UNIT(planner.settings.max_acceleration_mm_per_s2[Z_AXIS])
        #if DISABLED(DISTINCT_E_FACTORS)
          , " E", VOLUMETRIC_UNIT(planner.settings.max_acceleration_mm_per_s2[E_AXIS])
        #endif
      );
      #if ENABLED(DISTINCT_E_FACTORS)
        // #error dead code found by automatic analyses (see BFW-5461)
        CONFIG_ECHO_START();
        for (uint8_t i = 0; i < E_STEPPERS; i++)
          SERIAL_ECHOLNPAIR(
              "  M201 T", (int)i
            , " E", VOLUMETRIC_UNIT(planner.settings.max_acceleration_mm_per_s2[E_AXIS_N(i)])
          );
      #endif

      CONFIG_ECHO_HEADING("Acceleration (units/s2): P<print_accel> R<retract_accel> T<travel_accel>");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
          "  M204 P", LINEAR_UNIT(planner.settings.acceleration)
        , " R", LINEAR_UNIT(planner.settings.retract_acceleration)
        , " T", LINEAR_UNIT(planner.settings.travel_acceleration)
      );

      if (!forReplay) {
        CONFIG_ECHO_START();
        SERIAL_ECHOPGM("Advanced: B<min_segment_time_us> S<min_feedrate> T<min_travel_feedrate>");
        #if DISABLED(CLASSIC_JERK)
          SERIAL_ECHOPGM(" J<junc_dev>");
        #endif
        #if HAS_CLASSIC_JERK
          SERIAL_ECHOPGM(" X<max_x_jerk> Y<max_y_jerk> Z<max_z_jerk>");
          #if HAS_CLASSIC_E_JERK
            SERIAL_ECHOPGM(" E<max_e_jerk>");
          #endif
        #endif
        SERIAL_EOL();
      }
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
          "  M205 B", LINEAR_UNIT(planner.settings.min_segment_time_us)
        , " S", LINEAR_UNIT(planner.settings.min_feedrate_mm_s)
        , " T", LINEAR_UNIT(planner.settings.min_travel_feedrate_mm_s)
        #if DISABLED(CLASSIC_JERK)
          , " J", LINEAR_UNIT(planner.junction_deviation_mm)
        #endif
        #if HAS_CLASSIC_JERK
          , " X", LINEAR_UNIT(planner.settings.max_jerk.x)
          , " Y", LINEAR_UNIT(planner.settings.max_jerk.y)
          , " Z", LINEAR_UNIT(planner.settings.max_jerk.z)
          #if HAS_CLASSIC_E_JERK
            , " E", LINEAR_UNIT(planner.settings.max_jerk.e)
          #endif
        #endif
      );
    #endif /* HAS_PLANNER() */

    #if HAS_M206_COMMAND
      CONFIG_ECHO_HEADING("Home offset:");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR("  M206"
        " X", LINEAR_UNIT(home_offset.x),
        " Y", LINEAR_UNIT(home_offset.y),
        " Z", LINEAR_UNIT(home_offset.z)
      );
    #endif

    #if HAS_HOTEND_OFFSET
      CONFIG_ECHO_HEADING("Hotend offsets:");
      CONFIG_ECHO_START();
      for (auto tool : PhysicalToolIndex::all()) {
        SERIAL_ECHOPAIR(
          "  M218 T", static_cast<int>(tool.to_raw()),
          " X", LINEAR_UNIT(hotend_offset[tool].x), " Y", LINEAR_UNIT(hotend_offset[tool].y)
        );
        SERIAL_ECHOLNPAIR_F(" Z", LINEAR_UNIT(hotend_offset[tool].z), 3);
      }
    #endif

    /**
     * Bed Leveling
     */
    #if HAS_LEVELING

      #if ENABLED(AUTO_BED_LEVELING_UBL)

        if (!forReplay) {
          CONFIG_ECHO_START();
          ubl.echo_name();
          SERIAL_ECHOLNPGM(":");
        }

      #endif

      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
        "  M420 S", planner.leveling_active ? 1 : 0
        #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
          , " Z", LINEAR_UNIT(planner.z_fade_height)
        #endif
      );

      #if ENABLED(AUTO_BED_LEVELING_UBL)

        if (!forReplay) {
          SERIAL_EOL();
          ubl.report_state();
        }

       //ubl.report_current_mesh();   // This is too verbose for large meshes. A better (more terse)
                                                  // solution needs to be found.
      #endif

    #endif // HAS_LEVELING

    #if ENABLED(Z_TRIPLE_ENDSTOPS)
      // #error dead code found by automatic analyses (see BFW-5461)

      CONFIG_ECHO_HEADING("Endstop adjustment:");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR("  M666 S1 Z", LINEAR_UNIT(endstops.z2_endstop_adj));
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR("  M666 S2 Z", LINEAR_UNIT(endstops.z3_endstop_adj));

    #endif

    #if HAS_PID_HEATING

      CONFIG_ECHO_HEADING("PID settings:");

      #if ENABLED(PIDTEMP)
        for (auto tool : PhysicalToolIndex::all()) {
          const auto &pid = Hotend::for_tool(tool).nozzle_pid_config();

          CONFIG_ECHO_START();
          SERIAL_ECHOPAIR("  M301"
            #if HOTENDS > 1
              " E", tool.to_raw(),
            #endif
              " P", pid.Kp
            , " I", unscalePID_i(pid.Ki)
            , " D", unscalePID_d(pid.Kd)
          );
          #if ENABLED(PID_EXTRUSION_SCALING)
            SERIAL_ECHOPAIR(" C", pid.Kc);
          #endif
          SERIAL_EOL();
        }
      #endif // PIDTEMP

      #if ENABLED(PIDTEMPBED)
        CONFIG_ECHO_START();
        SERIAL_ECHOLNPAIR(
            "  M304 P", thermalManager.temp_bed.pid.Kp
          , " I", unscalePID_i(thermalManager.temp_bed.pid.Ki)
          , " D", unscalePID_d(thermalManager.temp_bed.pid.Kd)
        );
      #endif

    #endif // PIDTEMP || PIDTEMPBED

    /**
     * Probe Offset
     */
    #if HAS_BED_PROBE
      if (!forReplay) {
        CONFIG_ECHO_START();
        SERIAL_ECHOPGM("Z-Probe Offset");
        say_units(true);
      }
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR("  M851 X", LINEAR_UNIT(probe_offset.x),
                              " Y", LINEAR_UNIT(probe_offset.y),
                              " Z", LINEAR_UNIT(probe_offset.z));
    #endif

    #if HAS_TRINAMIC

      /**
       * TMC stepper driver current
       */
      CONFIG_ECHO_HEADING("Stepper driver current:");

      #if AXIS_IS_TMC(X) || AXIS_IS_TMC(Y) || AXIS_IS_TMC(Z)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(
          #if AXIS_IS_TMC(X)
            " X", stepperX.getMilliamps(),
          #endif
          #if AXIS_IS_TMC(Y)
            " Y", stepperY.getMilliamps(),
          #endif
          #if AXIS_IS_TMC(Z)
            " Z", stepperZ.getMilliamps()
          #endif
        );
      #endif

      #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOPGM(" I1");
        SERIAL_ECHOLNPAIR(
          #if AXIS_IS_TMC(Z2)
            // #error dead code found by automatic analyses (see BFW-5461)
            " Z", stepperZ2.getMilliamps()
          #endif
        );
      #endif

      #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" I2 Z", stepperZ3.getMilliamps());
      #endif

      #if AXIS_IS_TMC(E0)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T0 E", stepperE0.getMilliamps());
      #endif
      #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T1 E", stepperE1.getMilliamps());
      #endif
      #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T2 E", stepperE2.getMilliamps());
      #endif
      #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T3 E", stepperE3.getMilliamps());
      #endif
      #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T4 E", stepperE4.getMilliamps());
      #endif
      #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        say_M906(forReplay);
        SERIAL_ECHOLNPAIR(" T5 E", stepperE5.getMilliamps());
      #endif
      SERIAL_EOL();

      /**
       * TMC Hybrid Threshold
       */
      #if ENABLED(HYBRID_THRESHOLD)
        CONFIG_ECHO_HEADING("Hybrid Threshold:");
        #if AXIS_HAS_STEALTHCHOP(X) || AXIS_HAS_STEALTHCHOP(Y) || AXIS_HAS_STEALTHCHOP(Z)
          say_M913(forReplay);
        #endif
        #if AXIS_HAS_STEALTHCHOP(X)
          SERIAL_ECHOPAIR(" X", stepperX.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(Y)
          SERIAL_ECHOPAIR(" Y", stepperY.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(Z)
          SERIAL_ECHOPAIR(" Z", stepperZ.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(X) || AXIS_HAS_STEALTHCHOP(Y) || AXIS_HAS_STEALTHCHOP(Z)
          SERIAL_EOL();
        #endif

        #if AXIS_HAS_STEALTHCHOP(Z2)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" I1 Z", stepperZ2.get_pwm_thrs());
        #endif

        #if AXIS_HAS_STEALTHCHOP(Z3)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" I2 Z", stepperZ3.get_pwm_thrs());
        #endif

        #if AXIS_HAS_STEALTHCHOP(E0)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T0 E", stepperE0.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(E1)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T1 E", stepperE1.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(E2)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T2 E", stepperE2.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(E3)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T3 E", stepperE3.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(E4)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T4 E", stepperE4.get_pwm_thrs());
        #endif
        #if AXIS_HAS_STEALTHCHOP(E5)
          // #error dead code found by automatic analyses (see BFW-5461)
          say_M913(forReplay);
          SERIAL_ECHOLNPAIR(" T5 E", stepperE5.get_pwm_thrs());
        #endif
        SERIAL_EOL();
      #endif // HYBRID_THRESHOLD

      /**
       * TMC Sensorless homing thresholds
       */
      #if USE_SENSORLESS
        CONFIG_ECHO_HEADING("StallGuard threshold:");
        #if X_SENSORLESS || Y_SENSORLESS || Z_SENSORLESS
          CONFIG_ECHO_START();
          say_M914();
          #if X_SENSORLESS
            SERIAL_ECHOPAIR(" X", stepperX.stall_sensitivity());
          #endif
          #if Y_SENSORLESS
            SERIAL_ECHOPAIR(" Y", stepperY.stall_sensitivity());
          #endif
          #if Z_SENSORLESS
            SERIAL_ECHOPAIR(" Z", stepperZ.stall_sensitivity());
          #endif
          SERIAL_EOL();
        #endif

        #if Z2_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          CONFIG_ECHO_START();
          say_M914();
          SERIAL_ECHOLNPAIR(" I1 Z", stepperZ2.stall_sensitivity());
        #endif

        #if Z3_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          CONFIG_ECHO_START();
          say_M914();
          SERIAL_ECHOLNPAIR(" I2 Z", stepperZ3.stall_sensitivity());
        #endif

      #endif // USE_SENSORLESS

      /**
       * TMC stepping mode
       */
      #if HAS_STEALTHCHOP
        CONFIG_ECHO_HEADING("Driver stepping mode:");
        #if AXIS_HAS_STEALTHCHOP(X)
          const bool chop_x = stepperX.get_stealthChop_status();
        #else
          constexpr bool chop_x = false;
        #endif
        #if AXIS_HAS_STEALTHCHOP(Y)
          const bool chop_y = stepperY.get_stealthChop_status();
        #else
          constexpr bool chop_y = false;
        #endif
        #if AXIS_HAS_STEALTHCHOP(Z)
          const bool chop_z = stepperZ.get_stealthChop_status();
        #else
          constexpr bool chop_z = false;
        #endif

        if (chop_x || chop_y || chop_z) {
          say_M569(forReplay);
          if (chop_x) SERIAL_ECHOPGM(" X");
          if (chop_y) SERIAL_ECHOPGM(" Y");
          if (chop_z) SERIAL_ECHOPGM(" Z");
          SERIAL_EOL();
        }

        #if AXIS_HAS_STEALTHCHOP(Z2)
          // #error dead code found by automatic analyses (see BFW-5461)
          const bool chop_z2 = stepperZ2.get_stealthChop_status();
        #else
          constexpr bool chop_z2 = false;
        #endif

        if (chop_z2) {
          say_M569(forReplay, PSTR("I1"));
          if (chop_z2) SERIAL_ECHOPGM(" Z");
          SERIAL_EOL();
        }

        #if AXIS_HAS_STEALTHCHOP(Z3)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperZ3.get_stealthChop_status()) { say_M569(forReplay, PSTR("I2 Z"), true); }
        #endif

        #if AXIS_HAS_STEALTHCHOP(E0)
          if (stepperE0.get_stealthChop_status()) { say_M569(forReplay, PSTR("T0 E"), true); }
        #endif
        #if AXIS_HAS_STEALTHCHOP(E1)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperE1.get_stealthChop_status()) { say_M569(forReplay, PSTR("T1 E"), true); }
        #endif
        #if AXIS_HAS_STEALTHCHOP(E2)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperE2.get_stealthChop_status()) { say_M569(forReplay, PSTR("T2 E"), true); }
        #endif
        #if AXIS_HAS_STEALTHCHOP(E3)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperE3.get_stealthChop_status()) { say_M569(forReplay, PSTR("T3 E"), true); }
        #endif
        #if AXIS_HAS_STEALTHCHOP(E4)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperE4.get_stealthChop_status()) { say_M569(forReplay, PSTR("T4 E"), true); }
        #endif
        #if AXIS_HAS_STEALTHCHOP(E5)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (stepperE5.get_stealthChop_status()) { say_M569(forReplay, PSTR("T5 E"), true); }
        #endif

      #endif // HAS_STEALTHCHOP

    #endif // HAS_TRINAMIC

    /**
     * Advanced Pause filament load & unload lengths
     */
    #if HAS_PAUSE()
      CONFIG_ECHO_HEADING("Filament load/unload lengths:");
      for(size_t i = 0; i < std::size(fc_settings); i++) {
        const auto &s = fc_settings[i];
        say_M603(forReplay);
        SERIAL_ECHOLNPAIR("T", i, " L", LINEAR_UNIT(s.load_length), " U", LINEAR_UNIT(s.unload_length)); 
      }
    #endif

    #if EXTRUDERS > 1
      CONFIG_ECHO_HEADING("Tool-changing:");
      CONFIG_ECHO_START();
      M217_report();
    #endif

    #if ENABLED(BACKLASH_GCODE)
      // #error dead code found by automatic analyses (see BFW-5461)
      CONFIG_ECHO_HEADING("Backlash compensation:");
      CONFIG_ECHO_START();
      SERIAL_ECHOLNPAIR(
        "  M425 F", backlash.get_correction(),
        " X", LINEAR_UNIT(backlash.distance_mm.x),
        " Y", LINEAR_UNIT(backlash.distance_mm.y),
        " Z", LINEAR_UNIT(backlash.distance_mm.z)
        #ifdef BACKLASH_SMOOTHING_MM
          // #error dead code found by automatic analyses (see BFW-5461)
          , " S", LINEAR_UNIT(backlash.smoothing_mm)
        #endif
      );
    #endif

    #if HAS_PHASE_STEPPING()
      #if HAS_BURST_STEPPING()
        // #error dead code found by automatic analyses (see BFW-5461)
        CONFIG_ECHO_HEADING("Phase stepping (burst):");
      #else
        CONFIG_ECHO_HEADING("Phase stepping:");
      #endif
      CONFIG_ECHO_START();
      SERIAL_ECHO("  ");
      M970_report(true);
    #endif
  }

#endif // !DISABLE_M503
