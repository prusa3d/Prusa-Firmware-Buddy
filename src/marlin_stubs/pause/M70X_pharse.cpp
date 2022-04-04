#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "RAII.hpp"
#include "M70X.hpp"
#include "fs_event_autolock.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "pause_stubbed.hpp"
using namespace m1400;
static Pause &pause = Pause::Instance();

/**
 * M701: Load filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  Z<distance> - Move the Z axis by this distance
 *  L<distance> - Extrude distance for insertion (positive value) (manual reload)
 *  S"Filament" - save filament by name, for example S"PLA". RepRap compatible.
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    filament_t filament_to_be_loaded = Filaments::Default;
    const char *text_begin = 0;
    if (parser.seen('S')) {
        text_begin = strchr(parser.string_arg, '"');
        if (text_begin) {
            ++text_begin; // move pointer from '"' to first letter
            const char *text_end = strchr(text_begin, '"');
            if (text_end) {
                filament_t filament = Filaments::FindByName(text_begin, text_end - text_begin);
                if (filament != filament_t::NONE) {
                    filament_to_be_loaded = filament;
                }
            }
        }
    }
    const bool isL = (parser.seen('L') && (!text_begin || strchr(parser.string_arg, 'L') < text_begin));
    const float fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : pause.GetDefaultFastLoadLength());
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    M701_no_parser(filament_to_be_loaded, fast_load_length, min_Z_pos);
}

/**
 * M702: Unload filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, if omitted, current extruder
 *                (or ALL extruders with FILAMENT_UNLOAD_ALL_EXTRUDERS).
 *  Z<distance> - Move the Z axis by this distance
 *  U<distance> - Retract distance for removal (manual reload)
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    Pause::Instance().SetUnloadLength(parser.seen('U') ? parser.value_axis_units(E_AXIS) : NAN);
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    load_unload(
        LoadUnloadMode::Unload, &Pause::FilamentUnload, min_Z_pos, X_AXIS_UNLOAD_POS);
}

static bool m1400_in_progress = false;
bool m1400::IsInProgress() { return m1400_in_progress; }

/**
 * M1400: Preheat
 * not meant to be used during print
 *
 * S<bit fields value>
 * [0 - 3] PreheatMode - 0 None
 *                     - 1 Load
 *                     - 2 Unload
 *                     - 3 Purge
 *                     - 4 Change_phase1 == unload + recursively call Change_phase2
 *                     - 5 Change_phase2 (internal use only, do load)
 *                     - 6 Unload, with unloaded check
 *                     - 7 Autoload
 *                     - 8 MMU_unload,
 *                     - 9 MMU_load
 *                     - 10 MMU_command
 * [4 - 5] reserved
 * [6] has return option
 * [7] has cooldown option, PreheatMode must be PreheatMode::None, othervise ignored
 * [8 - 31] reserved
 *  Default value S0
 */
void PrusaGcodeSuite::M1400() {
    AutoRestore AR(m1400_in_progress, true);
    FS_EventAutolock LOCK;

    mmu_command_t mmu_cmd = mmu_command_t::no_command;
    uint8_t command_data = 0;

    const uint32_t val = parser.ulongval('S', 0);
    const PreheatData data(val);

    // load to nozzle == normal load + MMU load
    // so it has MMU command too
    bool mmu_command = data.Mode() == PreheatMode::MMU_command || data.Mode() == PreheatMode::MMU_load;

    if (mmu_command) {
        if (parser.seenval(char(mmu_command_t::load_filament))) {
            mmu_cmd = mmu_command_t::load_filament;
        } else if (parser.seenval(char(mmu_command_t::eject_filament))) {
            mmu_cmd = mmu_command_t::eject_filament;
        } else if (parser.seen(char(mmu_command_t::Reset))) {
            mmu_cmd = mmu_command_t::Reset;
        } else if (parser.seen(char(mmu_command_t::Button))) {
            mmu_cmd = mmu_command_t::Button;
            /*} else if (parser.seen(char(mmu_command_t::unload))) {
            mmu_cmd = mmu_command_t::unload;*/
        } else if (parser.seen(char(mmu_command_t::load_filament_to_nozzle_slot_index))) {
            //dont set command, only data, which is automatic
        } else if (parser.seen(char(mmu_command_t::cut_filament))) {
            mmu_cmd = mmu_command_t::cut_filament;
        } else if (parser.seen(char(mmu_command_t::Home))) {
            mmu_cmd = mmu_command_t::Home;
        } else if (parser.seen(char(mmu_command_t::StartStop))) {
            mmu_cmd = mmu_command_t::StartStop;
        }
        // command are mutally exclusive, so if i find one, I will just read its data
        command_data = parser.value_byte();
    }

    m1400::M1400_no_parser(data, mmu_cmd, command_data);

    FSensors_instance().ClrAutoloadSent();
}
