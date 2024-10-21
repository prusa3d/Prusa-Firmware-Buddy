#pragma once

#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_toolchanger.h>
#include <option/has_loadcell.h>
#include <option/has_selftest.h>
#include <option/has_phase_stepping.h>
#include <option/has_coldpull.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_belt_tuning.h>

#include <inc/MarlinConfigPre.h>
#include <device/board.h>

#include <stdint.h>
#include <utils/utility_extensions.hpp>

#ifdef __cplusplus
// C++ checks enum classes

// Client finite state machines
// bound to src/client_response.hpp
enum class ClientFSM : uint8_t {
    Serial_printing,
    Load_unload,
    Preheat,
    #if HAS_SELFTEST()
    Selftest,
    #endif
    NetworkSetup,
    Printing, // not a dialog
    #if ENABLED(CRASH_RECOVERY)
    CrashRecovery,
    #endif
    QuickPause,
    Warning,
    PrintPreview,
    #if HAS_COLDPULL()
    ColdPull,
    #endif
    #if HAS_PHASE_STEPPING()
    PhaseStepping,
    #endif
    #if HAS_INPUT_SHAPER_CALIBRATION()
    InputShaperCalibration,
    #endif
    #if HAS_BELT_TUNING()
    BeltTuning,
    #endif
    _none, // cannot be created, must have same index as _count
    _count = _none
};

// We have only 5 bits for it in the serialization of data sent between server and client
static_assert(ftrstd::to_underlying(ClientFSM::_count) < 32);

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

enum class RetAndCool_t : uint8_t {
    Neither = 0b00,
    Cooldown = 0b01,
    Return = 0b10,
    Both = 0b11,
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
    #if ENABLED(POWER_PANIC)
    HeatbedColdAfterPP,
    #endif
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
    #if HAS_BED_PROBE
    ProbingFailed,
    #endif
    #if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    NozzleCleaningFailed,
    #endif
    #if XL_ENCLOSURE_SUPPORT()
    EnclosureFilterExpirWarning,
    EnclosureFilterExpiration,
    EnclosureFanError,
    #endif
    NotDownloaded,
    GcodeCorruption,
    GcodeCropped,
    MetricsConfigChangePrompt,
    AccelerometerCommunicationFailed,
    _last = AccelerometerCommunicationFailed
};

using message_cb_t = void (*)(char *);
#else // !__cplusplus
// C
typedef void (*message_cb_t)(const char *);
typedef void (*warning_cb_t)(uint32_t);
#endif //__cplusplus
