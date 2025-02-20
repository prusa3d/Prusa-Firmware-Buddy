#pragma once

#include "client_response.hpp"

enum class WarningType : uint32_t {
    HotendFanError,
    PrintFanError,
    HeatersTimeout,
    HotendTempDiscrepancy,
    NozzleTimeout,
    FilamentLoadingTimeout,
    FilamentSensorStuckHelp,
#if HAS_MMU2()
    FilamentSensorStuckHelpMMU,
#endif
    FilamentSensorsDisabled,
#if _DEBUG
    SteppersTimeout,
#endif
    USBFlashDiskError,
    USBDriveUnsupportedFileSystem,
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
#if ENABLED(DETECT_PRINT_SHEET)
    SteelSheetNotDetected,
#endif
    NotDownloaded,
    GcodeCorruption,
    GcodeCropped,
    MetricsConfigChangePrompt,
#if HAS_EMERGENCY_STOP()
    DoorOpen,
#endif
#if HAS_CHAMBER_API()
    FailedToReachChamberTemperature,
#endif
#if PRINTER_IS_PRUSA_COREONE()
    OpenChamberVents,
    CloseChamberVents,
#endif
#if HAS_UNEVEN_BED_PROMPT()
    BedUnevenAlignmentPrompt,
#endif
#if HAS_CHAMBER_API()
    ChamberOverheatingTemperature,
    ChamberCriticalTemperature,
#endif
#if HAS_XBUDDY_EXTENSION()
    ChamberCoolingFanError,
#endif
#if HAS_XBUDDY_EXTENSION() || XL_ENCLOSURE_SUPPORT()
    ChamberFiltrationFanError,
#endif
    AccelerometerCommunicationFailed,
    _last = AccelerometerCommunicationFailed,
};

PhasesWarning warning_type_phase(WarningType warning);
