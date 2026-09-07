#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include <raii/auto_restore.hpp>
#include "M70X.hpp"
#include "fs_event_autolock.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/feature/prusa/e-stall_detector.h"
#include "pause_stubbed.hpp"
#include "pause_settings.hpp"
#include <option/has_mmu2.h>
#include <utils/variant_utils.hpp>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

using namespace filament_gcodes;

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M701: Load filament <a href="https://reprap.org/wiki/G-code#M701:_Load_filament">M701: Load filament</a>
 *
 *#### Usage
 *
 *    M701 [ T | Z | L | S | P | W | O | R | G ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number
 * - `Z` - Minimal Z parking position
 * - `L` - Extrude distance for insertion (positive value)
 *   - `0` - PURGE
 *
 * - `S"Filament"` - save filament by name, for example S"PLA". RepRap compatible.
 * - `G` - Use filament data from an OpenPrintTag with the specific UID hash
 *
 * - `P<mmu>` - MMU index of slot (zero based)
 * - `W<value>` - Preheat
 *   - `W255` - default without preheat
 *   - `W0` - preheat no return no cool down
 *   - `W1` - preheat with cool down option
 *   - `W2` - preheat with return option
 *   - `W3` - preheat with cool down and return options
 * - `O<value>` - Color number corresponding to Color, RGB order
 * - `R` - resume print if paused
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const std::optional<int8_t> mmu_slot = p.option<int8_t>('P', int8_t(0), int8_t(VirtualToolIndex::count - 1));

#if HAS_MMU2()
    // HACK: The MMU case is _little bit_ wrong.
    //
    // On all our current MMU printers, we have single, always present head. If
    // we ever get something like XL with 5*MMU, this will not work, but the
    // M701_load should be changed to take physical tool, not virtual - that
    // is, we would want to say something like "load 3rd slot of 2nd tool".
    //
    // The MMU case doesn't currently use 'T' for tool (extruder, there's just
    // one). It uses 'P' for the slot. It is not even clear if on XL+5*MMU, the
    // case would be something like T2 P3 or T2 P13 (eg. 2*5+3) or just P13..
    static_assert(PhysicalToolIndex::count == 1);
    const bool mmu_enabled = MMU2::mmu2.Enabled();
#else
    constexpr bool mmu_enabled = false;
#endif

    std::optional<VirtualToolIndex> virtual_tool;
    if (mmu_enabled) {
        if (mmu_slot.has_value()) {
            virtual_tool = VirtualToolIndex::from_raw(*mmu_slot);
        }
    } else {
        virtual_tool = stdext::get_optional<VirtualToolIndex>(PrusaGcodeSuite::get_target_virtual_from_command(p));
    }

    if (!virtual_tool.has_value()) {
        return;
    }

    M701LoadArgs args {
        .filament_to_be_loaded = p.option<FilamentType>('S').value_or(NoFilamentType()),
        .fast_load_length = p.option<float>('L').transform(fabsf),
        .z_min_pos = p.option<float>('Z').value_or(Z_AXIS_LOAD_POS),
        .op_preheat = p.option<RetAndCool_t>('W', std::to_underlying(RetAndCool_t::last_) + 1),
        .virtual_tool = *virtual_tool,
        .mmu_slot = mmu_slot,
        .color_to_be_loaded = p.option<Color>('O'),
        .resume_print_request = p.option<bool>('R').value_or(false),
#if HAS_ANFC()
        .openprinttag_uid_hash = p.option<uint16_t>('G'),
#endif
    };

    M701_load(args);
}

/**
 *### M702: Unload filament <a href="https://reprap.org/wiki/G-code#M702:_Unload_filament">M702: Unload filament</a>
 *
 *#### Usage
 *
 *    M702 [ T | Z | U | W | I ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number
 * - `Z` - Minimal Z parking position
 * - `U` - Retract distance for removal (manual reload)
 * - `W` - Preheat
 *   - `255` - default without preheat
 *   - `0` - preheat no return no cool down
 *   - `1` - preheat with cool down option
 *   - `2` - preheat with return option
 *   - `3` - preheat with cool down and return options
 * - `I` - ask successful unload
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    const std::optional<float> unload_len = parser.seen('U') ? std::optional<float>(parser.value_axis_units(E_AXIS)) : std::nullopt;
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    const uint8_t preheat = parser.byteval('W', 255);
    const bool ask_unloaded = parser.seen('I');

    GcodeSuite::VirtualToolFromCommand target_result = GcodeSuite::get_target_virtual_from_command();

#if HAS_MMU2()
    // On MMU, allow unload even with no filament loaded and none specified.
    //
    // This allows clearing mismatches between firmware state and physical
    // reality, partially loaded filaments, etc.
    //
    // For that, we just use an arbitrary dummy index.
    if (std::holds_alternative<NoTool>(target_result)) {
        // If we have more, we need to think better on which virtual tool we
        // unload. One of the currently picked physical one would be good.
        static_assert(PhysicalToolIndex::count == 1);
        target_result = VirtualToolIndex::from_raw(0);
    }
#endif

    // FIXME: Should this actually be _physical_ one?
    //
    // If we imagine an XL+5*MMU printer kind of thing, with M701 above would
    // `M701 T2 P3` probably mean something like "load slot 3 on the 2 head".
    //
    // Similarly, `M702 T2` should then mean "Unload whatever is loaded in the
    // 2 head".
    //
    // But for now, the actual implementations take Virtual indexes.
    const std::optional<VirtualToolIndex> target = stdext::get_optional<VirtualToolIndex>(target_result);
    if (!target.has_value()) {
        return;
    }

    std::optional<RetAndCool_t> op_preheat = std::nullopt;
    if (preheat <= uint8_t(RetAndCool_t::last_)) {
        op_preheat = RetAndCool_t(preheat);
    }

    M702_unload(unload_len, min_Z_pos, op_preheat, *target, ask_unloaded);
}
/** @}*/
