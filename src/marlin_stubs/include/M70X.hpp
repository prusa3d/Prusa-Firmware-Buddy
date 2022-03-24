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
// just to minimize the amount of changes to Marlin's parser system (i.e. not adding any additional code letters)
enum class mmu_command_t : char {
    load_filament = 'L',
    eject_filament = 'E',
    Reset = 'X',
    Button = 'B',
    // this is currently handled elsewhere
    // unload = 'U',
    load_filament_to_nozzle_slot_index = 'N', // used to specify filament slot
    cut_filament = 'C',
    Home = 'H',
    StartStop = 'T', // used to be S but cannot use it within M1400
    no_command = 0   // just some value not coliding with letters
};

//TODO split this to multiple headers
namespace m1400 {
using Func = bool (Pause::*)(); // member fnc pointer

bool IsInProgress();
void M1400_no_parser(PreheatData data, mmu_command_t mmu_cmd, uint8_t mmu_command_data);
std::pair<std::optional<PreheatStatus::Result>, filament_t> preheat(PreheatData data);
bool load_unload(LoadUnloadMode type, m1400::Func f_load_unload, uint32_t min_Z_pos, float X_pos);
void M701_no_parser(filament_t filament_to_be_loaded, float fast_load_length, float z_min_pos = Z_AXIS_LOAD_POS);
};
