#include "PrusaGcodeSuite.hpp"

#include <config_store/store_instance.hpp>
#include <gcode/gcode_parser.hpp>
#include <module/prusa/toolchanger_utils.h>
#include <mapi/feedrates/standard_feedrates.hpp>
#include <raii/scope_guard.hpp>
#include <module/planner.h>
#include <filament.hpp>
#include <tool_index.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 * ### G750: Nozzle cleaner move
 *
 * Linear move used by the nozzle cleaner sequences. X/Y are taken relative to the nozzle cleaner
 * origin (plus the calibrated origin offset) and the move is executed in machine coordinates,
 * bypassing mesh bed leveling (the wastebin lies outside the MBL area).
 *
 * INDX only.
 *
 * #### Usage
 *
 *     G750 [ X ] [ Y ] [ E ] [ F ] [ L ] [ A ]
 *
 * #### Parameters
 *
 * - `X` - Target X position, relative to the nozzle cleaner origin
 * - `Y` - Target Y position, relative to the nozzle cleaner origin
 * - `E` - Relative extrusion amount (added to the current E position)
 * - `F` - Feedrate [mm/s] (defaults to the travel feedrate)
 * - `L` - Adjust the feedrate based on the loaded filament's properties (currently slows down
 *         flexible filaments)
 * - `A` - Asynchronous; do not wait for the planner to finish the move
 *
 */
void PrusaGcodeSuite::G750() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    MachinePosXYZE target = current_machine_position();

    // Unfold the current tool's offset so the nozzle, not the carriage, lands on the cleaner feature.
    xy_float_t current_nozzle_offset = { 0.0f, 0.0f };
#if HAS_HOTEND_OFFSET
    current_nozzle_offset = hotend_currently_applied_offset.xy();
#endif

    if (auto x = p.option<float>('X')) {
        target.x = *x + X_NOZZLE_CLEANER_ORIGIN + config_store().nozzle_cleaner_x_origin_offset.get() - current_nozzle_offset.x;
    }
    if (auto y = p.option<float>('Y')) {
        target.y = *y + Y_NOZZLE_CLEANER_ORIGIN + config_store().nozzle_cleaner_y_origin_offset.get() - current_nozzle_offset.y;
    }
    if (auto e = p.option<float>('E')) {
        target.e += *e;
    }

    float feedrate = p.option<float>('F').value_or(PrusaToolChangerUtils::TRAVEL_MOVE_MM_S);

    // L = adjust the feedrate based on the loaded filament's properties.
    if (p.has_option('L')) {
        feedrate = buddy::adjust_feedrate_for_filament(feedrate, FilamentType::for_tool_heuristic(VirtualToolIndex::currently_selected()));
    }

    // Use machine coordinates - wastebin is outside of MBL area, applying MBL would do funny stuff.
    line_to_machine_pos(target, feedrate, { .ignore_e_factor = true });

    // A = asynchronous - do not wait for the planner to finish the move
    if (!p.has_option('A')) {
        planner.synchronize();
    }
}

/** @}*/
