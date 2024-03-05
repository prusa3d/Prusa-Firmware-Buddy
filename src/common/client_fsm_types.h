#pragma once

#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_toolchanger.h>

#include <inc/MarlinConfig.h>

#include <stdint.h>
#include <utils/utility_extensions.hpp>

#ifdef __cplusplus
// C++ checks enum classes

// Client finite state machines
// bound to src/common/client_response.hpp
enum class ClientFSM : uint8_t {
    Serial_printing,
    Load_unload,
    Preheat,
    Selftest,
    ESP,
    Printing, // not a dialog
    CrashRecovery,
    QuickPause,
    Warning,
    PrintPreview,
    _none, // cannot be created, must have same index as _count
    _count = _none
};

// We have only 5 bits for it in the serialization of data sent between server and client
static_assert(ftrstd::to_underlying(ClientFSM::_count) < 32);

enum class ClientFSM_Command : uint8_t {
    none = 0x00,
    create = 0x80,
    destroy = 0x40,
    change = create | destroy,
    _mask = change
};

enum class LoadUnloadMode : uint8_t {
    Change,
    Load,
    Unload,
    Purge,
    FilamentStuck,
    Test,
    Cut, // MMU
    Eject, // MMU
};

enum class PreheatMode : uint8_t {
    None,
    Load,
    Unload,
    Purge,
    Change_phase1, // do unload, call Change_phase2 after load finishes
    Change_phase2, // do load, meant to be used recursively in Change_phase1
    Unload_askUnloaded,
    Autoload,
    _last = Autoload
};

enum class RetAndCool_t {
    Neither,
    Cooldown,
    Return,
    Both,
    last_ = Both
};

enum class WarningType : uint32_t {
    HotendFanError,
    PrintFanError,
    HeatersTimeout,
    HotendTempDiscrepancy,
    NozzleTimeout,
    #if _DEBUG
    SteppersTimeout,
    #endif
    USBFlashDiskError,
    HeatbedColdAfterPP,
    HeatBreakThermistorFail,
    #if ENABLED(CALIBRATION_GCODE)
    NozzleDoesNotHaveRoundSection,
    #endif
    BuddyMCUMaxTemp,
    #if HAS_DWARF()
    DwarfMCUMaxTemp,
    #endif
    #if HAS_MODULARBED()
    ModBedMCUMaxTemp,
    #endif
    NotDownloaded,
    EnclosureFilterExpirWarning,
    EnclosureFilterExpiration,
    EnclosureFanError,
    _last = EnclosureFanError
};

// Open dialog has a parameter because I need to set a caption of change filament dialog (load / unload / change).
// Use extra state of statemachine to set the caption would be cleaner, but I can miss events.
// Only the last sent event is guaranteed to pass its data.
using fsm_cb_t = void (*)(uint32_t, uint16_t); // create/destroy/change finite state machine
using message_cb_t = void (*)(const char *);
#else // !__cplusplus
// C
typedef void (*fsm_cb_t)(uint32_t, uint16_t); // create/destroy/change finite state machine
typedef void (*message_cb_t)(const char *);
typedef void (*warning_cb_t)(uint32_t);
#endif //__cplusplus
