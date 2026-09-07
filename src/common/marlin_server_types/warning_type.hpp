#pragma once

#include "client_response.hpp"
#include <guiconfig/guiconfig.h>
#include <option/has_chamber_vents.h>
#include <option/has_dwarf.h>
#include <option/has_power_panic.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_remote_bed.h>
#include <option/has_chamber_filtration_api.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_selftest.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_bed_fan.h>
#include <option/has_psu_fan.h>
#include <option/has_mmu2.h>
#include <option/has_human_interactions.h>
#include <option/has_anfc.h>
#include <option/has_tool_offset_pin_calibration.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_ht_hotend.h>
#include <option/has_ceiling_clearance.h>
#include <option/has_chamber_api.h>
#include <option/has_emergency_stop.h>
#include <option/has_uneven_bed_prompt.h>
#include <option/xl_enclosure_support.h>

enum class WarningType : uint32_t {
#if HAS_EMERGENCY_STOP()
    DoorOpen,
#endif
#if HAS_HT_HOTEND()
    /// Burn risk warning: door is open and nozzle is above burn_warning_temp.
    /// Independent of emergency stop (a single-Z printer with a door + HT hotend still
    /// needs it); every HT printer has a door sensor, so HAS_HT_HOTEND() is the right gate.
    HotendBurnRisk,
#endif
    HotendFanError,
    PrintFanError,
#if HAS_INDX()
    DockFanError,
#endif
    HeatersTimeout,
    HotendTempDiscrepancy,
    NozzleTimeout,
    FilamentLoadingTimeout,
    FilamentSensorStuckHelp,
#if HAS_MMU2()
    FilamentSensorStuckHelpMMU,
    MaintenanceWarningFails,
    MaintenanceWarningChanges,
#endif
    FilamentSensorsDisabled,
#if _DEBUG
    SteppersTimeout,
#endif
    USBFlashDiskError,
    USBDriveUnsupportedFileSystem,
#if HAS_SELFTEST()
    SelftestNotSuccessfullyCompleted,
    ActionSelftestRequired,
#endif
#if HAS_POWER_PANIC()
    HeatbedColdAfterPP,
#endif
    HeatBreakThermistorFail,
#if HAS_TOOL_OFFSET_PIN_CALIBRATION()
    NozzleDoesNotHaveRoundSection,
#endif
    BuddyMCUMaxTemp,
#if HAS_DWARF()
    DwarfMCUMaxTemp,
#endif
#if HAS_REMOTE_BED()
    BedMCUMaxTemp,
#endif
    ProbingFailed,
#if XL_ENCLOSURE_SUPPORT() || HAS_CHAMBER_FILTRATION_API()
    EnclosureFilterExpirWarning,
    EnclosureFilterExpiration,
#endif
#if XL_ENCLOSURE_SUPPORT()
    EnclosureFanError,
#endif
#if HAS_PRINT_SHEET_DETECTION()
    SteelSheetNotDetected,
#endif
    NotDownloaded,
    GcodeCorruption,
    GcodeCropped,
    MetricsConfigChangePrompt,
#if HAS_CHAMBER_API()
    FailedToReachChamberTemperature,
#endif
#if HAS_CHAMBER_VENTS()
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
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    ChamberCoolingFanError,
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD() || XL_ENCLOSURE_SUPPORT()
    ChamberFiltrationFanError,
#endif
#if HAS_CEILING_CLEARANCE()
    CeilingClearanceViolation,
#endif
#if HAS_PRECISE_HOMING_COREXY()
    HomingCalibrationNeeded,
    HomingRefinementFailed,
    HomingCalibrationFromMenuNeeded,
#endif
    AccelerometerCommunicationFailed,
#if HAS_TOOL_OFFSET_SENSOR()
    ToolOffsetCalibrationFailed,
    HotendOffsetUnsafeZDeviation,
    HotendOffsetUnsafeXyDeviation,
    HotendOffsetUnsafeSensorXY,
#endif
#if HAS_ANFC()
    /// Filament tracking unrecoverably failed for any reason
    OpenPrintTagCannotTrack,

    /// Filament tracking write failed, maybe the user removed the tag?
    OpenPrintTagUsageWriteFailed,
#endif

#if HAS_WASTEBIN_FILL_TRACKING()
    /// The INDX nozzle-cleaner wastebin is full; the print is paused so the user can empty it (Ignore / Done).
    NozzleCleanerFull,
    /// The wastebin is full but auto-pause is off: informational only, the print keeps running (single dismiss).
    NozzleCleanerFullInfo,
    /// Manual "Empty Wastebin" (M1986): the print is parked, prompting the user to empty the bin and confirm.
    NozzleCleanerManualEmpty,
#endif

#if PRINTER_IS_PRUSA_XL()
    /// XL-CAN bridge detected at boot; printer type set to XLS.
    PrinterDetectedAsXLS,
    /// No XL-CAN bridge found at boot; printer type set to XL.
    PrinterDetectedAsXL,
    /// XL-CAN bridge not responding and MB reset not under master control; check cabling.
    XlCanWiringSuspected,
#endif
#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
    DisplayProblemDetected,
#endif
    _cnt,
};

PhasesWarning warning_type_phase(WarningType warning);

uint32_t warning_lifespan_sec(WarningType type);
