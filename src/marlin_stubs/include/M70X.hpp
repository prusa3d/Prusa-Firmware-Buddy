/**
 * @file M70X.hpp
 * @author Radek Vana
 * @brief Header to access load/unload/preheat states
 * to be used in main thread only!!!
 * @date 2021-11-23
 */

#pragma once
#include "config_features.h"
#include "client_fsm_types.h"
#include "preheat_multithread_status.hpp"
#include <optional>
#include <algorithm>
#include "filament.hpp"
#include "pause_stubbed.hpp"

namespace filament_gcodes {
using Func = bool (Pause::*)(const pause::Settings &); // member fnc pointer

class InProgress {
    static uint lock;

public:
    InProgress() { ++lock; }
    ~InProgress() { --lock; }
    static bool Active() { return lock > 0; }
};

bool load_unload(LoadUnloadMode type, filament_gcodes::Func f_load_unload, pause::Settings &rSettings);
void M701_no_parser(filament_t filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot);
void M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded);
void M70X_process_user_response(PreheatStatus::Result res);

void M1600_no_parser(uint8_t target_extruder);
void M1700_no_parser(RetAndCool_t preheat, uint8_t target_extruder, bool save, bool enforce_target_temp);
void M1701_no_parser(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder);

void mmu_load(uint8_t data);
void mmu_eject(uint8_t data);
void mmu_cut(uint8_t data);

void mmu_reset(uint8_t level);
void mmu_on();
void mmu_off();

std::pair<std::optional<PreheatStatus::Result>, filament_t> preheat(PreheatData preheat_data);
std::pair<std::optional<PreheatStatus::Result>, filament_t> preheat_for_change_load(PreheatData data);
void preheat_to(filament_t filament);
} // namespace filament_gcodes

namespace PreheatStatus {
void SetResult(Result res);
} // namespace PreheatStatus
