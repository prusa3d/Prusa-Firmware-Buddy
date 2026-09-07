/// @file
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <span>
#include <utility>

#include "client_fsm_types.hpp"
#include "general_response.hpp"
#include "printers.h"
#include <utils/enum_array.hpp>
#include <guiconfig/guiconfig.h>
#include <option/filament_sensor.h>
#include <option/has_attachable_accelerometer.h>
#include <option/has_coldpull.h>
#include <option/has_crash_detection.h>
#include <option/has_emergency_stop.h>
#include <option/has_esp.h>
#include <option/has_ht_hotend.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_loadcell.h>
#include <option/has_mmu2.h>
#include <option/has_nfc.h>
#include <option/has_phase_stepping_calibration.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_selftest.h>
#include <option/has_heaters_selftest_gcode.h>
#include <option/has_heaters_selftest_bed_sheet_retry.h>
#include <option/has_heaters_selftest_revise.h>
#include <option/has_toolchanger.h>
#include <option/has_tool_mapping.h>
#include <option/xl_enclosure_support.h>
#include <option/has_chamber_api.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_uneven_bed_prompt.h>
#include <option/has_ceiling_clearance.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_auto_retract.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_chamber_vents.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_side_fsensor.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_human_interactions.h>
#include <option/has_tool_crash_recovery.h>
#include <option/has_dwarf.h>

#include <option/has_hotend_type_support.h>
#if HAS_HOTEND_TYPE_SUPPORT()
    #include <hotend_type.hpp>
#endif

#include <device/board.h>
#include <option/has_e2ee_support.h>
#include <option/has_indx.h>
#include <option/has_serial_print.h>
#include <feature/tool_offset_wizard/has_tool_offset_nozzle_cleaning_wizard.hpp>
#include <bsod/bsod.h>

/// Maximum number of responses available during a FSM phase
#if HAS_MMU2()
    // MMU_ERRWaitingForUser I'm angrily looking at you
    #define MAX_RESPONSES 7
#else
    #define MAX_RESPONSES 4
#endif

using PhaseResponses = std::array<Response, MAX_RESPONSES>;
inline constexpr PhaseResponses empty_phase_responses = {};

// count enum class members (if "_last" is defined)
template <class T>
constexpr size_t CountPhases() {
    return static_cast<size_t>(T::_last) + 1;
}
// use this when creating an event
// encodes enum to position in phase
template <class T>
constexpr uint8_t GetPhaseIndex(T phase) {
    return static_cast<size_t>(phase);
}

template <class T>
constexpr T GetEnumFromPhaseIndex(size_t index) {
    debug_assert(index < CountPhases<T>());
    return static_cast<T>(index);
}

using PhaseUnderlyingType = uint8_t;

struct FSMAndPhase {

public:
    constexpr FSMAndPhase(ClientFSM fsm, PhaseUnderlyingType phase)
        : fsm(fsm)
        , phase(phase) {
    }

    template <typename Phase>
    constexpr FSMAndPhase(Phase phase)
        : fsm(client_fsm_from_phase(phase))
        , phase(std::to_underlying(phase)) {
    }

    constexpr FSMAndPhase(const FSMAndPhase &) = default;

    constexpr bool operator==(const FSMAndPhase &) const = default;
    constexpr bool operator!=(const FSMAndPhase &) const = default;

public:
    ClientFSM fsm;
    PhaseUnderlyingType phase;
};

enum class PhaseWait : PhaseUnderlyingType {
    /// Shows PrintStatusMessage if available, otherwise empty text
    print_status_message,
    _cnt,
    _last = _cnt - 1,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseWait) { return ClientFSM::Wait; }

#if HAS_SELFTEST()
// GUI phases of selftest/wizard
// WARNING: make sure that _first_xx and _last_xx are defined after normal selftest phases. This enum is exported by magic_enum library, and it has
// a limitation that only first item with same value is exported.
enum class PhasesSelftest : PhaseUnderlyingType {
    _none,

    Loadcell_prepare,
    Loadcell_move_away,
    Loadcell_tool_select,
    Loadcell_cooldown,
    Loadcell_user_tap_ask_abort,
    Loadcell_user_tap_ask_ignore_abort,
    Loadcell_user_tap_countdown,
    Loadcell_user_tap_check,
    Loadcell_user_tap_ok,
    Loadcell_fail,
    _first_Loadcell = Loadcell_prepare,
    _last_Loadcell = Loadcell_fail,

    CalibZ,
    _first_CalibZ = CalibZ,
    _last_CalibZ = CalibZ,

    Axis,
    _first_Axis = Axis,
    _last_Axis = Axis,

    Heaters,
    #if HAS_INDX()
    /// INDX-only: shown while physically picking a tool before the nozzle
    /// heater test. INDX has a single active hotend instance (only the picked
    /// tool can be read/controlled), so the test must pick a tool first. XL,
    /// MK4 and others either index per tool data at compile time or have a
    /// single fixed hotend, so no pickup screen is needed there.
    Heaters_PickingTool,
    #endif
    HeatersDisabledDialog,
    Heaters_AskBedSheetAfterFail, ///< After bed heater selftest failed, this state prompts the user if he didn't forget to put on the print sheet
    _first_Heaters = Heaters,
    _last_Heaters = Heaters_AskBedSheetAfterFail,

    FirstLayer_mbl,
    FirstLayer_print,
    _first_FirstLayer = FirstLayer_mbl,
    _last_FirstLayer = FirstLayer_print,

    FirstLayer_filament_known_and_not_unsensed,
    FirstLayer_filament_not_known_or_unsensed,
    FirstLayer_calib,
    FirstLayer_use_val,
    FirstLayer_start_print,
    FirstLayer_reprint,
    FirstLayer_clean_sheet,
    FirstLayer_failed,
    _first_FirstLayerQuestions = FirstLayer_filament_known_and_not_unsensed,
    _last_FirstLayerQuestions = FirstLayer_failed,

    Dock_needs_calibration,
    Dock_move_away,
    Dock_wait_user_park1,
    Dock_wait_user_park2,
    Dock_wait_user_park3,
    Dock_wait_user_remove_pins,
    Dock_wait_user_loosen_pillar,
    Dock_wait_user_lock_tool,
    Dock_wait_user_tighten_top_screw,
    Dock_measure,
    Dock_wait_user_tighten_bottom_screw,
    Dock_wait_user_install_pins,
    Dock_selftest_park_test,
    Dock_selftest_failed,
    Dock_calibration_success,
    _first_Dock = Dock_needs_calibration,
    _last_Dock = Dock_calibration_success,

    ToolOffsets_wait_user_confirm_start,
    ToolOffsets_wait_user_clean_nozzle_cold,
    ToolOffsets_wait_user_clean_nozzle_hot,
    ToolOffsets_wait_user_install_sheet,
    ToolOffsets_pin_install_prepare,
    ToolOffsets_wait_user_install_pin,
    ToolOffsets_wait_stable_temp,
    ToolOffsets_wait_calibrate,
    ToolOffsets_wait_move_away,
    ToolOffsets_wait_user_remove_pin,
    _first_Tool_Offsets = ToolOffsets_wait_user_confirm_start,
    _last_Tool_Offsets = ToolOffsets_wait_user_remove_pin,

    RevisePrinterStatus_ask_revise, ///< Notifies that a selftest part failed and asks if the user wants to revise the setup
    RevisePrinterStatus_revise, ///< ScreenPrinterSetup being shown, user revising the printer setup
    RevisePrinterStatus_ask_retry, ///< After revision, ask the user to retry the selftest
    _first_RevisePrinterStatus = RevisePrinterStatus_ask_revise,
    _last_RevisePrinterStatus = RevisePrinterStatus_ask_retry,

    _last = _last_RevisePrinterStatus,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesSelftest) { return ClientFSM::Selftest; }

enum class PhasesFansSelftest : PhaseUnderlyingType {
    test_100_percent,
    #if PRINTER_IS_PRUSA_MK3_5()
    manual_check,
    #endif
    test_40_percent,
    results,
    _last = results,
};

constexpr inline ClientFSM client_fsm_from_phase(PhasesFansSelftest) { return ClientFSM::FansSelftest; }
#endif

#if HAS_HEATERS_SELFTEST_GCODE()
enum class PhasesHeatersSelftest : PhaseUnderlyingType {
    #if HAS_INDX()
    picking_tool, ///< INDX: pick a tool before testing its nozzle heater
    #endif
    heating, ///< cooldown -> preheat -> timed heat measurement (live data via HeatersSelftestData)
    hotend_fan_failed_dialog, ///< skip the nozzle heater test because the hotend (heatbreak) fan failed
    #if HAS_INDX()
    nozzle_failed_dialog, ///< nozzle heater failed: show why (HeatersSelftestFailReason in phase data)
    #endif
    #if HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
    ask_bed_sheet_after_fail, ///< bed heater failed: ask to refit the steel sheet and retry
    #endif
    #if HAS_HEATERS_SELFTEST_REVISE()
    revise_ask_revise, ///< nozzle heater failed: ask whether to revise the printer setup
    revise_revise, ///< printer setup screen shown while the user revises the setup
    revise_ask_retry, ///< after the revision, ask whether to retry the test
    #endif
    #if HAS_HEATERS_SELFTEST_REVISE()
    _last = revise_ask_retry,
    #elif HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
    _last = ask_bed_sheet_after_fail,
    #elif HAS_INDX()
    _last = nozzle_failed_dialog,
    #else
    _last = hotend_fan_failed_dialog,
    #endif
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesHeatersSelftest) { return ClientFSM::HeatersSelftest; }
#endif

#if HAS_ESP()
enum class PhaseNetworkSetup : PhaseUnderlyingType {
    init,

    ask_switch_to_wifi, ///< User is already connected through an ethernet cable, ask him if he wants to switch to wi-fi
    action_select, ///< Letting the user to choose how the wi-fi should be set up
    wifi_scan, ///< Scanning available wi-fi networks (the scanning is fully handled on the GUI thread)
    wait_for_ini_file, ///< Prompting user to insert a flash drive with creds
    ask_delete_ini_file, ///< Asking the user if he wants to delete the ini file
    #if HAS_NFC()
    ask_use_prusa_app, ///< User is prompted if he wants to use the Prusa app to connect to the wi-fi
    wait_for_nfc, ///< Prompting user to provide the credentials through NFW
    nfc_confirm, ///< Loaded credentials via NFC, asking for confirmation
    #endif
    connecting_finishable, ///< The user is connecting to a Wi-Fi. The screen offers a "Finish" button that keeps connecting on the background and "Cancel" to go back.
    connecting_nonfinishable, ///< The user is connecting to a Wi-Fi. The screen only offers a "Cancel" button to go back.
    connected,
    ask_setup_prusa_connect, ///< Prompts the user if he wants to set up Prusa Connect
    prusa_conect_setup, ///< Setup connect is running, waiting for it to finish

    no_interface_error,
    connection_error,
    help_qr, ///< Display as QR code to the help page

    finish,
    _last = finish,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseNetworkSetup) { return ClientFSM::NetworkSetup; }
#endif

#if HAS_CRASH_DETECTION()
enum class PhasesCrashRecovery : PhaseUnderlyingType {
    check_X,
    check_Y,
    home,
    home_gcode_interrupt, //< Rehoming during gcode interrupt
    axis_NOK, //< just for unification of the two below
    axis_short,
    axis_long,
    repeated_crash,
    home_fail, //< Homing failed, ask to retry
    #if HAS_TOOL_CRASH_RECOVERY() && HAS_DWARF()
    tool_recovery, //< Toolchanger recovery, tool fell off
    _last = tool_recovery
    #else
    _last = home_fail
    #endif
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesCrashRecovery) { return ClientFSM::CrashRecovery; }
#endif

enum class PhasesQuickPause : PhaseUnderlyingType {
    QuickPaused,
    _last = QuickPaused
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesQuickPause) { return ClientFSM::QuickPause; }

enum class PhasesWarning : PhaseUnderlyingType {
#if HAS_EMERGENCY_STOP()
    DoorOpen,
#endif
#if HAS_HT_HOTEND()
    // Like DoorOpen: no buttons, firmware-driven dismissal.
    HotendBurnRisk,
#endif
    // Generic warning with a Continue button, just for dismissing it.
    Warning,

// These have some actual buttons that need to be handled.
#if XL_ENCLOSURE_SUPPORT() || HAS_CHAMBER_FILTRATION_API()
    EnclosureFilterExpiration,
#endif

#if HAS_CHAMBER_VENTS()
    ChamberVents,
#endif

    ProbingFailed,

    FilamentSensorStuckHelp,

#if HAS_MMU2()
    FilamentSensorStuckHelpMMU,
#endif

#if HAS_PRINT_SHEET_DETECTION()
    /// Shown on failed print sheet detection. Custom handling.
    SteelSheetNotDetected,
#endif

#if HAS_CHAMBER_API()
    FailedToReachChamberTemperature,
#endif

#if HAS_UNEVEN_BED_PROMPT()
    /// A prompt offering Z align calibration when uneven bed is detected
    BedUnevenAlignmentPrompt,
#endif

#if HAS_CEILING_CLEARANCE()
    CeilingClearanceViolation,
#endif

#if HAS_PRECISE_HOMING_COREXY()
    HomingCalibrationNeeded,
    HomingRefinementFailed,
    HomingCalibrationFromMenuNeeded,
#endif

#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
    DisplayProblemDetected,
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    /// Blocking dialog shown when tool offset calibration fails. Lets the user abort or retry.
    ToolOffsetCalibrationFailed,
    HotendOffsetUnsafeZDeviation,
    HotendOffsetUnsafeXyDeviation,
    HotendOffsetUnsafeSensorXY,
#endif

#if HAS_WASTEBIN_FILL_TRACKING()
    /// Mid-print, auto-pause on: the nozzle-cleaner wastebin is full. Ignore (disable check this print) / Done (reset counter).
    NozzleCleanerFull,
    /// Manual "Empty Wastebin": print parked, prompting the user to empty the bin and confirm (Done -> reset counter).
    NozzleCleanerManualEmpty,
#endif

    /// Shown when the M334 is attempting to change metrics configuration, prompting the user to confirm the change (security reasons)
    MetricsConfigChangePrompt,

    _last = MetricsConfigChangePrompt,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesWarning) { return ClientFSM::Warning; }

#if HAS_COLDPULL()
enum class PhasesColdPull : PhaseUnderlyingType {
    introduction,
    #if HAS_TOOLCHANGER()
    select_tool,
    pick_tool,
    #endif
    #if HAS_MMU2()
    stop_mmu,
    #endif
    #if HAS_TOOLCHANGER() || HAS_MMU2()
    unload_ptfe,
    load_ptfe,
    #endif
    prepare_filament,
    #if HAS_AUTO_RETRACT()
    deretract,
    #endif
    blank_load,
    blank_unload,
    cool_down,
    heat_up,
    automatic_pull,
    manual_pull,
    cleanup,
    pull_done,
    finish,
    _last = finish,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesColdPull) { return ClientFSM::ColdPull; }
#endif

#if HAS_PHASE_STEPPING_CALIBRATION()
enum class PhasesPhaseStepping : PhaseUnderlyingType {
    restore_defaults,
    intro,
    home,
    #if HAS_ATTACHABLE_ACCELEROMETER()
    connect_to_board,
    wait_for_extruder_temperature,
    attach_to_extruder,
    attach_to_bed,
    #endif
    calib_x,
    calib_y,
    calib_error,
    calib_nok,
    calib_ok,
    abort, ///< Internal dispatch only — never fsm_changed to. Handler marks the invocation aborted then transitions to finish.
    finish,
    _last = finish,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesPhaseStepping) { return ClientFSM::PhaseSteppingCalibration; }
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
enum class PhasesInputShaperCalibration : PhaseUnderlyingType {
    #if HAS_ATTACHABLE_ACCELEROMETER()
    info,
    connect_to_board,
    wait_for_extruder_temperature,
    attach_to_extruder,
    attach_to_bed,
    #endif
    parking,
    measuring_x_axis,
    measuring_y_axis,
    measurement_failed,
    computing,
    bad_results,
    results,
    abort, ///< Internal dispatch only — never fsm_changed to. Handler marks the invocation aborted then transitions to finish.
    finish,
    _last = finish,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesInputShaperCalibration) { return ClientFSM::InputShaperCalibration; }
#endif

enum class PhasesPrinting : PhaseUnderlyingType {
    active,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesPrinting) { return ClientFSM::Printing; }

#if HAS_SERIAL_PRINT()
enum class PhasesSerialPrinting : PhaseUnderlyingType {
    active,
};
constexpr inline ClientFSM client_fsm_from_phase(PhasesSerialPrinting) { return ClientFSM::Serial_printing; }
#endif

#if HAS_DOOR_SENSOR_CALIBRATION()
enum class PhaseDoorSensorCalibration : PhaseUnderlyingType {
    confirm_abort,
    repeat,
    skip_ask,
    confirm_closed,
    tighten_screw_half,
    confirm_open,
    loosen_screw_half,
    finger_test,
    loosen_screw_quarter,
    ask_enable_safety_features,
    warn_disabled_sensor,
    done,
    finish,
    _last = finish,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseDoorSensorCalibration) { return ClientFSM::DoorSensorCalibration; }
#endif

#if HAS_INDX()
enum class PhaseDockCalibration : PhaseUnderlyingType {
    intro,
    remove_tool,
    select_dock_count,
    select_docks,
    homing,
    moving_away,
    parking_tool,
    tighten_silver_screws,
    ask_position_dock,
    lock_position,
    measuring,
    loosen_each_bolt,
    calibration_success,
    calibration_failed,
    _last = calibration_failed,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseDockCalibration) { return ClientFSM::DockCalibration; }

enum class PhaseNozzleCleanerCalibration : PhaseUnderlyingType {
    intro,
    wait_for_nozzle_cooldown,
    picking_tool,
    homing,
    moving_away,
    move_to_z_point,
    ask_position_x,
    lock_position_x,
    measuring_x,
    evaluating_x,
    clean_nozzle,
    ask_position_y,
    lock_position_y,
    measuring_y,
    evaluating_y,
    calibration_success,
    _last = calibration_success,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseNozzleCleanerCalibration) { return ClientFSM::NozzleCleanerCalibration; }
#endif
#if HAS_TOOL_OFFSET_SENSOR()
enum class PhaseToolOffsetsCalibration : PhaseUnderlyingType {
    intro,
    #if HAS_TOOL_OFFSET_NOZZLE_CLEANING_WIZARD()
    clean_nozzles_cold,
    clean_nozzles_hot,
    #else
    ensure_nozzles_clean,
    #endif
    moving_away,
    homing,
    picking_tool,
    calibrating,
    calibration_success,
    calibration_failed,
    _last = calibration_failed,
};
constexpr inline ClientFSM client_fsm_from_phase(PhaseToolOffsetsCalibration) { return ClientFSM::ToolOffsetsCalibration; }
#endif

namespace ClientResponses {

#if HAS_SELFTEST()
inline constexpr EnumArray<PhasesSelftest, PhaseResponses, CountPhases<PhasesSelftest>()> SelftestResponses {
    { PhasesSelftest::_none, {} },
        { PhasesSelftest::Loadcell_prepare, {} },
        { PhasesSelftest::Loadcell_move_away, {} },
        { PhasesSelftest::Loadcell_tool_select, {} },
        { PhasesSelftest::Loadcell_cooldown, { Response::Abort } },

        { PhasesSelftest::Loadcell_user_tap_ask_abort, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Loadcell_user_tap_ask_ignore_abort, { Response::Continue, Response::Ignore, Response::Abort } },
        { PhasesSelftest::Loadcell_user_tap_countdown, {} },
        { PhasesSelftest::Loadcell_user_tap_check, {} },
        { PhasesSelftest::Loadcell_user_tap_ok, {} },
        { PhasesSelftest::Loadcell_fail, {} },

        { PhasesSelftest::CalibZ, {} },

        { PhasesSelftest::Axis, {} },

        { PhasesSelftest::Heaters, {} },
    #if HAS_INDX()
        { PhasesSelftest::Heaters_PickingTool, {} },
    #endif
        { PhasesSelftest::HeatersDisabledDialog, { Response::Ok } },
        { PhasesSelftest::Heaters_AskBedSheetAfterFail, { Response::Ok, Response::Retry } },

        { PhasesSelftest::FirstLayer_mbl, {} },
        { PhasesSelftest::FirstLayer_print, {} },

        { PhasesSelftest::FirstLayer_filament_known_and_not_unsensed, { Response::Next, Response::Unload } },
        { PhasesSelftest::FirstLayer_filament_not_known_or_unsensed, { Response::Next, Response::Load, Response::Unload } },
        { PhasesSelftest::FirstLayer_calib, { Response::Next } },
        { PhasesSelftest::FirstLayer_use_val, { Response::Yes, Response::No } },
        { PhasesSelftest::FirstLayer_start_print, { Response::Next } },
        { PhasesSelftest::FirstLayer_reprint, { Response::Yes, Response::No } },
        { PhasesSelftest::FirstLayer_clean_sheet, { Response::Next } },
        { PhasesSelftest::FirstLayer_failed, { Response::Next } },

        { PhasesSelftest::Dock_needs_calibration, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_move_away, {} },
        { PhasesSelftest::Dock_wait_user_park1, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_park2, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_park3, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_remove_pins, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_loosen_pillar, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_lock_tool, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_tighten_top_screw, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_measure, { Response::Abort } },
        { PhasesSelftest::Dock_wait_user_tighten_bottom_screw, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_wait_user_install_pins, { Response::Continue, Response::Abort } },
        { PhasesSelftest::Dock_selftest_park_test, { Response::Abort } },
        { PhasesSelftest::Dock_selftest_failed, {} },
        { PhasesSelftest::Dock_calibration_success, { Response::Continue } },

        { PhasesSelftest::ToolOffsets_wait_user_confirm_start, { Response::Continue, Response::Abort } },
        { PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold, { Response::Heatup, Response::Continue } },
        { PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot, { Response::Cooldown, Response::Continue } },
        { PhasesSelftest::ToolOffsets_wait_user_install_sheet, { Response::Continue } },
        { PhasesSelftest::ToolOffsets_pin_install_prepare, {} },
        { PhasesSelftest::ToolOffsets_wait_user_install_pin, { Response::Continue } },
        { PhasesSelftest::ToolOffsets_wait_stable_temp, {} },
        { PhasesSelftest::ToolOffsets_wait_calibrate, {} },
        { PhasesSelftest::ToolOffsets_wait_move_away, {} },
        { PhasesSelftest::ToolOffsets_wait_user_remove_pin, { Response::Continue } },
        { PhasesSelftest::RevisePrinterStatus_ask_revise, { Response::Adjust, Response::Skip } },
        { PhasesSelftest::RevisePrinterStatus_revise, { Response::Done } },
        { PhasesSelftest::RevisePrinterStatus_ask_retry, { Response::Yes, Response::No } },
};

inline constexpr PhaseResponses FanSelftestResponses[] = {
    {}, // PhasesFanSelftest::test_100_percent
    #if PRINTER_IS_PRUSA_MK3_5()
    { Response::Yes, Response::No }, // PhasesFanSelftest::manual_check
    #endif
    {}, // PhasesFanSelftest::test_40_percent
    {}, // PhasesFanSelftest::results
};
static_assert(std::size(ClientResponses::FanSelftestResponses) == CountPhases<PhasesFansSelftest>());
#endif

#if HAS_HEATERS_SELFTEST_GCODE()
inline constexpr EnumArray<PhasesHeatersSelftest, PhaseResponses, CountPhases<PhasesHeatersSelftest>()> heaters_selftest_responses {
    #if HAS_INDX()
    { PhasesHeatersSelftest::picking_tool, {} },
    #endif
        { PhasesHeatersSelftest::heating, {} },
        { PhasesHeatersSelftest::hotend_fan_failed_dialog, { Response::Ok } },
    #if HAS_INDX()
        { PhasesHeatersSelftest::nozzle_failed_dialog, { Response::Ok } },
    #endif
    #if HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
        { PhasesHeatersSelftest::ask_bed_sheet_after_fail, { Response::Ok, Response::Retry } },
    #endif
    #if HAS_HEATERS_SELFTEST_REVISE()
        { PhasesHeatersSelftest::revise_ask_revise, { Response::Adjust, Response::Skip } },
        { PhasesHeatersSelftest::revise_revise, { Response::Done } },
        { PhasesHeatersSelftest::revise_ask_retry, { Response::Yes, Response::No } },
    #endif
};
#endif

#if HAS_ESP()
inline constexpr EnumArray<PhaseNetworkSetup, PhaseResponses, CountPhases<PhaseNetworkSetup>()> network_setup_responses {
    { PhaseNetworkSetup::init, {} },
        { PhaseNetworkSetup::ask_switch_to_wifi, { Response::Yes, Response::No } },
        // Note: Additionally to this, the phase accepts various NetworkSetupResponse responses through FSMResponseVariant
        { PhaseNetworkSetup::action_select, { Response::Back, Response::Help } },
        // Note: Additionally to this, the phase accepts various NetworkSetupResponse responses through FSMResponseVariant
        { PhaseNetworkSetup::wifi_scan, { Response::Back } },
        { PhaseNetworkSetup::wait_for_ini_file, { Response::Cancel } },
        { PhaseNetworkSetup::ask_delete_ini_file, { Response::Yes, Response::No } },
    #if HAS_NFC()
        { PhaseNetworkSetup::ask_use_prusa_app, { Response::Yes, Response::No } },
        { PhaseNetworkSetup::wait_for_nfc, { Response::Cancel } },
        { PhaseNetworkSetup::nfc_confirm, { Response::Ok, Response::Cancel } },
    #endif
        { PhaseNetworkSetup::connecting_finishable, { Response::Finish, Response::Cancel } },
        { PhaseNetworkSetup::connecting_nonfinishable, { Response::Cancel } },
        { PhaseNetworkSetup::connected, { Response::Ok } },
        { PhaseNetworkSetup::ask_setup_prusa_connect, { Response::Yes, Response::No } },
        { PhaseNetworkSetup::prusa_conect_setup, { Response::Done } },

        { PhaseNetworkSetup::no_interface_error, { Response::Ok, Response::Help, Response::Retry } },
        { PhaseNetworkSetup::connection_error, { Response::Back, Response::Help, Response::Abort } },
        { PhaseNetworkSetup::help_qr, { Response::Back } },
        { PhaseNetworkSetup::finish, {} },
};
#endif

#if HAS_CRASH_DETECTION()
inline constexpr PhaseResponses CrashRecoveryResponses[] = {
    {}, // check X
    {}, // check Y
    {}, // home
    {}, // home_gcode_interrupt
    { Response::Retry, Response::Pause, Response::Resume }, // axis NOK
    {}, // axis short
    {}, // axis long
    { Response::Resume, Response::Pause }, // repeated crash
    { Response::Retry }, // home_fail
    #if HAS_TOOL_CRASH_RECOVERY() && HAS_DWARF()
    { Response::Continue }, // toolchanger recovery
    #endif
};
static_assert(std::size(ClientResponses::CrashRecoveryResponses) == CountPhases<PhasesCrashRecovery>());
#endif

inline constexpr PhaseResponses QuickPauseResponses[] = {
    { Response::Resume }, // QuickPaused
};
static_assert(std::size(ClientResponses::QuickPauseResponses) == CountPhases<PhasesQuickPause>());

inline constexpr EnumArray<PhasesWarning, PhaseResponses, CountPhases<PhasesWarning>()> WarningResponses {
#if HAS_EMERGENCY_STOP()
    { PhasesWarning::DoorOpen, {} },
#endif
#if HAS_HT_HOTEND()
        { PhasesWarning::HotendBurnRisk, {} },
#endif
        { PhasesWarning::Warning, { Response::Ok } },
#if XL_ENCLOSURE_SUPPORT() || HAS_CHAMBER_FILTRATION_API()
        { PhasesWarning::EnclosureFilterExpiration, { Response::Ignore, Response::Postpone5Days, Response::Done } },
#endif
#if HAS_CHAMBER_VENTS()
        { PhasesWarning::ChamberVents, { Response::Ok, Response::Disable } },
#endif
        { PhasesWarning::ProbingFailed, { Response::Yes, Response::No } },
        { PhasesWarning::FilamentSensorStuckHelp, { Response::Ok, Response::FS_disable } },
#if HAS_MMU2()
        { PhasesWarning::FilamentSensorStuckHelpMMU, { Response::Ok } },
#endif
#if HAS_PRINT_SHEET_DETECTION()
        { PhasesWarning::SteelSheetNotDetected, { Response::Retry, Response::Ignore, Response::Abort } },
#endif
#if HAS_CHAMBER_API()
        { PhasesWarning::FailedToReachChamberTemperature, { Response::Ok, Response::Skip } },
#endif
#if HAS_UNEVEN_BED_PROMPT()
        { PhasesWarning::BedUnevenAlignmentPrompt, { Response::Yes, Response::No } },
#endif
#if HAS_CEILING_CLEARANCE()
        { PhasesWarning::CeilingClearanceViolation, { Response::Continue, Response::Abort } },
#endif
#if HAS_PRECISE_HOMING_COREXY()
        { PhasesWarning::HomingCalibrationNeeded, { Response::Calibrate, Response::Skip, Response::Always, Response::Never } },
        { PhasesWarning::HomingRefinementFailed, { Response::Retry, Response::Abort, Response::Ignore } },
        { PhasesWarning::HomingCalibrationFromMenuNeeded, { Response::Abort, Response::Ignore } },
#endif
#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
        { PhasesWarning::DisplayProblemDetected, { Response::Yes, Response::No } },
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        { PhasesWarning::ToolOffsetCalibrationFailed, { Response::Retry, Response::Abort } },
        { PhasesWarning::HotendOffsetUnsafeZDeviation, { Response::Retry, Response::Abort } },
        { PhasesWarning::HotendOffsetUnsafeXyDeviation, { Response::Retry, Response::Abort } },
        { PhasesWarning::HotendOffsetUnsafeSensorXY, { Response::Retry, Response::Abort } },
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
        { PhasesWarning::NozzleCleanerFull, { Response::Ignore, Response::Done } },
        { PhasesWarning::NozzleCleanerManualEmpty, { Response::Done } },
#endif
        { PhasesWarning::MetricsConfigChangePrompt, { Response::Yes, Response::No } },
};

#if HAS_COLDPULL()
inline constexpr PhaseResponses ColdPullResponses[] = {
    { Response::Continue, Response::Stop }, // introduction,
    #if HAS_TOOLCHANGER()
    {}, // select_tool; selected tool passed through FSMResponseVariant as PhysicalToolIndex
    {}, // pick_tool
    #endif
    #if HAS_MMU2()
    { Response::Abort }, // stop_mmu,
    #endif
    #if HAS_TOOLCHANGER() || HAS_MMU2()
    { Response::Unload, Response::Continue, Response::Abort }, // unload_ptfe,
    { Response::Load, Response::Continue, Response::Abort }, // load_ptfe,
    #endif
    { Response::Unload, Response::Load, Response::Continue, Response::Abort }, // prepare_filament,
    #if HAS_AUTO_RETRACT()
    { Response::Abort }, // deretract
    #endif
    {}, // blank_load
    {}, // blank_unload
    { Response::Abort }, // cool_down,
    { Response::Abort }, // heat_up,
    {}, // automatic_pull,
    { Response::Continue }, // manual_pull,
    { Response::Abort }, // cleanup (restart_mmu),
    { Response::Finish }, // pull_done,
    {}, // finish,
};
static_assert(std::size(ClientResponses::ColdPullResponses) == CountPhases<PhasesColdPull>());
#endif

#if HAS_PHASE_STEPPING_CALIBRATION()
inline constexpr EnumArray<PhasesPhaseStepping, PhaseResponses, CountPhases<PhasesPhaseStepping>()> phase_stepping_calibration_responses {
    { PhasesPhaseStepping::restore_defaults, { Response::Ok } },
        { PhasesPhaseStepping::intro, { Response::Continue, Response::Abort } },
        { PhasesPhaseStepping::home, {} },
    #if HAS_ATTACHABLE_ACCELEROMETER()
        { PhasesPhaseStepping::connect_to_board, { Response::Abort } },
        { PhasesPhaseStepping::wait_for_extruder_temperature, { Response::Abort } },
        { PhasesPhaseStepping::attach_to_extruder, { Response::Continue, Response::Abort } },
        { PhasesPhaseStepping::attach_to_bed, { Response::Continue, Response::Abort } },
    #endif
        { PhasesPhaseStepping::calib_x, { Response::Abort } },
        { PhasesPhaseStepping::calib_y, { Response::Abort } },
        { PhasesPhaseStepping::calib_error, { Response::Ok } },
        { PhasesPhaseStepping::calib_nok, { Response::Ok } },
        { PhasesPhaseStepping::calib_ok, { Response::Ok } },
        { PhasesPhaseStepping::abort, {} },
        { PhasesPhaseStepping::finish, {} },
};
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
inline constexpr EnumArray<PhasesInputShaperCalibration, PhaseResponses, CountPhases<PhasesInputShaperCalibration>()> input_shaper_calibration_responses {
    #if HAS_ATTACHABLE_ACCELEROMETER()
    { PhasesInputShaperCalibration::info, { Response::Continue, Response::Abort } },
        { PhasesInputShaperCalibration::connect_to_board, { Response::Abort } },
        { PhasesInputShaperCalibration::wait_for_extruder_temperature, { Response::Abort } },
        { PhasesInputShaperCalibration::attach_to_extruder, { Response::Continue, Response::Abort } },
        { PhasesInputShaperCalibration::attach_to_bed, { Response::Continue, Response::Abort } },
    #endif
        { PhasesInputShaperCalibration::parking, {} },
        { PhasesInputShaperCalibration::measuring_x_axis, { Response::Abort } },
        { PhasesInputShaperCalibration::measuring_y_axis, { Response::Abort } },
        { PhasesInputShaperCalibration::measurement_failed, { Response::Retry, Response::Abort } },
        { PhasesInputShaperCalibration::computing, { Response::Abort } },
        { PhasesInputShaperCalibration::bad_results, { Response::Ok } },
        { PhasesInputShaperCalibration::results, { Response::Yes, Response::No } },
        { PhasesInputShaperCalibration::abort, {} },
        { PhasesInputShaperCalibration::finish, {} },
};
#endif

#if HAS_DOOR_SENSOR_CALIBRATION()
inline constexpr EnumArray<PhaseDoorSensorCalibration, PhaseResponses, CountPhases<PhaseDoorSensorCalibration>()> door_sensor_calibration_responses {
    { PhaseDoorSensorCalibration::confirm_abort, { Response::Back, Response::Skip } },
    { PhaseDoorSensorCalibration::repeat, { Response::Ok } },
    { PhaseDoorSensorCalibration::skip_ask, { Response::Calibrate, Response::Skip } },
    { PhaseDoorSensorCalibration::confirm_closed, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::tighten_screw_half, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::confirm_open, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::loosen_screw_half, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::finger_test, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::loosen_screw_quarter, { Response::Continue, Response::Abort } },
    { PhaseDoorSensorCalibration::ask_enable_safety_features, { Response::Yes, Response::No } },
    { PhaseDoorSensorCalibration::warn_disabled_sensor, { Response::Cancel, Response::Disable } }, // Cancel is first to have it selected (temporary solution till we rewrite the frames used to have radio button in as well to be able to change focus from inside the frame)
    { PhaseDoorSensorCalibration::done, { Response::Continue } },
    { PhaseDoorSensorCalibration::finish, {} },
};
#endif

#if HAS_INDX()
inline constexpr EnumArray<PhaseDockCalibration, PhaseResponses, CountPhases<PhaseDockCalibration>()> dock_calibration_responses {
    { PhaseDockCalibration::intro, { Response::Continue, Response::Abort } },
    { PhaseDockCalibration::remove_tool, { Response::Abort } },
    { PhaseDockCalibration::select_dock_count, { Response::Docks4, Response::Docks8, Response::Other } },
    { PhaseDockCalibration::select_docks, {} }, // per-dock actions passed through FSMResponseVariant as a DockSelection
    { PhaseDockCalibration::homing, {} },
    { PhaseDockCalibration::moving_away, {} },
    { PhaseDockCalibration::parking_tool, {} },
    { PhaseDockCalibration::tighten_silver_screws, { Response::Continue, Response::Abort } },
    { PhaseDockCalibration::ask_position_dock, { Response::Continue, Response::Abort } },
    { PhaseDockCalibration::lock_position, { Response::Continue, Response::Back, Response::Abort } },
    { PhaseDockCalibration::measuring, {} },
    { PhaseDockCalibration::loosen_each_bolt, { Response::Continue } },
    { PhaseDockCalibration::calibration_success, { Response::Continue } },
    { PhaseDockCalibration::calibration_failed, { Response::Retry, Response::Abort } },
};

inline constexpr EnumArray<PhaseNozzleCleanerCalibration, PhaseResponses, CountPhases<PhaseNozzleCleanerCalibration>()> nozzle_cleaner_calibration_responses {
    { PhaseNozzleCleanerCalibration::intro, { Response::Continue, Response::Abort } },
    { PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown, { Response::Abort } },
    { PhaseNozzleCleanerCalibration::picking_tool, {} },
    { PhaseNozzleCleanerCalibration::homing, {} },
    { PhaseNozzleCleanerCalibration::moving_away, {} },
    { PhaseNozzleCleanerCalibration::move_to_z_point, { Response::Continue, Response::Abort } },
    { PhaseNozzleCleanerCalibration::ask_position_x, { Response::Continue, Response::Abort } },
    { PhaseNozzleCleanerCalibration::lock_position_x, { Response::Continue, Response::Back, Response::Abort } },
    { PhaseNozzleCleanerCalibration::measuring_x, {} },
    { PhaseNozzleCleanerCalibration::evaluating_x, { Response::Yes, Response::Retry, Response::Abort } },
    { PhaseNozzleCleanerCalibration::clean_nozzle, { Response::Retry, Response::Abort } },
    { PhaseNozzleCleanerCalibration::ask_position_y, { Response::Continue, Response::Abort } },
    { PhaseNozzleCleanerCalibration::lock_position_y, { Response::Continue, Response::Back, Response::Abort } },
    { PhaseNozzleCleanerCalibration::measuring_y, {} },
    { PhaseNozzleCleanerCalibration::evaluating_y, { Response::Yes, Response::Retry, Response::Abort } },
    { PhaseNozzleCleanerCalibration::calibration_success, { Response::Continue } },
};
#endif
#if HAS_TOOL_OFFSET_SENSOR()
inline constexpr EnumArray<PhaseToolOffsetsCalibration, PhaseResponses, CountPhases<PhaseToolOffsetsCalibration>()> tool_offsets_calibration_responses {
    { PhaseToolOffsetsCalibration::intro, { Response::Continue, Response::Abort } },
    #if HAS_TOOL_OFFSET_NOZZLE_CLEANING_WIZARD()
        { PhaseToolOffsetsCalibration::clean_nozzles_cold, { Response::Heatup, Response::Continue, Response::Abort } },
        { PhaseToolOffsetsCalibration::clean_nozzles_hot, { Response::Cooldown, Response::Continue, Response::Abort } },
    #else
        { PhaseToolOffsetsCalibration::ensure_nozzles_clean, { Response::Continue, Response::Abort } },
    #endif
        { PhaseToolOffsetsCalibration::moving_away, {} },
        { PhaseToolOffsetsCalibration::homing, {} },
        { PhaseToolOffsetsCalibration::picking_tool, {} },
        { PhaseToolOffsetsCalibration::calibrating, { Response::Abort } },
        { PhaseToolOffsetsCalibration::calibration_success, { Response::Continue } },
        { PhaseToolOffsetsCalibration::calibration_failed, { Response::Continue } },
};
#endif

extern constinit const EnumArray<ClientFSM, std::span<const PhaseResponses>, ClientFSM::_count> fsm_phase_responses;

inline constexpr const PhaseResponses &get_fsm_responses(ClientFSM fsm_type, PhaseUnderlyingType phase) {
    if (std::to_underlying(fsm_type) >= fsm_phase_responses.size()) {
        return empty_phase_responses;
    }

    const auto &responses = fsm_phase_responses[fsm_type];
    if (phase >= responses.size()) {
        return empty_phase_responses;
    }

    return responses[phase];
}

// get all responses accepted in phase
inline constexpr const PhaseResponses &get_available_responses(FSMAndPhase fsm_phase) {
    return get_fsm_responses(fsm_phase.fsm, fsm_phase.phase);
}

// get index of single response in PhaseResponses
// return -1 (maxval) if does not exist
inline constexpr uint8_t GetIndex(FSMAndPhase fsm_phase, Response response) {
    const auto responses = fsm_phase_responses[fsm_phase.fsm];
    if (fsm_phase.phase >= responses.size()) {
        return -1;
    }

    const PhaseResponses &cmds = responses[fsm_phase.phase];
    for (size_t i = 0; i < MAX_RESPONSES; ++i) {
        if (cmds[i] == response) {
            return i;
        }
    }
    return -1;
}

// get response from PhaseResponses by index
inline constexpr const Response &get_available_response(FSMAndPhase phase, const uint8_t index) {
    if (index >= MAX_RESPONSES) {
        return ResponseNone;
    }
    return get_available_responses(phase)[index];
}

template <class T>
inline bool has_available_responses(const T phase) {
    return get_available_response(phase, 0) != Response::_none; // this phase has no responses
}
} // namespace ClientResponses

#if HAS_SELFTEST()
enum class SelftestParts {
    Axis,
    #if HAS_LOADCELL()
    Loadcell,
    #endif
    CalibZ,
    Heaters,
    FirstLayer,
    FirstLayerQuestions,
    #if HAS_TOOLCHANGER() && !HAS_INDX()
    Dock,
    ToolOffsets,
    #endif
    RevisePrinterSetup,
    _none, // cannot be created, must have same index as _count
    _count = _none
};

inline constexpr PhasesSelftest SelftestGetFirstPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::Axis:
        return PhasesSelftest::_first_Axis;
    #if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return PhasesSelftest::_first_Loadcell;
    #endif
    case SelftestParts::CalibZ:
        return PhasesSelftest::_first_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_first_Heaters;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_first_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_first_FirstLayerQuestions;
    #if HAS_TOOLCHANGER() && !HAS_INDX()
    case SelftestParts::Dock:
        return PhasesSelftest::_first_Dock;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_first_Tool_Offsets;
    #endif
    case SelftestParts::RevisePrinterSetup:
        return PhasesSelftest::_first_RevisePrinterStatus;

    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

inline constexpr PhasesSelftest SelftestGetLastPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::Axis:
        return PhasesSelftest::_last_Axis;
    #if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return PhasesSelftest::_last_Loadcell;
    #endif
    case SelftestParts::CalibZ:
        return PhasesSelftest::_last_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_last_Heaters;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_last_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_last_FirstLayerQuestions;
    #if HAS_TOOLCHANGER() && !HAS_INDX()
    case SelftestParts::Dock:
        return PhasesSelftest::_last_Dock;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_last_Tool_Offsets;
    #endif
    case SelftestParts::RevisePrinterSetup:
        return PhasesSelftest::_last_RevisePrinterStatus;

    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

inline constexpr bool SelftestPartContainsPhase(SelftestParts part, PhasesSelftest ph) {
    const PhaseUnderlyingType ph_u16 = PhaseUnderlyingType(ph);

    return (ph_u16 >= PhaseUnderlyingType(SelftestGetFirstPhaseFromPart(part))) && (ph_u16 <= PhaseUnderlyingType(SelftestGetLastPhaseFromPart(part)));
}

inline constexpr SelftestParts SelftestGetPartFromPhase(PhasesSelftest ph) {
    for (size_t i = 0; i < size_t(SelftestParts::_none); ++i) {
        if (SelftestPartContainsPhase(SelftestParts(i), ph)) {
            return SelftestParts(i);
        }
    }

    #if HAS_LOADCELL()
    if (SelftestPartContainsPhase(SelftestParts::Loadcell, ph)) {
        return SelftestParts::Loadcell;
    }
    #endif

    if (SelftestPartContainsPhase(SelftestParts::Axis, ph)) {
        return SelftestParts::Axis;
    }

    if (SelftestPartContainsPhase(SelftestParts::Heaters, ph)) {
        return SelftestParts::Heaters;
    }

    if (SelftestPartContainsPhase(SelftestParts::CalibZ, ph)) {
        return SelftestParts::CalibZ;
    }

    #if BOARD_IS_XLBUDDY()
    if (SelftestPartContainsPhase(SelftestParts::Dock, ph)) {
        return SelftestParts::Dock;
    }
    #endif

    if (SelftestPartContainsPhase(SelftestParts::RevisePrinterSetup, ph)) {
        return SelftestParts::RevisePrinterSetup;
    }

    return SelftestParts::_none;
};
#endif // HAS_SELFTEST()
