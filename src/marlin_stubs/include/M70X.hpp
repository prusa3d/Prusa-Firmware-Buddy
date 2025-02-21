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

#include "config_features.h"
#include <fs_event_autolock.hpp>
#include <feature/prusa/e-stall_detector.h>
#include "fsm_preheat_type.hpp"
#include "preheat_multithread_status.hpp"
#include "filament.hpp"
#include "pause_stubbed.hpp"
#include <color.hpp>
#include <option/has_chamber_api.h>

namespace filament_gcodes {
using Func = bool (Pause::*)(const pause::Settings &); // member fnc pointer

enum class AskFilament_t {
    Never,
    IfUnknown,
    Always
};

enum class ResumePrint_t : bool {
    No = false,
    Yes,
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

void M701_no_parser(FilamentType filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot, std::optional<Color> color_to_be_loaded, ResumePrint_t resume_print_request);
void M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded);
void M70X_process_user_response(PreheatStatus::Result res, uint8_t target_extruder);

void M1600_no_parser(FilamentType filament_to_be_loaded, uint8_t target_extruder, RetAndCool_t preheat, AskFilament_t ask_filament, std::optional<Color> color_to_be_loaded);

struct M1700Args {
    /// include return and/or cooldown items in menu
    RetAndCool_t preheat;

    /// preheat mode as part of load/unload
    PreheatMode mode;

    /// preheat this extruder (indexed from 0), or -1 to preheat all
    int8_t target_extruder;

    /// save selected filament settings to EEPROM
    bool save;

    /// true to enforce target temp, false to use preheat temp
    bool enforce_target_temp;

    /// true to also heat up bed
    bool preheat_bed;

#if HAS_CHAMBER_API()
    /// Whether to set target chamber temperature
    bool preheat_chamber;
#endif
};

/// Standalone preheat
void M1700_no_parser(const M1700Args &args);

void M1701_no_parser(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder);

void mmu_load(uint8_t data);
void mmu_load_test(uint8_t data);
void mmu_eject(uint8_t data);
void mmu_cut(uint8_t data);

void mmu_reset(uint8_t level);
void mmu_on();
void mmu_off();

/// This set of flags controls the behavior of preheating.
struct PreheatBehavior {
    bool force_temp : 1; ///< If false, the hotend and bed temperatures will not be decreased if the new target temperatures are lower than the current ones.
    bool preheat_bed : 1; ///< true -> heat up bed as well (usual case), false -> do not preheat bed (e.g. for unloading filament)
#if HAS_CHAMBER_API()
    bool set_chamber_temperature : 1; ///< true -> heat up chamber as well, false otherwise
#endif
    static constexpr PreheatBehavior force_preheat_bed_and_chamber(bool cf) {
        return PreheatBehavior {
            .force_temp = true,
            .preheat_bed = cf
#if HAS_CHAMBER_API()
                ,
            .set_chamber_temperature = cf
#endif
        };
    }
    static constexpr PreheatBehavior no_force_preheat_bed_and_chamber(bool cf) {
        return PreheatBehavior {
            .force_temp = false,
            .preheat_bed = cf
#if HAS_CHAMBER_API()
                ,
            .set_chamber_temperature = cf
#endif
        };
    }
    static constexpr PreheatBehavior force_preheat_only_extruder() {
        return PreheatBehavior {
            .force_temp = true,
            .preheat_bed = false
#if HAS_CHAMBER_API()
                ,
            .set_chamber_temperature = false
#endif
        };
    }
};

std::pair<std::optional<PreheatStatus::Result>, FilamentType> preheat(PreheatData preheat_data, uint8_t target_extruder, PreheatBehavior preheat_arg);
std::pair<std::optional<PreheatStatus::Result>, FilamentType> preheat_for_change_load(PreheatData data, uint8_t target_extruder);
void preheat_to(FilamentType filament, uint8_t target_extruder, PreheatBehavior preheat_arg);
} // namespace filament_gcodes

namespace PreheatStatus {
void SetResult(Result res);
} // namespace PreheatStatus
