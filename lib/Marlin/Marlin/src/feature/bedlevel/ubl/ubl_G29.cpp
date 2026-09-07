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

#include "../../../inc/MarlinConfig.h"
#include "config_store/store_instance.hpp"
#include <option/has_crash_detection.h>
#include <option/has_power_panic.h>

#if ENABLED(AUTO_BED_LEVELING_UBL)

  #include "../bedlevel.h"

  #include "../../../Marlin.h"
  #include "../../../libs/hex_print_routines.h"
  #include "../../../module/configuration_store.h"
  #include "../../../lcd/ultralcd.h"
  #include "../../../module/stepper.h"
  #include "../../../module/planner.h"
  #include "../../../module/motion.h"
  #include "../../../module/probe.h"
  #include "../../../gcode/gcode.h"
  #include "../../../libs/least_squares_fit.h"
  #include "../../../feature/print_area.h"
  #include <fanctl.hpp>
  #include <option/has_love_board.h>
  #include <raii/scope_guard.hpp>

  #if BOARD_IS_XBUDDY()
    #include <hw_configuration.hpp>
  #endif

  #include <feature/print_status_message/print_status_message_guard.hpp>
  #include <feature/pressure_advance/pressure_advance_config.hpp>
  #include <i18n.h>

  #if ENABLED(NOZZLE_LOAD_CELL)
    #include "loadcell.hpp"
  #endif

  #include <option/has_nozzle_cleaner_lite.h>
  #if HAS_NOZZLE_CLEANER_LITE()
    #include <feature/nozzle_cleaner_lite/include/nozzle_cleaner_lite.hpp>
  #endif

  #if ENABLED(EXTENSIBLE_UI)
    #include "../../../lcd/extensible_ui/ui_api.h"
  #endif

  #if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
  #endif
  #if HAS_POWER_PANIC()
    #include "power_panic.hpp"
  #endif

  #include <math.h>
  #include <algorithm>
  #include <logging/log.hpp>

  LOG_COMPONENT_REF(Marlin);

  #define UBL_G29_P31

  int    unified_bed_leveling::g29_verbose_level,
         unified_bed_leveling::g29_phase_value,
         unified_bed_leveling::g29_repetition_cnt,
         unified_bed_leveling::g29_storage_slot = 0,
         unified_bed_leveling::g29_map_type;
  bool   unified_bed_leveling::g29_c_flag;
  bool   unified_bed_leveling::g29_wait_for_preheat;
  float  unified_bed_leveling::g29_card_thickness = 0,
         unified_bed_leveling::g29_constant = 0;
  xy_bool_t unified_bed_leveling::xy_seen;
  xy_pos_t unified_bed_leveling::g29_pos;
  xy_float_t unified_bed_leveling::g29_size;
  bool unified_bed_leveling::g29_size_seen;

  std::optional<unified_bed_leveling::MeasuredZ> unified_bed_leveling::g29_min_max_measured_z;

  #if HAS_BED_PROBE
    int  unified_bed_leveling::g29_grid_size;
  #endif

  /**
   *   G29: Unified Bed Leveling by Roxy
   *
   *   Parameters understood by this leveling system:
   *
   *   A     Activate   Activate the Unified Bed Leveling system.
   *
   *   B #   Business   Use the 'Business Card' mode of the Manual Probe subsystem with P2.
   *                    Note: A non-compressible Spark Gap feeler gauge is recommended over a business card.
   *                    In this mode of G29 P2, a business or index card is used as a shim that the nozzle can
   *                    grab onto as it is lowered. In principle, the nozzle-bed distance is the same when the
   *                    same resistance is felt in the shim. You can omit the numerical value on first invocation
   *                    of G29 P2 B to measure shim thickness. Subsequent use of 'B' will apply the previously-
   *                    measured thickness by default.
   *
   *   C     Continue   G29 P1 C continues the generation of a partially-constructed Mesh without invalidating
   *                    previous measurements.
   *
   *   C                G29 P2 C tells the Manual Probe subsystem to not use the current nozzle
   *                    location in its search for the closest unmeasured Mesh Point. Instead, attempt to
   *                    start at one end of the uprobed points and Continue sequentially.
   *
   *                    G29 P3 C specifies the Constant for the fill. Otherwise, uses a "reasonable" value.
   *
   *   C     Current    G29 Z C uses the Current location (instead of bed center or nearest edge).
   *
   *   D     Disable    Disable the Unified Bed Leveling system.
   *
   *   E     Stow_probe Stow the probe after each sampled point.
   *
   *   F #   Fade       Fade the amount of Mesh Based Compensation over a specified height. At the
   *                    specified height, no correction is applied and natural printer kenimatics take over. If no
   *                    number is specified for the command, 10mm is assumed to be reasonable.
   *
   *   H #   Height     With P2, 'H' specifies the Height to raise the nozzle after each manual probe of the bed.
   *                    If omitted, the nozzle will raise by Z_CLEARANCE_BETWEEN_PROBES.
   *
   *   H #   Offset     With P4, 'H' specifies the Offset above the mesh height to place the nozzle.
   *                    If omitted, Z_CLEARANCE_BETWEEN_PROBES will be used.
   *
   *   I #   Invalidate Invalidate the specified number of Mesh Points near the given 'X' 'Y'. If X or Y are omitted,
   *                    the nozzle location is used. If no 'I' value is given, only the point nearest to the location is
   *                    invalidated. If a negative value is given, invalidate the entire mesh. Use 'T' to produce a map
   *                    afterward. This command is useful to invalidate a portion of the Mesh so it can be adjusted
   *                    using other UBL tools. When attempting to invalidate an isolated bad mesh point, the 'T' option
   *                    shows the nozzle position in the Mesh with (#). You can move the nozzle around and use this
   *                    feature to select the center of the area (or cell) to invalidate.
   *
   *   L     Load       Load Mesh from the previously activated location in the EEPROM.
   *
   *   L #   Load       Load Mesh from the specified location in the EEPROM. Set this location as activated
   *                    for subsequent Load and Store operations.
   *
   *   The P or Phase commands are used for the bulk of the work to setup a Mesh. In general, your Mesh will
   *   start off being initialized with a G29 P0 or a G29 P1. Further refinement of the Mesh happens with
   *   each additional Phase that processes it.
   *
   *   P0    Phase 0    Zero Mesh Data and turn off the Mesh Compensation System. This reverts the
   *                    3D Printer to the same state it was in before the Unified Bed Leveling Compensation
   *                    was turned on. Setting the entire Mesh to Zero is a special case that allows
   *                    a subsequent G or T leveling operation for backward compatibility.
   *
   *   P1    Phase 1    Invalidate entire Mesh and continue with automatic generation of the Mesh data using
   *                    the Z-Probe. Usually the probe can't reach all areas that the nozzle can reach. For delta
   *                    printers only the areas where the probe and nozzle can both reach will be automatically probed.
   *
   *                    Unreachable points will be handled in Phase 2 and Phase 3.
   *
   *                    Use 'C' to leave the previous mesh intact and automatically probe needed points. This allows you
   *                    to invalidate parts of the Mesh but still use Automatic Probing.
   *
   *                    The 'X' and 'Y' parameters prioritize where to try and measure points. If omitted, the current
   *                    probe position is used.
   *
   *                    Use 'T' (Topology) to generate a report of mesh generation.
   *
   *                    P1 will suspend Mesh generation if the controller button is held down. Note that you may need
   *                    to press and hold the switch for several seconds if moves are underway.
   *
   *   P2    Phase 2    Probe unreachable points.
   *
   *                    Use 'H' to set the height between Mesh points. If omitted, Z_CLEARANCE_BETWEEN_PROBES is used.
   *                    Smaller values will be quicker. Move the nozzle down till it barely touches the bed. Make sure the
   *                    nozzle is clean and unobstructed. Use caution and move slowly. This can damage your printer!
   *
   *                    The 'H' value can be negative if the Mesh dips in a large area. Press and hold the
   *                    controller button to terminate the current Phase 2 command. You can then re-issue "G29 P 2"
   *                    with an 'H' parameter more suitable for the area you're manually probing. Note that the command
   *                    tries to start in a corner of the bed where movement will be predictable. Override the distance
   *                    calculation location with the X and Y parameters. You can print a Mesh Map (G29 T) to see where
   *                    the mesh is invalidated and where the nozzle needs to move to complete the command. Use 'C' to
   *                    indicate that the search should be based on the current position.
   *
   *                    The 'B' parameter for this command is described above. It places the manual probe subsystem into
   *                    Business Card mode where the thickness of a business card is measured and then used to accurately
   *                    set the nozzle height in all manual probing for the duration of the command. A Business card can
   *                    be used, but you'll get better results with a flexible Shim that doesn't compress. This makes it
   *                    easier to produce similar amounts of force and get more accurate measurements. Google if you're
   *                    not sure how to use a shim.
   *
   *                    The 'T' (Map) parameter helps track Mesh building progress.
   *
   *                    NOTE: P2 requires an LCD controller!
   *
   *   P3    Phase 3    Fill the unpopulated regions of the Mesh with a fixed value. There are two different paths to
   *                    go down:
   *
   *                    - If a 'C' constant is specified, the closest invalid mesh points to the nozzle will be filled,
   *                      and a repeat count can then also be specified with 'R'. If a negative count is given,
   *                      the entire mesh is filled.
   *
   *                    - Leaving out 'C' invokes Smart Fill, which scans the mesh from the edges inward looking for
   *                      invalid mesh points. Adjacent points are used to determine the bed slope. If the bed is sloped
   *                      upward from the invalid point, it takes the value of the nearest point. If sloped downward, it's
   *                      replaced by a value that puts all three points in a line. This version of G29 P3 is a quick, easy
   *                      and (usually) safe way to populate unprobed mesh regions before continuing to G26 Mesh Validation
   *                      Pattern. Note that this populates the mesh with unverified values. Pay attention and use caution.
   *
   *   P4    Phase 4    Fine tune the Mesh. The Delta Mesh Compensation System assumes the existence of
   *                    an LCD Panel. It is possible to fine tune the mesh without an LCD Panel using
   *                    G42 and M421. See the UBL documentation for further details.
   *
   *                    Phase 4 is meant to be used with G26 Mesh Validation to fine tune the mesh by direct editing
   *                    of Mesh Points. Raise and lower points to fine tune the mesh until it gives consistently reliable
   *                    adhesion.
   *
   *                    P4 moves to the closest Mesh Point (and/or the given X Y), raises the nozzle above the mesh height
   *                    by the given 'H' offset (or default 0), and waits while the controller is used to adjust the nozzle
   *                    height. On click the displayed height is saved in the mesh.
   *
   *                    Start Phase 4 at a specific location with X and Y. Adjust a specific number of Mesh Points with
   *                    the 'R' (Repeat) parameter. (If 'R' is left out, the whole matrix is assumed.) This command can be
   *                    terminated early (e.g., after editing the area of interest) by pressing and holding the encoder button.
   *
   *                    The general form is G29 P4 [R points] [X position] [Y position]
   *
   *                    The H [offset] parameter is useful if a shim is used to fine-tune the mesh. For a 0.4mm shim the
   *                    command would be G29 P4 H0.4. The nozzle is moved to the shim height, you adjust height to the shim,
   *                    and on click the height minus the shim thickness will be saved in the mesh.
   *
   *                    !!Use with caution, as a very poor mesh could cause the nozzle to crash into the bed!!
   *
   *                    NOTE:  P4 is not available unless you have LCD support enabled!
   *
   *   P5    Phase 5    Find Mean Mesh Height and Standard Deviation. Typically, it is easier to use and
   *                    work with the Mesh if it is Mean Adjusted. You can specify a C parameter to
   *                    Correct the Mesh to a 0.00 Mean Height. Adding a C parameter will automatically
   *                    execute a G29 P6 C <mean height>.
   *
   *   P6    Phase 6    Shift Mesh height. The entire Mesh's height is adjusted by the height specified
   *                    with the C parameter. Being able to adjust the height of a Mesh is useful tool. It
   *                    can be used to compensate for poorly calibrated Z-Probes and other errors. Ideally,
   *                    you should have the Mesh adjusted for a Mean Height of 0.00 and the Z-Probe measuring
   *                    0.000 at the Z Home location.
   *
   *   Q     Test       Load specified Test Pattern to assist in checking correct operation of system. This
   *                    command is not anticipated to be of much value to the typical user. It is intended
   *                    for developers to help them verify correct operation of the Unified Bed Leveling System.
   *
   *   R #   Repeat     Repeat this command the specified number of times. If no number is specified the
   *                    command will be repeated GRID_MAX_POINTS_X * GRID_MAX_POINTS_Y times.
   *
   *   S     Store      Store the current Mesh in the Activated area of the EEPROM. It will also store the
   *                    current state of the Unified Bed Leveling system in the EEPROM.
   *
   *   S #   Store      Store the current Mesh at the specified location in EEPROM. Activate this location
   *                    for subsequent Load and Store operations. Valid storage slot numbers begin at 0 and
   *                    extend to a limit related to the available EEPROM storage.
   *
   *   S -1  Store      Print the current Mesh as G-code that can be used to restore the mesh anytime.
   *
   *   T     Topology   Display the Mesh Map Topology.
   *                    'T' can be used alone (e.g., G29 T) or in combination with most of the other commands.
   *                    This option works with all Phase commands (e.g., G29 P4 R 5 T X 50 Y100 C -.1 O)
   *                    This parameter can also specify a Map Type. T0 (the default) is user-readable. T1
   *                    is suitable to paste into a spreadsheet for a 3D graph of the mesh.
   *
   *   U     Unlevel    Perform a probe of the outer perimeter to assist in physically leveling unlevel beds.
   *                    Only used for G29 P1 T U. This speeds up the probing of the edge of the bed. Useful
   *                    when the entire bed doesn't need to be probed because it will be adjusted.
   *
   *   V #   Verbosity  Set the verbosity level (0-4) for extra details. (Default 0)
   *
   *   X #              X Location for this command
   *
   *   Y #              Y Location for this command
   *
   * With UBL_DEVEL_DEBUGGING:
   *
   *   K #  Kompare     Kompare current Mesh with stored Mesh #, replacing current Mesh with the result.
   *                    This command literally performs a diff between two Meshes.
   *
   *   Q-1  Dump EEPROM Dump the UBL contents stored in EEPROM as HEX format. Useful for developers to help
   *                    verify correct operation of the UBL.
   *
   *   W    What?       Display valuable UBL data.
   *
   *   UBL is founded on Edward Patel's Mesh Bed Leveling code. A big 'Thanks!' to him and the creators of
   *   3-Point and Grid Based leveling. Combining their contributions we now have the functionality and
   *   features of all three systems combined.
   */

  #if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()

  // Apply weighted correction on each point based on it's location.
  // The whole correction is then converted from µm to mm.

  float x_axis_correction(int x, int y) {
    int32_t left_correction_um{config_store().left_bed_correction.get()};
    int32_t right_correction_um{config_store().right_bed_correction.get()};

    int32_t x_len{GRID_MAX_POINTS_X-1};

    return ( (left_correction_um*(x_len-x)) + (right_correction_um*(x)) ) / static_cast<float>(x_len);
  }

  float y_axis_correction(int x, int y) {
    int32_t front_correction_um{config_store().front_bed_correction.get()};
    int32_t rear_correction_um{config_store().rear_bed_correction.get()};

    int32_t y_len{GRID_MAX_POINTS_Y-1};

    return ( (front_correction_um*(y_len-y)) + (rear_correction_um*(y)) ) / static_cast<float>(y_len);
  }

  void apply_bed_level_correction(int x, int y) {
    ubl.z_values[x][y] += 0.001f*(x_axis_correction(x, y) + y_axis_correction(x,y));
  }

  #endif

  void unified_bed_leveling::G29() {
    // Setting pwm on heatbreak fan on coreone causes electrical and mechanical noise on loadcell.
    // To make sure that the MBL is perfect as possible we turn the fan on without pwm modulation.
    // This is potentionaly a problem on all the xBuddy boards with love board.
    //
    // NOTE: We can't turn the fan off, because some filaments can start get runny in the whole tube that goes to the nozzle.
    #if HAS_LOVE_BOARD() && !PRINTER_IS_PRUSA_iX()
    auto &fan = Fans::heat_break(PhysicalToolIndex::from_raw(0));
    fan.disable_autocontrol();
    if (buddy::hw::Configuration::Instance().has_inverted_fans()) {
        buddy::hw::fanHeatBreakPwm.write(buddy::hw::Pin::State::low);
    } else {
        buddy::hw::fanHeatBreakPwm.write(buddy::hw::Pin::State::high);
    }
    ScopeGuard restore_fan_autocontrol([&]() {
        fan.enable_autocontrol();
    });
    #endif

    bool probe_deployed = false;
    if (g29_parameter_parsing()) return; // Abort on parameter error

    if (g29_wait_for_preheat) {
        Temperature::wait_for_frame_heatup();
    }

    // Check for commands that require the printer to be homed
    const bool may_move = g29_phase_value == 1 || g29_phase_value == 2 || g29_phase_value == 4;
    if (may_move) {
      if (axes_need_homing()) {
        gcode.process_subcommands_now_P(PSTR("G28 O"));
      }
    }

    // Invalidate Mesh Points. This command is a little bit asymmetrical because
    // it directly specifies the repetition count and does not use the 'R' parameter.
    if (parser.seen('I')) {
      uint8_t cnt = 0;
      g29_repetition_cnt = parser.has_value() ? parser.value_int() : 1;
      if (g29_repetition_cnt >= GRID_MAX_POINTS || g29_repetition_cnt < 0) {
        set_all_mesh_points_to_value(NAN);
      }
      else {
        while (g29_repetition_cnt--) {
          if (cnt > 20) { cnt = 0; idle(false); }
          const mesh_index_pair closest = find_closest_mesh_point_of_type(REAL, g29_pos);
          const xy_int8_t &cpos = closest.pos;
          if (cpos.x < 0) {
            // No more REAL mesh points to invalidate, so we ASSUME the user
            // meant to invalidate the ENTIRE mesh, which cannot be done with
            // find_closest_mesh_point loop which only returns REAL points.
            set_all_mesh_points_to_value(NAN);
            SERIAL_ECHOLNPGM("Entire Mesh invalidated.\n");
            break;            // No more invalid Mesh Points to populate
          }
          z_values[cpos.x][cpos.y] = NAN;
          #if ENABLED(EXTENSIBLE_UI)
            ExtUI::onMeshUpdate(closest.pos.x, closest.pos.y, 0);
          #endif
          cnt++;
        }
      }
      SERIAL_ECHOLNPGM("Locations invalidated.\n");
    }

    if (parser.seen('Q')) {
      const int test_pattern = parser.has_value() ? parser.value_int() : -99;
      if (!WITHIN(test_pattern, -1, 2)) {
        SERIAL_ECHOLNPGM("Invalid test_pattern value. (-1 to 2)\n");
        return;
      }
      SERIAL_ECHOLNPGM("Loading test_pattern values.\n");
      switch (test_pattern) {

        #if ENABLED(UBL_DEVEL_DEBUGGING)
          // #error dead code found by automatic analyses (see BFW-5461)
          case -1:
            g29_eeprom_dump();
            break;
        #endif

        case 0:
          for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++) {   // Create a bowl shape - similar to
            for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++) { // a poorly calibrated Delta.
              const float p1 = 0.5f * (GRID_MAX_POINTS_X) - x,
                          p2 = 0.5f * (GRID_MAX_POINTS_Y) - y;
              z_values[x][y] += 2.0f * HYPOT(p1, p2);
              #if ENABLED(EXTENSIBLE_UI)
                ExtUI::onMeshUpdate(x, y, z_values[x][y]);
              #endif
            }
          }
          break;

        case 1:
          for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++) {  // Create a diagonal line several Mesh cells thick that is raised
            z_values[x][x] += 9.999f;
            z_values[x][x + (x < GRID_MAX_POINTS_Y - 1) ? 1 : -1] += 9.999f; // We want the altered line several mesh points thick
            #if ENABLED(EXTENSIBLE_UI)
              ExtUI::onMeshUpdate(x, x, z_values[x][x]);
              ExtUI::onMeshUpdate(x, (x + (x < GRID_MAX_POINTS_Y - 1) ? 1 : -1), z_values[x][x + (x < GRID_MAX_POINTS_Y - 1) ? 1 : -1]);
            #endif

          }
          break;

        case 2:
          // Allow the user to specify the height because 10mm is a little extreme in some cases.
          for (uint8_t x = (GRID_MAX_POINTS_X) / 3; x < 2 * (GRID_MAX_POINTS_X) / 3; x++)     // Create a rectangular raised area in
            for (uint8_t y = (GRID_MAX_POINTS_Y) / 3; y < 2 * (GRID_MAX_POINTS_Y) / 3; y++) { // the center of the bed
              z_values[x][y] += parser.seen('C') ? g29_constant : 9.99f;
              #if ENABLED(EXTENSIBLE_UI)
                ExtUI::onMeshUpdate(x, y, z_values[x][y]);
              #endif
            }
          break;
      }
    }

    if (parser.seen('P')) {
      if (WITHIN(g29_phase_value, 0, 1) && storage_slot == -1) {
        storage_slot = 0;
        SERIAL_ECHOLNPGM("Default storage slot 0 selected.");
      }

      switch (g29_phase_value) {
        case 0:
          //
          // Zero Mesh Data
          //
          reset();
          SERIAL_ECHOLNPGM("Mesh zeroed.");
          break;

        #if HAS_BED_PROBE

          case 1: {
            //
            // Invalidate Entire Mesh and Automatically Probe Mesh in areas that can be reached by the probe
            //
            const bool extend_mesh = parser.seen('C');

            if (!extend_mesh) {
              invalidate();
              SERIAL_ECHOLNPGM("Mesh invalidated. Probing mesh.");
            }

            #if HAS_CRASH_DETECTION()
              // we're going to move to an absolute position: inhibit XYZ repositioning
              crash_s.set_gcode_replay_flags(Crash_s::RECOVER_AXIS_STATE);
            #endif

            const auto probe_area = ((xy_seen.x || xy_seen.y) && g29_size_seen) ?
              PrintArea::rect_t(g29_pos, g29_pos + g29_size) :
              // probe area is print area enlarged by one major point
              print_area.get_bounding_rect().inset(-MESH_X_DIST * GRID_MAJOR_STEP,
                                                   -MESH_Y_DIST * GRID_MAJOR_STEP);

            probe_major_points(probe_area, parser.seen('T'), parser.seen('E'), extend_mesh);

            report_current_position();
            probe_deployed = true;
          } break;

        #endif // HAS_BED_PROBE

        case 2: {
          SERIAL_ECHOLNPGM("?P2 is only available when an LCD is present.");
          return;
        } break;

        case 3: {
          /**
           * Populate invalid mesh areas. Proceed with caution.
           * Two choices are available:
           *   - Specify a constant with the 'C' parameter.
           *   - Allow 'G29 P3' to choose a 'reasonable' constant.
           */

          if (g29_c_flag) {
            if (g29_repetition_cnt >= GRID_MAX_POINTS || g29_repetition_cnt < 0) {
              set_all_mesh_points_to_value(g29_constant);
            }
            else {
              while (g29_repetition_cnt--) {  // this only populates reachable mesh points near
                const mesh_index_pair closest = find_closest_mesh_point_of_type(INVALID, g29_pos);
                const xy_int8_t &cpos = closest.pos;
                if (cpos.x < 0) {
                  // No more REAL INVALID mesh points to populate, so we ASSUME
                  // user meant to populate ALL INVALID mesh points to value
                  for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
                    for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++)
                      if (isnan(z_values[x][y])) z_values[x][y] = g29_constant;
                  break; // No more invalid Mesh Points to populate
                }
                else {
                  z_values[cpos.x][cpos.y] = g29_constant;
                  #if ENABLED(EXTENSIBLE_UI)
                    ExtUI::onMeshUpdate(closest.pos.x, closest.pos.y, g29_constant);
                  #endif
                }
              }
            }
          }
          else {
            const float cvf = parser.value_float();
            switch ((int)truncf(cvf * 10.0f) - 30) {   // 3.1 -> 1
              #if ENABLED(UBL_G29_P31)
                case 1: {

                  // P3.1  use least squares fit to fill missing mesh values
                  // P3.10 zero weighting for distance, all grid points equal, best fit tilted plane
                  // P3.11 10X weighting for nearest grid points versus farthest grid points
                  // P3.12 100X distance weighting
                  // P3.13 1000X distance weighting, approaches simple average of nearest points

                  const float weight_power  = (cvf - 3.10f) * 100.0f,  // 3.12345 -> 2.345
                              weight_factor = weight_power ? POW(10.0f, weight_power) : 0;
                  smart_fill_wlsf(weight_factor);
                }
                break;
              case 2: {
                bicubic_fill_mesh();
                break;
              }
              #endif
              case 0:   // P3 or P3.0
              default:  // and anything P3.x that's not P3.1
                smart_fill_mesh();  // Do a 'Smart' fill using nearby known values
                break;
            }
          }
          break;
        }

        case 4: // Fine Tune (i.e., Edit) the Mesh
          SERIAL_ECHOLNPGM("?P4 is only available when an LCD is present.");
          return;
          break;

        case 5: adjust_mesh_to_mean(g29_c_flag, g29_constant); break;

        case 6: shift_mesh_height(); break;

        #if ENABLED(NOZZLE_LOAD_CELL) && ENABLED(PROBE_CLEANUP_SUPPORT)
          case 9: {
            #if HAS_CRASH_DETECTION()
              // we're going to move to an absolute position: inhibit XYZ repositioning
              crash_s.set_gcode_replay_flags(Crash_s::RECOVER_AXIS_STATE);
            #endif
            #if HAS_NOZZLE_CLEANER_LITE()
              // The nozzle cleaner physically removes debris instead of just
              // detecting it, and cleans at a fixed hardcoded location, so it
              // doesn't need the X/Y/W/H rectangle the loadcell tap-based
              // cleaning below scans.
              if (nozzle_cleaner_lite::is_available()) {
                // The start gcode set the probing temperature; clean at it and
                // keep the cooled-down default target for the probing that follows.
                ubl.g29_nozzle_cleaning_failed |= !nozzle_cleaner_lite::clean(
                  { .cleaning_temp = thermalManager.degTargetHotend(active_extruder), .cooldown = true, .keep_target = true });
                break;
              }
            #endif
            if (g29_size_seen && (xy_seen.x || xy_seen.y)) {
              ubl.g29_nozzle_cleaning_failed |= !cleanup_probe(g29_pos, g29_pos + g29_size);
            } else {
              SERIAL_ECHOLNPGM("G29 P9 requires X, Y, W and H arguments");
              return;
            }
            break;
          }
        #endif

        case 10: probe_at_point(g29_pos, parser.seen('E') ? PROBE_PT_STOW : PROBE_PT_RAISE, g29_verbose_level); break;
      }
    }

    #if ENABLED(UBL_DEVEL_DEBUGGING)
      // #error dead code found by automatic analyses (see BFW-5461)

      //
      // Much of the 'What?' command can be eliminated. But until we are fully debugged, it is
      // good to have the extra information. Soon... we prune this to just a few items
      //
      if (parser.seen('W')) g29_what_command();

      //
      // When we are fully debugged, this may go away. But there are some valid
      // use cases for the users. So we can wait and see what to do with it.
      //

      if (parser.seen('K')) // Kompare Current Mesh Data to Specified Stored Mesh
        g29_compare_current_mesh_to_stored_mesh();

    #endif // UBL_DEVEL_DEBUGGING

    if (parser.seen('T'))
      display_map(g29_map_type);

    if (!parser.seen_any()) {
        // backward compatibility with ABL
        gcode.process_subcommands_now_P("G29 P1 X0 Y0");
        gcode.process_subcommands_now_P("G29 P3.2");
        gcode.process_subcommands_now_P("G29 P3.13");
        gcode.process_subcommands_now_P("G29 A");
    }

    #ifdef Z_PROBE_END_SCRIPT
      // #error dead code found by automatic analyses (see BFW-5461)
      if (probe_deployed) {
        planner.synchronize();
        gcode.process_subcommands_now_P(PSTR(Z_PROBE_END_SCRIPT));
      }
    #else
      UNUSED(probe_deployed);
    #endif

    return;
  }

  void unified_bed_leveling::adjust_mesh_to_mean(const bool cflag, const float value) {
    float sum = 0;
    int n = 0;
    for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
      for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++)
        if (!isnan(z_values[x][y])) {
          sum += z_values[x][y];
          n++;
        }

    const float mean = sum / n;

    //
    // Sum the squares of difference from mean
    //
    float sum_of_diff_squared = 0;
    for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
      for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++)
        if (!isnan(z_values[x][y]))
          sum_of_diff_squared += sq(z_values[x][y] - mean);

    SERIAL_ECHOLNPAIR("# of samples: ", n);
    SERIAL_ECHOLNPAIR_F("Mean Mesh Height: ", mean, 6);

    const float sigma = SQRT(sum_of_diff_squared / (n + 1));
    SERIAL_ECHOLNPAIR_F("Standard Deviation: ", sigma, 6);

    if (cflag)
      for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
        for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++)
          if (!isnan(z_values[x][y])) {
            z_values[x][y] -= mean + value;
            #if ENABLED(EXTENSIBLE_UI)
              ExtUI::onMeshUpdate(x, y, z_values[x][y]);
            #endif
          }
  }

  void unified_bed_leveling::shift_mesh_height() {
    for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
      for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++)
        if (!isnan(z_values[x][y])) {
          z_values[x][y] += g29_constant;
          #if ENABLED(EXTENSIBLE_UI)
            ExtUI::onMeshUpdate(x, y, z_values[x][y]);
          #endif
        }
  }

#if HAS_BED_PROBE
  void unified_bed_leveling::probe_major_points(const PrintArea::rect_t &probe_area, const bool do_ubl_mesh_map, const bool stow_probe, const bool extend_mesh) {
    save_ubl_active_state_and_disable();  // No bed level correction so only raw data is obtained
    pressure_advance::PressureAdvanceDisabler pa_disabler; // Reduce move delays as we don't extrude

    #if ENABLED(NOZZLE_LOAD_CELL)
      // Enable loadcell high precision across the entire sequence to prime the noise filters
      auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell);
    #endif

    #if UBL_TRAVEL_ACCELERATION
      auto saved_acceleration = planner.settings.travel_acceleration;
      {
        auto s = planner.user_settings;
        s.travel_acceleration = UBL_TRAVEL_ACCELERATION;
        planner.apply_settings(s);
      }
    #endif

    PrintStatusMessageGuard statusGuard;

    /**
      * Enumerate over points in the grid.
      * @param func Function to be called for each grid point.
      */  
    auto for_each_grid_point = [&](auto&& func) {
      for (int y = GRID_MAX_POINTS_Y - GRID_BORDER - 1; y >= GRID_BORDER; y -= GRID_MAJOR_STEP) {
        int y_idx = (y - GRID_BORDER) / GRID_MAJOR_STEP;
        bool is_odd_y_position = y_idx % 2 == 1;
        int x0 = is_odd_y_position ? GRID_BORDER : GRID_MAX_POINTS_X - 1 - GRID_BORDER;
        int xStep = is_odd_y_position ? GRID_MAJOR_STEP : -GRID_MAJOR_STEP;

        for (int x = x0; GRID_BORDER <= x && x < GRID_MAX_POINTS_X - GRID_BORDER; x += xStep) {
          if(ubl.g29_probing_failed) {
            return;
          }
          
          xy_pos_t pos = {mesh_index_to_xpos(x), mesh_index_to_ypos(y)};

          // skip points the probe can't reach
          if (!position_is_reachable_by_probe(pos.x, pos.y))
              continue;

          // skip points outside print area
          if (!probe_area.contains(pos))
              continue;

          func(pos, x, y);
        }
      }
    };

    int num_of_points_to_probe = 0;
    for_each_grid_point([&](const xy_pos_t& pos, int x, int y) {
      num_of_points_to_probe++;
    });

    bool is_initial_probe = true;
    int num_of_probed_points = 0;
    for_each_grid_point([&](const xy_pos_t& pos, int x, int y) {
      // skip points having meaningful value already
      if (z_values[x][y] != 0 && !std::isnan(z_values[x][y])) 
        return;

      // print UBL map if we were told to do so
      if (do_ubl_mesh_map)
        display_map(g29_map_type);

      // make initial move manually (has a different speed)
      if (is_initial_probe) {
        xyz_pos_t start_pos = xyz_pos_t(pos);
        start_pos -= probe_offset;
        start_pos.z = Z_CLEARANCE_BEFORE_PROBING;
        #if HAS_HOTEND_OFFSET
        start_pos -= hotend_currently_applied_offset;
        #endif
        do_blocking_move_to(start_pos.x, start_pos.y, start_pos.z);
        is_initial_probe = false;
      }

      num_of_probed_points++;
      // Display different status messages depending on whether we are extending the mesh (used for probing the purge area) or not (print area probing)
      if (extend_mesh) {
        statusGuard.update<PrintStatusMessage::additional_probing>({.current = (float) num_of_probed_points, .target = (float) num_of_points_to_probe});
      } else {
        statusGuard.update<PrintStatusMessage::probing_bed>({.current = (float) num_of_probed_points, .target = (float) num_of_points_to_probe});
      }

      // and finally, probe
      const float measured_z = probe_at_point(
        pos, 
        stow_probe ? PROBE_PT_STOW : PROBE_PT_RAISE, 
        g29_verbose_level
      );

      if (std::isnan(measured_z)) {
        ubl.g29_probing_failed = true;
        return;
      }
      z_values[x][y] = measured_z;

      const auto prev_measured_z = g29_min_max_measured_z.value_or(MeasuredZ{measured_z, measured_z});
      g29_min_max_measured_z = { std::min(prev_measured_z.min, measured_z), std::max(prev_measured_z.max, measured_z) };
      log_info(Marlin, "Measured z: %f", (double) measured_z);

      #if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
        //apply bed level correction on each probed point
        apply_bed_level_correction(x,y);
      #endif

      #if ENABLED(EXTENSIBLE_UI)
        ExtUI::onMeshUpdate(x, y, measured_z);
      #endif
    });

    // make sure the probe is stowed when finished no matter the `stow_probe` argument
    STOW_PROBE();

    #ifdef Z_AFTER_PROBING
      move_z_after_probing();
    #endif

    #if UBL_TRAVEL_ACCELERATION
      {
        auto s = planner.user_settings;
        s.travel_acceleration = saved_acceleration;
        planner.apply_settings(s);
      }
    #endif

    restore_ubl_active_state_and_leave();
  }  


#endif // HAS_BED_PROBE

  bool unified_bed_leveling::g29_parameter_parsing() {
    bool err_flag = false;

    g29_constant = 0;
    g29_repetition_cnt = 0;

    g29_wait_for_preheat = parser.seen('G');

    if (parser.seen('R')) {
      g29_repetition_cnt = parser.has_value() ? parser.value_int() : GRID_MAX_POINTS;
      NOMORE(g29_repetition_cnt, GRID_MAX_POINTS);
      if (g29_repetition_cnt < 1) {
        SERIAL_ECHOLNPGM("?(R)epetition count invalid (1+).\n");
        return UBL_ERR;
      }
    }

    g29_verbose_level = parser.seen('V') ? parser.value_int() : 0;
    if (!WITHIN(g29_verbose_level, 0, 4)) {
      SERIAL_ECHOLNPGM("?(V)erbose level implausible (0-4).\n");
      err_flag = true;
    }

    if (parser.seen('P')) {
      const int pv = parser.value_int();
      #if !HAS_BED_PROBE
        // #error dead code found by automatic analyses (see BFW-5461)
        if (pv == 1) {
          SERIAL_ECHOLNPGM("G29 P1 requires a probe.\n");
          err_flag = true;
        }
        else
      #endif
        {
          g29_phase_value = pv;
          if (!WITHIN(g29_phase_value, 0, 6) && !WITHIN(g29_phase_value, 9, 10)) {
            SERIAL_ECHOLNPGM("?(P)hase value invalid (0-6).\n");
            err_flag = true;
          }
        }
    }

    xy_seen.x = parser.seenval('X');
    float sx = xy_seen.x ? parser.value_float() : current_position.x;
    xy_seen.y = parser.seenval('Y');
    float sy = xy_seen.y ? parser.value_float() : current_position.y;

    if (xy_seen.x != xy_seen.y) {
      SERIAL_ECHOLNPGM("Both X & Y locations must be specified.\n");
      err_flag = true;
    }

    // Make sure <X,Y> is a valid positions
    sx = std::clamp(sx, static_cast<float>(X_MIN_POS), static_cast<float>(X_MAX_POS));
    sy = std::clamp(sy, static_cast<float>(Y_MIN_POS), static_cast<float>(Y_MAX_POS));

    if (err_flag) return UBL_ERR;

    g29_pos.set(sx, sy);

    if (parser.seenval('W') && parser.seenval('H')) {
      g29_size.set(parser.floatval('W'), parser.floatval('H'));
      g29_size_seen = true;
    } else {
      g29_size_seen = false;
    }

    /**
     * Activate or deactivate UBL
     * Note: UBL's G29 restores the state set here when done.
     *       Leveling is being enabled here with old data, possibly
     *       none. Error handling should disable for safety...
     */
    if (parser.seen('A')) {
      if (parser.seen('D')) {
        SERIAL_ECHOLNPGM("?Can't activate and deactivate at the same time.\n");
        return UBL_ERR;
      }
      set_bed_leveling_enabled(true);
      report_state();

      #if HAS_POWER_PANIC()
      // prepare for PP only when successful
      if (!planner.draining())
        power_panic::prepare();
      #endif
    }
    else if (parser.seen('D')) {
      set_bed_leveling_enabled(false);
      report_state();
    }

    // Set global 'C' flag and its value
    if ((g29_c_flag = parser.seen('C')))
      g29_constant = parser.value_float();

    #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
      if (parser.seenval('F')) {
        const float fh = parser.value_float();
        if (!WITHIN(fh, 0, 100)) {
          SERIAL_ECHOLNPGM("?(F)ade height for Bed Level Correction not plausible.\n");
          return UBL_ERR;
        }
        set_z_fade_height(fh);
      }
    #endif

    g29_map_type = parser.intval('T');
    if (!WITHIN(g29_map_type, 0, 2)) {
      SERIAL_ECHOLNPGM("Invalid map type.\n");
      return UBL_ERR;
    }
    return UBL_OK;
  }

  static uint8_t ubl_state_at_invocation = 0;

  #if ENABLED(UBL_DEVEL_DEBUGGING)
    // #error dead code found by automatic analyses (see BFW-5461)
    static uint8_t ubl_state_recursion_chk = 0;
  #endif

  void unified_bed_leveling::save_ubl_active_state_and_disable() {
    #if ENABLED(UBL_DEVEL_DEBUGGING)
      // #error dead code found by automatic analyses (see BFW-5461)
      ubl_state_recursion_chk++;
      if (ubl_state_recursion_chk != 1) {
        SERIAL_ECHOLNPGM("save_ubl_active_state_and_disabled() called multiple times in a row.");
        return;
      }
    #endif
    ubl_state_at_invocation = planner.leveling_active;
    set_bed_leveling_enabled(false);
  }

  void unified_bed_leveling::restore_ubl_active_state_and_leave() {
    #if ENABLED(UBL_DEVEL_DEBUGGING)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (--ubl_state_recursion_chk) {
        SERIAL_ECHOLNPGM("restore_ubl_active_state_and_leave() called too many times.");
        return;
      }
    #endif
    set_bed_leveling_enabled(ubl_state_at_invocation);
  }

  mesh_index_pair unified_bed_leveling::find_furthest_invalid_mesh_point() {

    bool found_a_NAN = false, found_a_real = false;

    mesh_index_pair farthest { .pos { -1, -1 }, .distance = -99999.99f };

    for (int8_t i = 0; i < GRID_MAX_POINTS_X; i++) {
      for (int8_t j = 0; j < GRID_MAX_POINTS_Y; j++) {

        if (isnan(z_values[i][j])) {                  // Invalid mesh point?

          // Skip points the probe can't reach
          if (!position_is_reachable_by_probe(mesh_index_to_xpos(i), mesh_index_to_ypos(j)))
            continue;

          found_a_NAN = true;

          xy_int8_t near { -1, -1 };
          float d1, d2 = 99999.9f;
          for (int8_t k = 0; k < GRID_MAX_POINTS_X; k++) {
            for (int8_t l = 0; l < GRID_MAX_POINTS_Y; l++) {
              if (!isnan(z_values[k][l])) {
                found_a_real = true;

                // Add in a random weighting factor that scrambles the probing of the
                // last half of the mesh (when every unprobed mesh point is one index
                // from a probed location).

                d1 = HYPOT(i - k, j - l) + (1.0f / ((millis() % 47) + 13));

                if (d1 < d2) {    // Invalid mesh point (i,j) is closer to the defined point (k,l)
                  d2 = d1;
                  near.set(i, j);
                }
              }
            }
          }

          //
          // At this point d2 should have the near defined mesh point to invalid mesh point (i,j)
          //

          if (found_a_real && near.x >= 0 && d2 > farthest.distance) {
            farthest.pos = near;      // Found an invalid location farther from the defined mesh point
            farthest.distance = d2;
          }
        }
      } // for j
    } // for i

    if (!found_a_real && found_a_NAN) {        // if the mesh is totally unpopulated, start the probing
      farthest.pos.set(GRID_MAX_POINTS_X / 2, GRID_MAX_POINTS_Y / 2);
      farthest.distance = 1;
    }
    return farthest;
  }

  mesh_index_pair unified_bed_leveling::find_closest_mesh_point_of_type(const MeshPointType type, const xy_pos_t &pos, const bool probe_relative/*=false*/) {
    mesh_index_pair closest;
    closest.invalidate();
    closest.distance = -99999.9f;

    // Get the reference position, either nozzle or probe
    const xy_pos_t ref = probe_relative ? pos + probe_offset.xy() : pos;

    float best_so_far = 99999.99f;

    for (int8_t i = 0; i < GRID_MAX_POINTS_X; i++) {
      for (int8_t j = 0; j < GRID_MAX_POINTS_Y; j++) {
        if (type == (isnan(z_values[i][j]) ? INVALID : REAL)) {
          // Found a Mesh Point of the specified type!
          const xy_pos_t mpos = { mesh_index_to_xpos(i), mesh_index_to_ypos(j) };

          // If using the probe as the reference there are some unreachable locations.
          // Also for round beds, there are grid points outside the bed the nozzle can't reach.
          // Prune them from the list and ignore them till the next Phase (manual nozzle probing).

          if (probe_relative ? !position_is_reachable_by_probe(mpos) : !position_is_reachable(mpos))
            continue;

          // Reachable. Check if it's the best_so_far location to the nozzle.

          const xy_pos_t diff = (current_position - mpos).xy();
          const float distance = (ref - mpos).magnitude() + diff.magnitude() * 0.1f;

          // factor in the distance from the current location for the normal case
          // so the nozzle isn't running all over the bed.
          if (distance < best_so_far) {
            best_so_far = distance;   // Found a closer location with the desired value type.
            closest.pos.set(i, j);
            closest.distance = best_so_far;
          }
        }
      } // for j
    } // for i

    return closest;
  }

  /**
   * 'Smart Fill': Scan from the outward edges of the mesh towards the center.
   * If an invalid location is found, use the next two points (if valid) to
   * calculate a 'reasonable' value for the unprobed mesh point.
   */

  bool unified_bed_leveling::smart_fill_one(const uint8_t x, const uint8_t y, const int8_t xdir, const int8_t ydir) {
    const float v = z_values[x][y];
    if (isnan(v)) {                           // A NAN...
      const int8_t dx = x + xdir, dy = y + ydir;
      const float v1 = z_values[dx][dy];
      if (!isnan(v1)) {                       // ...next to a pair of real values?
        const float v2 = z_values[dx + xdir][dy + ydir];
        if (!isnan(v2)) {
          z_values[x][y] = v1 < v2 ? v1 : v1 + v1 - v2;
          #if ENABLED(EXTENSIBLE_UI)
            ExtUI::onMeshUpdate(x, y, z_values[x][y]);
          #endif
          return true;
        }
      }
    }
    return false;
  }

  typedef struct { uint8_t sx, ex, sy, ey; bool yfirst; } smart_fill_info;

  void unified_bed_leveling::smart_fill_mesh() {
    static const smart_fill_info
      info0 PROGMEM = { 0, GRID_MAX_POINTS_X,      0, GRID_MAX_POINTS_Y - 2,  false },  // Bottom of the mesh looking up
      info1 PROGMEM = { 0, GRID_MAX_POINTS_X,      GRID_MAX_POINTS_Y - 1, 0,  false },  // Top of the mesh looking down
      info2 PROGMEM = { 0, GRID_MAX_POINTS_X - 2,  0, GRID_MAX_POINTS_Y,      true  },  // Left side of the mesh looking right
      info3 PROGMEM = { GRID_MAX_POINTS_X - 1, 0,  0, GRID_MAX_POINTS_Y,      true  };  // Right side of the mesh looking left
    static const smart_fill_info * const info[] PROGMEM = { &info0, &info1, &info2, &info3 };

    for (uint8_t i = 0; i < COUNT(info); ++i) {
      const smart_fill_info *f = (smart_fill_info*)pgm_read_ptr(&info[i]);
      const int8_t sx = pgm_read_byte(&f->sx), sy = pgm_read_byte(&f->sy),
                   ex = pgm_read_byte(&f->ex), ey = pgm_read_byte(&f->ey);
      if (pgm_read_byte(&f->yfirst)) {
        const int8_t dir = ex > sx ? 1 : -1;
        for (uint8_t y = sy; y != ey; ++y)
          for (uint8_t x = sx; x != ex; x += dir)
            if (smart_fill_one(x, y, dir, 0)) break;
      }
      else {
        const int8_t dir = ey > sy ? 1 : -1;
         for (uint8_t x = sx; x != ex; ++x)
          for (uint8_t y = sy; y != ey; y += dir)
            if (smart_fill_one(x, y, 0, dir)) break;
      }
    }
  }

  /**
   *  Cubic Interpolation
   *
   *  Interpolates a value between two values p[1], p[2] using a third order polynomial.
   *  Values p[0] and p[3] are used to approximate the first derivative of the polynomial at x=0 and x=1.
   *  In case p[0] or p[1] are NaN, their values are estimated based on p[1] and p[2].
   *
   *  The function returns a single interpolated value at x=`x` (expecting x to be an element of interval (0, 1)).
   *
   *                           ^
   *                         y |
   *                           |          XXXXXX p[3]
   *                           |     XXXXXX    XXXXXX
   *                           | XXXXX p[2]         XXXX
   *       XX                  XX                      XXXX
   *         X              XXXX                           XXX
   *          XXX    p[0] XX   |p[1]                          XX
   *             XXXXXXXXXX    |
   *                  +        |        +        +
   *       <---------------------------------------------------->
   *                  |        +        +        +            x
   *                  -1       0        1        2
   *
   */
  float unified_bed_leveling::bicubic_interp(const float p[4], float x) {
    float p0 = p[0], p3 = p[3];
    const float p1 = p[1], p2 = p[2];
    if (isnan(p0))
      p0 = 2 * p1 - p2;
    if (isnan(p3))
      p3 = 2 * p2 - p1;
    return p1 + 0.5f * x * (p2 - p0 + x * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 + x * (3.0f * (p1 - p2) + p3 - p0)));
  }

  /**
   * Bicubic Interpolation
   *
   * Interpolates a value at position `pos` (expected to be an element of ((0, 1), (0, 1))).
   * Values `p[x][y]` have to have analogous values to the `p` array in bicubic_interp:
   *
   *                    ^
   *                    |
   *                    |
   *           +--------2-----------------+
   *           |p[0][3] |p[1][3] |p[2][3] |p[3][3]
   *           |        |        |        |
   *           |        |        |        |
   *           +--------1-----------------+
   *           |p[0][2] |p[1][2] |p[2][2] |p[3][2]
   *           |        |        |        |
   *           |        |        |        |
   *      <-- -1-------0,0-------1--------2----->
   *           |p[0][1] |p[1][1] |p[2][1] |p[3][1]
   *           |        |        |        |
   *           |        |        |        |
   *           +--------1-----------------+
   *            p[0][0] |p[1][0]  p[2][0] |p[3][0]
   *                    |
   *                    v
   */
  float unified_bed_leveling::bicubic_interp2d(const float p[4][4], xy_pos_t pos) {
    float x[4];
    x[0] = bicubic_interp(p[0], pos.y);
    x[1] = bicubic_interp(p[1], pos.y);
    x[2] = bicubic_interp(p[2], pos.y);
    x[3] = bicubic_interp(p[3], pos.y);
    return bicubic_interp(x, pos.x);
  }

  void unified_bed_leveling::bicubic_fill_mesh() {
    // precondition: all major points have to be available already

    float p[4][4]; // 2d array used for interpolation

    // iterate over each sub-region of the bed of size (GRID_MAJOR_STEP, GRID_MAJOR_STEP)
    // and interpolate all the points within this square
    for (uint8_t x0 = GRID_BORDER; x0 < GRID_MAX_POINTS_X - GRID_BORDER - GRID_MAJOR_STEP; x0 += GRID_MAJOR_STEP) {
      for (uint8_t y0 = GRID_BORDER; y0 < GRID_MAX_POINTS_Y - GRID_BORDER - GRID_MAJOR_STEP; y0 += GRID_MAJOR_STEP) {

        // fill in the `p` array with major points around
        for (int i = -1; i < 3; i++) {
          for (int j = -1; j < 3; j++) {
            int x = x0 + i * GRID_MAJOR_STEP;
            int y = y0 + j * GRID_MAJOR_STEP;
            if (x < 0 || x >= GRID_MAX_POINTS_X || y < 0 || y >= GRID_MAX_POINTS_Y)
              p[i + 1][j + 1] = NAN;
            else
              p[i + 1][j + 1] = z_values[x][y];
          }
        }

        // iterate over all the missing points within the sub-region and perform interpolation
        const float step = 1.0f / GRID_MAJOR_STEP;
        for (int i = 0; i <= GRID_MAJOR_STEP; i++) {
          for (int j = 0; j <= GRID_MAJOR_STEP; j++) {
            if (!isnan(z_values[x0 + i][y0 + j]))
              continue; // skip if we have a Z coordinate already
            z_values[x0 + i][y0 + j] = bicubic_interp2d(p, {step * i, step * j});
          }
        }
      }
    }

    // We have everything except the border, let's approximate it now based on the values around.
    for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x ++) {
      for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y ++) {
        bool is_horizontal = x < GRID_BORDER || x >= (GRID_MAX_POINTS_X - GRID_BORDER);
        bool is_vertical = y < GRID_BORDER || y >= (GRID_MAX_POINTS_Y - GRID_BORDER);
        if (!is_horizontal && !is_vertical)
          continue;
        static_assert(GRID_BORDER == 1, "Only GRID_BORDER equal to 1 is currently supported");
        // 1. precalculate the direction, in which we want to find samples to approximate
        //    the current position's value.
        xy_int_t direction;
        direction.x = is_horizontal ? (x == 0 ? +1 : -1) : 0;
        direction.y = is_vertical ? (y == 0 ? +1 : -1) : 0;
        // 2. Take first two samples in that direction.
        xy_int_t current_pos = {x, y};
        xy_int_t first_sample_pos = current_pos + direction;
        xy_int_t second_sample_pos = first_sample_pos + direction;
        const float first_sample = z_values[first_sample_pos.x][first_sample_pos.y];
        const float second_sample = z_values[second_sample_pos.x][second_sample_pos.y];
        // 3. Use them for simple linear approximation of the border.
        z_values[x][y] = 2 * first_sample - second_sample;
      }
    }
  }

  #if ENABLED(UBL_G29_P31)
    void unified_bed_leveling::smart_fill_wlsf(const float &weight_factor) {

      // For each undefined mesh point, compute a distance-weighted least squares fit
      // from all the originally populated mesh points, weighted toward the point
      // being extrapolated so that nearby points will have greater influence on
      // the point being extrapolated.  Then extrapolate the mesh point from WLSF.

      uint32_t bitmap[GRID_MAX_POINTS_X] = { 0 };
      struct linear_fit_data lsf_results;

      SERIAL_ECHOLNPGM("Extrapolating mesh...");

      const float weight_scaled = weight_factor * _MAX(MESH_X_DIST, MESH_Y_DIST);

      for (uint8_t jx = 0; jx < GRID_MAX_POINTS_X; jx++)
        for (uint8_t jy = 0; jy < GRID_MAX_POINTS_Y; jy++)
          if (!isnan(z_values[jx][jy]))
            SBI(bitmap[jx], jy);

      xy_pos_t ppos;
      for (uint8_t ix = 0; ix < GRID_MAX_POINTS_X; ix++) {
        ppos.x = mesh_index_to_xpos(ix);
        for (uint8_t iy = 0; iy < GRID_MAX_POINTS_Y; iy++) {
          ppos.y = mesh_index_to_ypos(iy);
          if (isnan(z_values[ix][iy])) {
            // undefined mesh point at (ppos.x,ppos.y), compute weighted LSF from original valid mesh points.
            incremental_LSF_reset(&lsf_results);
            xy_pos_t rpos;
            for (uint8_t jx = 0; jx < GRID_MAX_POINTS_X; jx++) {
              rpos.x = mesh_index_to_xpos(jx);
              for (uint8_t jy = 0; jy < GRID_MAX_POINTS_Y; jy++) {
                if (TEST(bitmap[jx], jy)) {
                  rpos.y = mesh_index_to_ypos(jy);
                  const float rz = z_values[jx][jy],
                               w = 1.0f + weight_scaled / (rpos - ppos).magnitude();
                  incremental_WLSF(&lsf_results, rpos, rz, w);
                }
              }
            }
            if (finish_incremental_LSF(&lsf_results)) {
              SERIAL_ECHOLNPGM("Insufficient data");
              return;
            }
            const float ez = -lsf_results.D - lsf_results.A * ppos.x - lsf_results.B * ppos.y;
            z_values[ix][iy] = ez;
            #if ENABLED(EXTENSIBLE_UI)
              ExtUI::onMeshUpdate(ix, iy, z_values[ix][iy]);
            #endif
            idle(false); // housekeeping
          }
        }
      }

      SERIAL_ECHOLNPGM("done");
    }
  #endif // UBL_G29_P31

  #if ENABLED(UBL_DEVEL_DEBUGGING)
    // #error dead code found by automatic analyses (see BFW-5461)
    /**
     * Much of the 'What?' command can be eliminated. But until we are fully debugged, it is
     * good to have the extra information. Soon... we prune this to just a few items
     */
    void unified_bed_leveling::g29_what_command() {
      report_state();

      if (storage_slot == -1)
        SERIAL_ECHOPGM("No Mesh Loaded.");
      else
        SERIAL_ECHOPAIR("Mesh ", storage_slot, " Loaded.");
      SERIAL_EOL();
      serial_delay(50);

      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_ECHOLNPAIR_F("Fade Height M420 Z", planner.z_fade_height, 4);
      #endif

      adjust_mesh_to_mean(g29_c_flag, g29_constant);

      #if HAS_BED_PROBE
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_ECHOLNPAIR_F("Probe Offset M851 Z", probe_offset.z, 7);
      #endif

      SERIAL_ECHOLNPAIR("MESH_MIN_X  " STRINGIFY(MESH_MIN_X) "=", MESH_MIN_X); serial_delay(50);
      SERIAL_ECHOLNPAIR("MESH_MIN_Y  " STRINGIFY(MESH_MIN_Y) "=", MESH_MIN_Y); serial_delay(50);
      SERIAL_ECHOLNPAIR("MESH_MAX_X  " STRINGIFY(MESH_MAX_X) "=", MESH_MAX_X); serial_delay(50);
      SERIAL_ECHOLNPAIR("MESH_MAX_Y  " STRINGIFY(MESH_MAX_Y) "=", MESH_MAX_Y); serial_delay(50);
      SERIAL_ECHOLNPAIR("GRID_MAX_POINTS_X  ", GRID_MAX_POINTS_X);             serial_delay(50);
      SERIAL_ECHOLNPAIR("GRID_MAX_POINTS_Y  ", GRID_MAX_POINTS_Y);             serial_delay(50);
      SERIAL_ECHOLNPAIR("MESH_X_DIST  ", MESH_X_DIST);
      SERIAL_ECHOLNPAIR("MESH_Y_DIST  ", MESH_Y_DIST);                         serial_delay(50);

      SERIAL_ECHOPGM("X-Axis Mesh Points at: ");
      for (uint8_t i = 0; i < GRID_MAX_POINTS_X; i++) {
        xyz_pos_t pos { 0, 0, 0 };
        pos.x = mesh_index_to_xpos(i);
        SERIAL_ECHO_F(pos.asLogical().x, 3);
        SERIAL_ECHOPGM("  ");
        serial_delay(25);
      }
      SERIAL_EOL();

      SERIAL_ECHOPGM("Y-Axis Mesh Points at: ");
      for (uint8_t i = 0; i < GRID_MAX_POINTS_Y; i++) {
        xyz_pos_t pos { 0, 0, 0 };
        pos.y = mesh_index_to_ypos(i);
        SERIAL_ECHO_F(pos.asLogical().y, 3);
        SERIAL_ECHOPGM("  ");
        serial_delay(25);
      }
      SERIAL_EOL();

      #if HAS_KILL
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_ECHOLNPAIR("Kill pin on :", int(KILL_PIN), "  state:", READ(KILL_PIN));
      #endif
      SERIAL_EOL();
      serial_delay(50);

      #if ENABLED(UBL_DEVEL_DEBUGGING)
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_ECHOLNPAIR("ubl_state_at_invocation :", ubl_state_at_invocation, "\nubl_state_recursion_chk :", ubl_state_recursion_chk);
        serial_delay(50);

        SERIAL_ECHOLNPAIR("Meshes go from ", hex_address((void*)settings.meshes_start_index()), " to ", hex_address((void*)settings.meshes_end_index()));
        serial_delay(50);

        SERIAL_ECHOLNPAIR("sizeof(ubl) :  ", (int)sizeof(ubl));         SERIAL_EOL();
        SERIAL_ECHOLNPAIR("z_value[][] size: ", (int)sizeof(z_values)); SERIAL_EOL();
        serial_delay(25);

        SERIAL_ECHOLNPAIR("EEPROM free for UBL: ", hex_address((void*)(settings.meshes_end_index() - settings.meshes_start_index())));
        serial_delay(50);

        SERIAL_ECHOLNPAIR("EEPROM can hold ", settings.calc_num_meshes(), " meshes.\n");
        serial_delay(25);
      #endif // UBL_DEVEL_DEBUGGING

      if (!sanity_check()) {
        echo_name();
        SERIAL_ECHOLNPGM(" sanity checks passed.");
      }
    }

    /**
     * When we are fully debugged, the EEPROM dump command will get deleted also. But
     * right now, it is good to have the extra information. Soon... we prune this.
     */
    void unified_bed_leveling::g29_eeprom_dump() {
      uint8_t cccc;

      SERIAL_ECHO_MSG("EEPROM Dump:");
      persistentStore.access_start();
      for (uint16_t i = 0; i < persistentStore.capacity(); i += 16) {
        if (!(i & 0x3)) idle(false);
        print_hex_word(i);
        SERIAL_ECHOPGM(": ");
        for (uint16_t j = 0; j < 16; j++) {
          persistentStore.read_data(i + j, &cccc, sizeof(uint8_t));
          print_hex_byte(cccc);
          SERIAL_CHAR(' ');
        }
        SERIAL_EOL();
      }
      SERIAL_EOL();
      persistentStore.access_finish();
    }

    /**
     * When we are fully debugged, this may go away. But there are some valid
     * use cases for the users. So we can wait and see what to do with it.
     */
    void unified_bed_leveling::g29_compare_current_mesh_to_stored_mesh() {
      const int16_t a = settings.calc_num_meshes();

      if (!a) {
        SERIAL_ECHOLNPGM("?EEPROM storage not available.");
        return;
      }

      if (!parser.has_value() || !WITHIN(g29_storage_slot, 0, a - 1)) {
        SERIAL_ECHOLNPAIR("?Invalid storage slot.\n?Use 0 to ", a - 1);
        return;
      }

      g29_storage_slot = parser.value_int();

      float tmp_z_values[GRID_MAX_POINTS_X][GRID_MAX_POINTS_Y];
      settings.load_mesh(g29_storage_slot, &tmp_z_values);

      SERIAL_ECHOLNPAIR("Subtracting mesh in slot ", g29_storage_slot, " from current mesh.");

      for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++)
        for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++) {
          z_values[x][y] -= tmp_z_values[x][y];
          #if ENABLED(EXTENSIBLE_UI)
            // #error dead code found by automatic analyses (see BFW-5461)
            ExtUI::onMeshUpdate(x, y, z_values[x][y]);
          #endif
        }
    }

  #endif // UBL_DEVEL_DEBUGGING

#endif // AUTO_BED_LEVELING_UBL
