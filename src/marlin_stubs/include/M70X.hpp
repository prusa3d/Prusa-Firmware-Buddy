/**
 * @file M70X.hpp
 * @author Radek Vana
 * @brief Header to access load/unload/preheat states
 * to be used in main thread only!!!
 * @date 2021-11-23
 */

#pragma once
#include <fs_event_autolock.hpp>
#include <feature/prusa/e-stall_detector.h>
#include "fsm_preheat_type.hpp"
#include "preheat_multithread_status.hpp"
#include <optional>
#include "filament.hpp"
#include "pause_stubbed.hpp"

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

bool load_unload(LoadUnloadMode type, filament_gcodes::Func f_load_unload, pause::Settings &rSettings);

void M701_no_parser(filament::Type filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot, std::optional<filament::Colour> color_to_be_loaded, ResumePrint_t resume_print_request);
void M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded);
void M70X_process_user_response(PreheatStatus::Result res, uint8_t target_extruder);

void M1600_no_parser(filament::Type filament_to_be_loaded, uint8_t target_extruder, RetAndCool_t preheat, AskFilament_t ask_filament, std::optional<filament::Colour> color_to_be_loaded);

/**
 * @brief Stand alone preheat.
 *
 * @param preheat include return and/or cooldown items in menu
 * @param mode preheat mode as part of load/unload
 * @param target_extruder preheat this extruder (indexed from 0), or -1 to preheat all
 * @param save save selected filament settings to EEPROM
 * @param enforce_target_temp true to enforce target temp, false to use preheat temp
 * @param preheat_bed true to also heat up bed
 */
void M1700_no_parser(RetAndCool_t preheat, PreheatMode mode, int8_t target_extruder, bool save, bool enforce_target_temp, bool preheat_bed);

void M1701_no_parser(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder);

void mmu_load(uint8_t data);
void mmu_load_test(uint8_t data);
void mmu_eject(uint8_t data);
void mmu_cut(uint8_t data);

void mmu_reset(uint8_t level);
void mmu_on();
void mmu_off();

std::pair<std::optional<PreheatStatus::Result>, filament::Type> preheat(PreheatData preheat_data, uint8_t target_extruder);
std::pair<std::optional<PreheatStatus::Result>, filament::Type> preheat_for_change_load(PreheatData data, uint8_t target_extruder);
void preheat_to(filament::Type filament, uint8_t target_extruder);
} // namespace filament_gcodes

namespace PreheatStatus {
void SetResult(Result res);
} // namespace PreheatStatus
