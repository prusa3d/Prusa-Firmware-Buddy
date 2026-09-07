/**
 * @file M70X.hpp
 * @author Radek Vana
 * @brief Header to access load/unload/preheat states
 * to be used in main thread only!!!
 * @date 2021-11-23
 */

#pragma once

#include <optional>
#include <algorithm>

#include <option/has_chamber_api.h>

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
#endif

#include "config_features.h"
#include <fs_event_autolock.hpp>
#include <feature/prusa/e-stall_detector.h>
#include "fsm_preheat_type.hpp"
#include "preheat_multithread_status.hpp"
#include "filament.hpp"
#include "pause_stubbed.hpp"
#include <color.hpp>
#include <option/has_chamber_api.h>
#include <utils/compact_optional.hpp>
#include <utils/overloaded_visitor.hpp>

namespace filament_gcodes {
using Func = bool (Pause::*)(const pause::Settings &); // member fnc pointer

enum class AskFilament_t {
    Never,
    IfUnknown,
    Always
};

class InProgress {
    static uint lock;

public:
    InProgress() { ++lock; }
    ~InProgress() { --lock; }
    static bool Active() { return lock > 0; }

private:
    FS_EventAutolock fs_lock;
    BlockEStallDetection estall_lock;
};

struct M701LoadArgs {
    /// filament type to load; FilamentType::none to ask the user
    FilamentType filament_to_be_loaded = FilamentType::none;

    /// length of the fast load segment; <= 0 means purge only
    CompactOptional<float, NAN> fast_load_length = std::nullopt;

    /// minimal Z parking position
    float z_min_pos;

    /// preheat mode; nullopt to skip preheating
    std::optional<RetAndCool_t> op_preheat = std::nullopt;

    /// tool to load into
    VirtualToolIndex virtual_tool;

    /// MMU slot to load; -1 if not applicable
    CompactOptional<int8_t, -1> mmu_slot = std::nullopt;

    /// color to load; nullopt if unknown
    CompactOptional<Color, COLOR_NONE> color_to_be_loaded = std::nullopt;

    /// resume print if paused after the load
    bool resume_print_request = false;

#if HAS_ANFC()
    /// If provided, tries to load data from the specified OpenPrintTag
    buddy::openprinttag::ToolTag::UIDHashOptional openprinttag_uid_hash = std::nullopt;
#endif
};

void M701_load(const M701LoadArgs &args);
void M702_unload(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, VirtualToolIndex virtual_tool, bool ask_unloaded);
void M70X_process_user_response(PreheatStatus::Result res, VirtualToolIndex target_extruder);

void M1600_change_filament(FilamentType filament_to_be_loaded, VirtualToolIndex virtual_tool, RetAndCool_t preheat, AskFilament_t ask_filament, std::optional<Color> color_to_be_loaded);

struct M1700Args {
    /// include return and/or cooldown items in menu
    RetAndCool_t preheat;

    /// preheat mode as part of load/unload
    PreheatMode mode;

    /// preheat this tool
    std::variant<VirtualToolIndex, AllTools> tool;

    /// save selected filament settings to EEPROM
    bool save : 1;

    /// true to enforce target temp, false to use preheat temp
    bool enforce_target_temp : 1;

    /// true to also heat up bed
    bool preheat_bed : 1;

#if HAS_CHAMBER_API()
    /// Whether to set target chamber temperature
    bool preheat_chamber : 1;
#endif

#if HAS_FILAMENT_HEATBREAK_PARAM()
    /// Whether to set target heatbreak temperature
    bool set_heatbreak : 1;
#endif
};

/// Standalone preheat
void M1700_preheat(const M1700Args &args);

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
void M1701_autoload(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder);

void mmu_load(uint8_t data);
void mmu_load_test(VirtualToolIndex slot);
void mmu_eject(uint8_t data);
void mmu_cut(uint8_t data);

void mmu_reset(uint8_t level);
void mmu_on();
void mmu_off();

struct FilamentSelectionArgs {
    using ToolIndex = PreheatData::ToolIndex;

    PreheatMode mode;
    ToolIndex tool;
    RetAndCool_t ret_cool;

#if HAS_ANFC()
    /// If provided, try to load data from the specified tag
    buddy::openprinttag::ToolTag::UIDHashOptional openprinttag_uid_hash = std::nullopt;
#endif

    /// Ask the user for filament selection even if it could be deduced from the currently loaded filament
    bool disregard_loaded_filament : 1 = false;

    PreheatData fsm_data() const;
};

/// This set of flags controls the behavior of preheating.
struct PreheatBehavior {
    bool force_temp : 1; ///< If false, the hotend and bed temperatures will not be decreased if the new target temperatures are lower than the current ones.
    bool preheat_bed : 1; ///< true -> heat up bed as well (usual case), false -> do not preheat bed (e.g. for unloading filament)
#if HAS_CHAMBER_API()
    bool set_chamber_temperature : 1; ///< true -> heat up chamber as well, false otherwise
#endif

    /// If true, preheats to max(selected_filament_type, previous_filament_type)
    bool consider_previous_filament : 1;

    /// @returns preheat behavior for loads during filament change
    static PreheatBehavior for_filament_load(bool force_temp = true);

    /// @returns preheat behavior for unloads during filament change
    static PreheatBehavior for_filament_unload(bool force_temp = true);
};

std::pair<std::optional<PreheatStatus::Result>, FilamentType> preheat(const FilamentSelectionArgs &selection_args, PreheatBehavior preheat_arg);
void preheat_to(FilamentType filament, std::variant<PhysicalToolIndex, AllTools> tools, PreheatBehavior preheat_arg);

} // namespace filament_gcodes

namespace PreheatStatus {
void SetResult(Result res);
} // namespace PreheatStatus
