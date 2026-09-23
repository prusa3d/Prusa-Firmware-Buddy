/// @file
#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"
#include <utils/variant_utils.hpp>
#include <raii/scope_guard.hpp>
#include <filament_to_load.hpp>
#include <option/has_bowden.h>
#include <option/has_indx.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

using namespace filament_gcodes;

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1700: Preheat
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M1700 [ T | W | S | E | B0 | C0 | H0 ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *       For non-mixing, current extruder if omitted.
 *   - `T-1` - all extruders
 * - `W` - Preheat
 *   - `W0` - preheat no return no cool down
 *   - `W1` - preheat with cool down option
 *   - `W2` - preheat with return option
 *   - `W3` - preheat with cool down and return options - default
 * - `S` - Set filament
 * - `E` - Enforce target temperature
 * - `B0` - Do not preheat the bed
 * - `C0` - Do not set chamber temperature
 * - `H0` - Do not set heatbreak temperature
 */
void PrusaGcodeSuite::M1700() {
    const uint8_t preheat = std::min(parser.byteval('W', 3), uint8_t(RetAndCool_t::last_));

    std::variant<VirtualToolIndex, AllTools> tool = AllTools {};
    if (parser.intval('T') == -1) {
        tool = AllTools {}; // -1 means all extruders
    } else {
        const std::optional<VirtualToolIndex> vt = stdext::get_optional<VirtualToolIndex>(GcodeSuite::get_target_virtual_from_command()); // Get particular extruder or current extruder
        if (vt.has_value()) {
            tool = *vt;
        }
    }

    filament_gcodes::M1700_preheat(filament_gcodes::M1700Args {
        .preheat = RetAndCool_t(preheat),
        .mode = PreheatMode::preheat,
        .tool = tool,
        .save = parser.boolval('S'),
        .enforce_target_temp = parser.boolval('E'),
        .preheat_bed = parser.boolval('B', true),
#if HAS_CHAMBER_API()
        .preheat_chamber = parser.boolval('C', true),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
        .set_heatbreak = parser.boolval('H', true),
#endif
    });
}

/**
 *### M1701: Autoload
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M1701 [ T ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *        For non-mixing, current extruder if omitted.
 * - `G` - Use filament data from an OpenPrintTag with the specific UID hash (internal parameter)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M1701() {
    constexpr auto fast_load_length = FILAMENT_CHANGE_FAST_LOAD_LENGTH;
    constexpr auto z_min_pos = Z_AXIS_LOAD_POS;

    // FIXME:
    // It's a bit unclear if the target_extruder here shall be virtual or physical.
    //
    // * It is triggered by a filament sensor on a physical head (that is, in case
    //   of MMU printer, there's single filament sensor, but 5 slots), which would
    //   suggest physical.
    // * We do _not_ set the T parameter on trigger, and trigger it only in case we
    //   have the specific head selected (on XL), ignoring for others.
    // * We use the parameter for things like filament properties, which filament
    //   to load, etc, which suggest virtual.
    // * We take a physical tool to load into by independently using
    //   PhysicalToolIndex::currently_selected, which seems completely wrong in
    //   principle.
    const std::optional<VirtualToolIndex> virtual_tool = stdext::get_optional<VirtualToolIndex>(GcodeSuite::get_target_virtual_from_command());
    if (!virtual_tool.has_value()) {
        return;
    }

#if HAS_ANFC()
    buddy::openprinttag::ToolTag::UIDHashOptional tool_tag_hash;
    const auto ephemeral_tag = buddy::openprinttag::ToolTag::for_tool_ephemeral(*virtual_tool);
    if (ephemeral_tag.has_value() && parser.seenval('G') && parser.value_ushort() == ephemeral_tag->uid_hash()) {
        static_assert(sizeof(buddy::openprinttag::ToolTag::UIDHash) == 2, "value_ushort");
        tool_tag_hash = ephemeral_tag->uid_hash();
    }
#endif

    filament::set_type_to_load(FilamentType::none);

    InProgress progress;
    ScopeGuard autoload_clr = [&] {
        FSensors_instance().ClrAutoloadSent();
    };

    if constexpr (option::has_bowden) {
        config_store().set_filament_type(*virtual_tool, FilamentType::none);
        M701_load(M701LoadArgs {
            .fast_load_length = fast_load_length,
            .z_min_pos = z_min_pos,
            .op_preheat = RetAndCool_t::Return,
            .virtual_tool = *virtual_tool,
#if HAS_ANFC()
            .openprinttag_uid_hash = tool_tag_hash,
#endif
        });
        return;
    }

#if HAS_INDX()
    // Park the tool back to its dock after autoload (covers all early-return paths)
    ScopeGuard park_after_autoload = [&] {
        tool_change(NoTool {}, tool_return_t::dock_backoff);
    };
#endif

    pause::Settings settings;
    settings.SetExtruder(*virtual_tool);
    settings.SetFastLoadLength(fast_load_length);
    settings.SetRetractLength(0.f);
    float e_pos_to_restore = current_position.e;
    mapi::ParkingPosition pos = mapi::get_parking_position(mapi::ParkPosition::load);
    pos.z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = z_min_pos };
    settings.SetParkPoint(pos);

    auto active_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!active_tool.has_value()) {
        bsod("Autoload to notool");
    }

    const uint16_t orig_temp = Temperature::degTargetHotend(*active_tool);

    ScopeGuard fail_guard = [&] {
        thermalManager.setTargetHotend(orig_temp, *active_tool);
        PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);
    };

    auto unload_filament = [&](Pause::LoadType unload_type) {
        Pause::Instance().perform(unload_type, settings);
    };

    if (orig_temp < EXTRUDE_MINTEMP) {
        thermalManager.setTargetHotend(EXTRUDE_MINTEMP, *active_tool);
    }

    const auto no_filament_in_extruder = [&]() {
        return FSensors_instance().no_filament_surely(LogicalFilamentSensor::extruder)
            || (!FSensors_instance().sensor(LogicalFilamentSensor::extruder) && FSensors_instance().no_filament_surely(LogicalFilamentSensor::side));
    };

    // catch filament in gear and then ask for temp
    if (!Pause::Instance().perform(Pause::LoadType::load_to_gears, settings) && !no_filament_in_extruder()) {
        // Unload when user said stop and filament was already loaded
        unload_filament(Pause::LoadType::unload_from_gears);
        return;
    }
    // check if filament is in gears before continuing to preheat
    if (no_filament_in_extruder()) {
        return;
    }

    if constexpr (option::has_human_interactions) {
        const FilamentSelectionArgs data {
            .mode = PreheatMode::autoload,
            .tool = *virtual_tool,
            .ret_cool = RetAndCool_t::Return,
#if HAS_ANFC()
            .openprinttag_uid_hash = tool_tag_hash,
#endif
        };
        auto preheat_ret = preheat(data, PreheatBehavior::for_filament_load());

        if (preheat_ret.first) {
            // canceled
            unload_filament(Pause::LoadType::unload_from_gears);
            return;
        }

        const FilamentType filament = preheat_ret.second;
        filament::set_type_to_load(filament);

        mapi::ParkingPosition park_position = { .z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = z_min_pos } };
        // Returning to previous position is unwanted outside of printing (M1701 should be used only outside of printing)
        settings.SetParkPoint(park_position);

        if (!Pause::Instance().perform(Pause::LoadType::autoload, settings)) {
            // This is a bit problematic, since we dont know how far the autoload has gotten (if only waiting for preheat or already loading to nozzle) -> therefore we have to always do the full unload even if it was stoped during wait_temp (where only unload from gears would suffice)
            // This could possibly be solved if we move preheating and the whole autoload process into pause (so that is wouldn't be seperated to two operations (load_to_gears and autoload)) and then we could tell apart when the autoload was stopped
            unload_filament(Pause::LoadType::unload);
            return;
        }
    }
    sync_e_position_to(e_pos_to_restore);
    destination.e = e_pos_to_restore;

    // at this point autoload is considered successful so fail guard is not to be triggered and we report DoneHasFilament as status
    fail_guard.disarm();
    PreheatStatus::SetResult(PreheatStatus::Result::DoneHasFilament);
}

/**
 *### M1600: non-print filament change <a href=" "> </a>
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M1600 [ T | R | U | S | O ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 * - `R` -  Preheat Return option
 * - `U` - Ask Unload type
 *   - `U0` - return if filament unknown (default)
 *   - `U1` - ask only if filament unknown
 *   - `U2` - always ask
 * - `S"Filament"` - change to filament by name, for example `S"PLA"`
 * - `O<value>` - Color number corresponding to Color, RGB order
 * - `P` - If set, the parameter 'T' is interpreted as a VirtualToolIndex (tool mapping is not applied)
 */
void PrusaGcodeSuite::M1600() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const std::optional<VirtualToolIndex> virtual_tool = stdext::get_optional<VirtualToolIndex>(PrusaGcodeSuite::get_target_virtual_from_command_p(p));
    if (!virtual_tool.has_value()) {
        return;
    }

    const FilamentType filament_to_be_loaded = p.option<FilamentType>('S').value_or(NoFilamentType());
    std::optional<Color> color_to_be_loaded = p.option<Color>('O');
    const filament_gcodes::AskFilament_t ask_unload = filament_gcodes::AskFilament_t(p.option<int>('U').value_or(0));
    const bool hasReturn = p.option<bool>('R').value_or(false);

    filament_gcodes::M1600_change_filament(filament_to_be_loaded, *virtual_tool, hasReturn ? RetAndCool_t::Return : RetAndCool_t::Neither, ask_unload, color_to_be_loaded);
}

/** @}*/
