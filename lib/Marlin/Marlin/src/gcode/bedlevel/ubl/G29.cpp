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

    #include <marlin_server.hpp>
    #include <calibration_z.hpp>
    #include <option/has_uneven_bed_prompt.h>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G29: Unified Bed Leveling <a href="https://reprap.org/wiki/G-code#G29_Unified_Bed_Leveling_.28Marlin_-_MK4duo.29">G29 Unified Bed Leveling (Marlin - MK4duo)</a>
 *
 *#### Usage
 *
 *    G29 [ A | B | C | D | E | F | H | I | J | L | P | Q | R | S | T | U | X | Y | | K | Q | W ]
 *
 *#### Parameters
 *
 * - `A` - Activate the Unified Bed Leveling system.
 * - `B` - Use the 'Business Card' mode of the Manual Probe subsystem with P2.
 *                    Note: A non-compressible Spark Gap feeler gauge is recommended over a business card.
 *                    In this mode of G29 P2, a business or index card is used as a shim that the nozzle can
 *                    grab onto as it is lowered. In principle, the nozzle-bed distance is the same when the
 *                    same resistance is felt in the shim. You can omit the numerical value on first invocation
 *                    of G29 P2 B to measure shim thickness. Subsequent use of 'B' will apply the previously-
 *                    measured thickness by default.
 * - `C` - uses the Current location (instead of bed center or nearest edge).
 *   - `C P1` - continues the generation of a partially-constructed Mesh without invalidating previous measurements.
 *   - `C P2` - tells the Manual Probe subsystem to not use the current nozzle
 *              location in its search for the closest unmeasured Mesh Point. Instead, attempt to
 *              start at one end of the uprobed points and Continue sequentially.
 *   - `C P3` - specifies the Constant for the fill. Otherwise, uses a "reasonable" value.
 * - `D` - Disable the Unified Bed Leveling system.
 * - `E` - Stow_probe Stow the probe after each sampled point.
 * - `F` - Fade the amount of Mesh Based Compensation over a specified height. At the
 *         specified height, no correction is applied and natural printer kenimatics take over. If no
 *         number is specified for the command, 10mm is assumed to be reasonable.
 * - `H` - Height
 *   - `H P2` - specify the Height to raise the nozzle after each manual probe of the bed.
 *              If omitted, the nozzle will raise by Z_CLEARANCE_BETWEEN_PROBES.
 *   - `H P4` - specify the Offset above the mesh height to place the nozzle.
 *              If omitted, Z_CLEARANCE_BETWEEN_PROBES will be used.
 * - `I` - Invalidate the specified number of Mesh Points near the given 'X' 'Y'. If X or Y are omitted,
 *         the nozzle location is used. If no 'I' value is given, only the point nearest to the location is
 *         invalidated. If a negative value is given, invalidate the entire mesh. Use 'T' to produce a map
 *         afterward. This command is useful to invalidate a portion of the Mesh so it can be adjusted
 *         using other UBL tools. When attempting to invalidate an isolated bad mesh point, the 'T' option
 *         shows the nozzle position in the Mesh with (#). You can move the nozzle around and use this
 *         feature to select the center of the area (or cell) to invalidate.
 * - `J` - Perform a Grid Based Leveling of the current Mesh using a grid with n points on a side.
 *         Not specifying a grid size will invoke the 3-Point leveling function.
 * - `L` - Load Mesh from the previously activated location in the EEPROM.
 *
 * - `L#` - Load Mesh from the specified location in the EEPROM. Set this location as activated
 *          for subsequent Load and Store operations.
 *
 *   The P or Phase commands are used for the bulk of the work to setup a Mesh. In general, your Mesh will
 *   start off being initialized with a G29 P0 or a G29 P1. Further refinement of the Mesh happens with
 *   each additional Phase that processes it.
 *
 * - `P` - Phase
 *   - `0` - Zero Mesh Data and turn off the Mesh Compensation System. This reverts the
 *           3D Printer to the same state it was in before the Unified Bed Leveling Compensation
 *           was turned on. Setting the entire Mesh to Zero is a special case that allows
 *           a subsequent G or T leveling operation for backward compatibility.
 *   - `1` - Invalidate entire Mesh and continue with automatic generation of the Mesh data using
 *           the Z-Probe. Usually the probe can't reach all areas that the nozzle can reach. For delta
 *           printers only the areas where the probe and nozzle can both reach will be automatically probed.
 *
 *           Unreachable points will be handled in Phase 2 and Phase 3.
 *
 *           Use 'C' to leave the previous mesh intact and automatically probe needed points. This allows you
 *           to invalidate parts of the Mesh but still use Automatic Probing.
 *
 *           The 'X' and 'Y' parameters prioritize where to try and measure points. If omitted, the current
 *           probe position is used.
 *
 *           Use 'T' (Topology) to generate a report of mesh generation.
 *
 *           P1 will suspend Mesh generation if the controller button is held down. Note that you may need
 *           to press and hold the switch for several seconds if moves are underway.
 *
 *   - `2` - Probe unreachable points.
 *           Use 'H' to set the height between Mesh points. If omitted, Z_CLEARANCE_BETWEEN_PROBES is used.
 *           Smaller values will be quicker. Move the nozzle down till it barely touches the bed. Make sure the
 *           nozzle is clean and unobstructed. Use caution and move slowly. This can damage your printer!
 *           (Uses SIZE_OF_LITTLE_RAISE mm if the nozzle is moving less than BIG_RAISE_NOT_NEEDED mm.)
 *
 *           The 'H' value can be negative if the Mesh dips in a large area. Press and hold the
 *           controller button to terminate the current Phase 2 command. You can then re-issue "G29 P 2"
 *           with an 'H' parameter more suitable for the area you're manually probing. Note that the command
 *           tries to start in a corner of the bed where movement will be predictable. Override the distance
 *           calculation location with the X and Y parameters. You can print a Mesh Map (G29 T) to see where
 *           the mesh is invalidated and where the nozzle needs to move to complete the command. Use 'C' to
 *           indicate that the search should be based on the current position.
 *
 *           The 'B' parameter for this command is described above. It places the manual probe subsystem into
 *           Business Card mode where the thickness of a business card is measured and then used to accurately
 *           set the nozzle height in all manual probing for the duration of the command. A Business card can
 *           be used, but you'll get better results with a flexible Shim that doesn't compress. This makes it
 *           easier to produce similar amounts of force and get more accurate measurements. Google if you're
 *           not sure how to use a shim.
 *
 *           The 'T' (Map) parameter helps track Mesh building progress.
 *
 *           NOTE: P2 requires an LCD controller!
 *
 *   - `3` - Fill the unpopulated regions of the Mesh with a fixed value. There are two different paths to
 *           go down:
 *
 *           - If a 'C' constant is specified, the closest invalid mesh points to the nozzle will be filled,
 *             and a repeat count can then also be specified with 'R'. If a negative count is given,
 *             the entire mesh is filled.
 *
 *           - Leaving out 'C' invokes Smart Fill, which scans the mesh from the edges inward looking for
 *             invalid mesh points. Adjacent points are used to determine the bed slope. If the bed is sloped
 *             upward from the invalid point, it takes the value of the nearest point. If sloped downward, it's
 *             replaced by a value that puts all three points in a line. This version of G29 P3 is a quick, easy
 *             and (usually) safe way to populate unprobed mesh regions before continuing to G26 Mesh Validation
 *             Pattern. Note that this populates the mesh with unverified values. Pay attention and use caution.
 *
 *   - `4` - Fine tune the Mesh. The Delta Mesh Compensation System assumes the existence of
 *           an LCD Panel. It is possible to fine tune the mesh without an LCD Panel using
 *           G42 and M421. See the UBL documentation for further details.
 *
 *           Phase 4 is meant to be used with G26 Mesh Validation to fine tune the mesh by direct editing
 *           of Mesh Points. Raise and lower points to fine tune the mesh until it gives consistently reliable
 *           adhesion.
 *
 *           P4 moves to the closest Mesh Point (and/or the given X Y), raises the nozzle above the mesh height
 *           by the given 'H' offset (or default 0), and waits while the controller is used to adjust the nozzle
 *           height. On click the displayed height is saved in the mesh.
 *
 *           Start Phase 4 at a specific location with X and Y. Adjust a specific number of Mesh Points with
 *           the 'R' (Repeat) parameter. (If 'R' is left out, the whole matrix is assumed.) This command can be
 *           terminated early (e.g., after editing the area of interest) by pressing and holding the encoder button.
 *
 *           The general form is G29 P4 [R points] [X position] [Y position]
 *
 *           The H [offset] parameter is useful if a shim is used to fine-tune the mesh. For a 0.4mm shim the
 *           command would be G29 P4 H0.4. The nozzle is moved to the shim height, you adjust height to the shim,
 *           and on click the height minus the shim thickness will be saved in the mesh.
 *
 *           !!Use with caution, as a very poor mesh could cause the nozzle to crash into the bed!!
 *
 *           NOTE:  P4 is not available unless you have LCD support enabled!
 *
 *   - `5` - Find Mean Mesh Height and Standard Deviation. Typically, it is easier to use and
 *                    work with the Mesh if it is Mean Adjusted. You can specify a C parameter to
 *                    Correct the Mesh to a 0.00 Mean Height. Adding a C parameter will automatically
 *                    execute a G29 P6 C <mean height>.
 *
 *   - `6` - Shift Mesh height. The entire Mesh's height is adjusted by the height specified
 *                    with the C parameter. Being able to adjust the height of a Mesh is useful tool. It
 *                    can be used to compensate for poorly calibrated Z-Probes and other errors. Ideally,
 *                    you should have the Mesh adjusted for a Mean Height of 0.00 and the Z-Probe measuring
 *                    0.000 at the Z Home location.
 *
 * - `Q` - Load specified Test Pattern to assist in checking correct operation of system. This
 *         command is not anticipated to be of much value to the typical user. It is intended
 *         for developers to help them verify correct operation of the Unified Bed Leveling System.
 *
 * - `R` - Repeat this command the specified number of times. If no number is specified the
 *         command will be repeated GRID_MAX_POINTS_X * GRID_MAX_POINTS_Y times.
 *
 * - `S` - Store the current Mesh in the Activated area of the EEPROM. It will also store the
 *         current state of the Unified Bed Leveling system in the EEPROM.
 *
 *   - `S#` - Store the current Mesh at the specified location in EEPROM. Activate this location
 *            for subsequent Load and Store operations. Valid storage slot numbers begin at 0 and
 *            extend to a limit related to the available EEPROM storage.
 *
 *   - `S-1` - Print the current Mesh as G-code that can be used to restore the mesh anytime.
 *
 * - `T` - Display the Mesh Map Topology.
 *         'T' can be used alone (e.g., G29 T) or in combination with most of the other commands.
 *         This option works with all Phase commands (e.g., G29 P4 R 5 T X 50 Y100 C -.1 O)
 *         This parameter can also specify a Map Type. T0 (the default) is user-readable. T1
 *         is suitable to paste into a spreadsheet for a 3D graph of the mesh.
 *
 * - `U` - Perform a probe of the outer perimeter to assist in physically leveling unlevel beds.
 *         Only used for G29 P1 T U. This speeds up the probing of the edge of the bed. Useful
 *         when the entire bed doesn't need to be probed because it will be adjusted.
 *
 * - `V` - Set the verbosity level (0-4) for extra details. (Default 0)
 *
 * - `X` - X Location for this command
 *
 * - `Y` - Y Location for this command
 *
 * With UBL_DEVEL_DEBUGGING:
 *
 * - `K` - Kompare current Mesh with stored Mesh #, replacing current Mesh with the result.
 *         This command literally performs a diff between two Meshes.
 *
 * - `Q-1` -  Dump EEPROM Dump the UBL contents stored in EEPROM as HEX format. Useful for developers to help
 *            verify correct operation of the UBL.
 *
 * - `W` - Display valuable UBL data.
 *
 *   Release Notes:
 *   You MUST do M502, M500 to initialize the storage. Failure to do this will cause all
 *   kinds of problems. Enabling EEPROM Storage is required.
 *
 *   When you do a G28 and G29 P1 to automatically build your first mesh, you are going to notice that
 *   UBL probes points increasingly further from the starting location. (The starting location defaults
 *   to the center of the bed.) In contrast, ABL and MBL follow a zigzag pattern. The spiral pattern is
 *   especially better for Delta printers, since it populates the center of the mesh first, allowing for
 *   a quicker test print to verify settings. You don't need to populate the entire mesh to use it.
 *   After all, you don't want to spend a lot of time generating a mesh only to realize the resolution
 *   or probe offsets are incorrect. Mesh-generation gathers points starting closest to the nozzle unless
 *   an (X,Y) coordinate pair is given.
 *
 *   Unified Bed Leveling uses a lot of EEPROM storage to hold its data, and it takes some effort to get
 *   the mesh just right. To prevent this valuable data from being destroyed as the EEPROM structure
 *   evolves, UBL stores all mesh data at the end of EEPROM.
 *
 *   UBL is founded on Edward Patel's Mesh Bed Leveling code. A big 'Thanks!' to him and the creators of
 *   3-Point and Grid Based leveling. Combining their contributions we now have the functionality and
 *   features of all three systems combined.
 */
void GcodeSuite::G29() {
    marlin_server::FSM_Holder fsm_holder(PhaseWait::generic);

    BlockEStallDetection block_e_stall_detection;

    while (true) {
        ubl.g29_min_max_measured_z = std::nullopt;
        ubl.G29();

    #if HAS_UNEVEN_BED_PROMPT()
        // If we've done some measuring in this phase and it is too uneven, offer running Z calib
        if (
            ubl.g29_min_max_measured_z.has_value()
            && ubl.g29_min_max_measured_z->second - ubl.g29_min_max_measured_z->first >= MBL_Z_DIFF_CALIB_WARNING_THRESHOLD

            // Hack for the supplemenary "probe near purge place" - that is done after print area MBL and we don't want to offer Z align after that
            && !parser.seenval('C') //
        ) {
            log_warning(Marlin, "Uneven bet detected: %f - %f", (double) ubl.g29_min_max_measured_z->first, (double)ubl.g29_min_max_measured_z->second);

            marlin_server::set_warning(WarningType::BedUnevenAlignmentPrompt);
            const Response response = marlin_server::wait_for_response(warning_type_phase(WarningType::BedUnevenAlignmentPrompt));
            marlin_server::clear_warning(WarningType::BedUnevenAlignmentPrompt);

            if (response == Response::Yes) {
                // calib_Z does not have its own holder - we have to handle that
                marlin_server::FSM_Holder _fsm(PhasesSelftest::CalibZ);
                selftest::calib_Z(true);
                assert(!TEST(axis_homed, Z_AXIS));
                continue;
            }
        }
    #elif defined(MBL_Z_DIFF_CALIB_WARNING_THRESHOLD)
        #error "MBL_Z_DIFF_CALIB_WARNING_THRESHOLD defined but HAS_UNEVEN_BED_PROMPT is false"
    #endif

        break;
    }
}

/** @}*/

#endif // AUTO_BED_LEVELING_UBL
