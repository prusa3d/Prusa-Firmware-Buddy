/**
 * @file client_response.hpp
 * @brief every phase in dialog can have some buttons
 * buttons are generalized on this level as responses
 * because non GUI/WUI client can also use them
 * bound to ClientFSM in src/common/client_fsm_types.h
 */

#pragma once

#include "general_response.hpp"
#include "device/board.h"
#include <cstdint>
#include <cstddef>
#include <array>
#include "printers.h"
#include "option/has_loadcell.h"
#include "option/filament_sensor.h"
#include "option/has_toolchanger.h"
#include <option/has_mmu2.h>

enum { RESPONSE_BITS = 4, // number of bits used to encode response
    MAX_RESPONSES = (1 << RESPONSE_BITS) }; // maximum number of responses in one phase

using PhaseResponses = std::array<Response, MAX_RESPONSES>;

// count enum class members (if "_first" and "_last" is defined)
template <class T>
constexpr size_t CountPhases() {
    return static_cast<size_t>(T::_last) - static_cast<size_t>(T::_first) + 1;
}
// use this when creating an event
// encodes enum to position in phase
template <class T>
constexpr uint8_t GetPhaseIndex(T phase) {
    return static_cast<size_t>(phase) - static_cast<size_t>(T::_first);
}

template <class T>
constexpr T GetEnumFromPhaseIndex(size_t index) {
    return static_cast<T>(static_cast<size_t>(T::_first) + index);
}

// define enum classes for responses here
// and YES phase can have 0 responses
// every enum must have "_first" and "_last"
//"_first" ==  "previous_enum::_last" + 1
// EVERY response shall have a unique ID (so every button in GUI is unique)
enum class PhasesLoadUnload : uint16_t {
    _first = 0,
    ChangingTool,
    Parking_stoppable,
    Parking_unstoppable,
    WaitingTemp_stoppable,
    WaitingTemp_unstoppable,
    PreparingToRam_stoppable,
    PreparingToRam_unstoppable,
    Ramming_stoppable,
    Ramming_unstoppable,
    Unloading_stoppable,
    Unloading_unstoppable,
    RemoveFilament,
    IsFilamentUnloaded,
    FilamentNotInFS,
    ManualUnload,
    UserPush_stoppable,
    UserPush_unstoppable,
    MakeSureInserted_stoppable,
    MakeSureInserted_unstoppable,
    Inserting_stoppable,
    Inserting_unstoppable,
    IsFilamentInGear,
    Ejecting_stoppable,
    Ejecting_unstoppable,
    Loading_stoppable,
    Loading_unstoppable,
    Purging_stoppable,
    Purging_unstoppable,
    IsColor,
    IsColorPurge,
    Unparking,

#if HAS_LOADCELL()
    FilamentStuck,
#endif

#if HAS_MMU2()
    // MMU-specific dialogs
    LoadFilamentIntoMMU,
    // internal states of the MMU
    MMU_EngagingIdler,
    MMU_DisengagingIdler,
    MMU_UnloadingToFinda,
    MMU_UnloadingToPulley,
    MMU_FeedingToFinda,
    MMU_FeedingToBondtech,
    MMU_FeedingToNozzle,
    MMU_AvoidingGrind,
    MMU_FinishingMoves,
    MMU_ERRDisengagingIdler,
    MMU_ERREngagingIdler,
    MMU_ERRWaitingForUser,
    MMU_ERRInternal,
    MMU_ERRHelpingFilament,
    MMU_ERRTMCFailed,
    MMU_UnloadingFilament,
    MMU_LoadingFilament,
    MMU_SelectingFilamentSlot,
    MMU_PreparingBlade,
    MMU_PushingFilament,
    MMU_PerformingCut,
    MMU_ReturningSelector,
    MMU_ParkingSelector,
    MMU_EjectingFilament,
    MMU_RetractingFromFinda,
    MMU_Homing,
    MMU_MovingSelector,
    MMU_FeedingToFSensor,
    MMU_HWTestBegin,
    MMU_HWTestIdler,
    MMU_HWTestSelector,
    MMU_HWTestPulley,
    MMU_HWTestCleanup,
    MMU_HWTestExec,
    MMU_HWTestDisplay,
    MMU_ErrHwTestFailed,
#endif

#if HAS_LOADCELL() && HAS_MMU2()
    _last = MMU_ErrHwTestFailed
#elif HAS_LOADCELL()
    _last = FilamentStuck
#elif HAS_MMU2()
    _last = MMU_ErrHwTestFailed
#else
    _last = Unparking
#endif
};

enum class PhasesPreheat : uint16_t {
    _first = static_cast<uint16_t>(PhasesLoadUnload::_last) + 1,
    UserTempSelection,
    _last = UserTempSelection
};

enum class PhasesPrintPreview : uint16_t {
    _first = static_cast<uint16_t>(PhasesPreheat::_last) + 1,
    loading = _first,
    download_wait,
    main_dialog,
    unfinished_selftest,
    new_firmware_available,
    wrong_printer,
    wrong_printer_abort,
    filament_not_inserted,
    mmu_filament_inserted,
    tools_mapping,
    wrong_filament,
    file_error, ///< Something is wrong with the gcode file
    _last = file_error
};

// GUI phases of selftest/wizard
enum class PhasesSelftest : uint16_t {
    _first = static_cast<uint16_t>(PhasesPrintPreview::_last) + 1,
    _none = _first,

    _first_WizardPrologue,
    WizardPrologue_ask_run = _first_WizardPrologue,
    WizardPrologue_ask_run_dev, // developer version has ignore button
    WizardPrologue_info,
    WizardPrologue_info_detailed,
    _last_WizardPrologue = WizardPrologue_info_detailed,

    _first_Fans,
    Fans = _first_Fans,
    _last_Fans = Fans,

    _first_Loadcell,
    Loadcell_prepare = _first_Loadcell,
    Loadcell_move_away,
    Loadcell_tool_select,
    Loadcell_cooldown,
    Loadcell_user_tap_ask_abort,
    Loadcell_user_tap_countdown,
    Loadcell_user_tap_check,
    Loadcell_user_tap_ok,
    Loadcell_fail,
    _last_Loadcell = Loadcell_fail,

    _first_FSensor,
    FSensor_ask_unload = _first_FSensor,
    FSensor_wait_tool_pick,
    FSensor_unload_confirm,
    FSensor_calibrate,
    FSensor_insertion_wait,
    FSensor_insertion_ok,
    FSensor_insertion_calibrate,
    Fsensor_enforce_remove,
    FSensor_done,
    FSensor_fail,
    _last_FSensor = FSensor_fail,

    _first_GearsCalib,
    GearsCalib_filament_check = _first_GearsCalib,
    GearsCalib_filament_loaded_ask_unload,
    GearsCalib_filament_unknown_ask_unload,
    GearsCalib_release_screws,
    GearsCalib_alignment,
    GearsCalib_tighten,
    GearsCalib_done,
    _last_GearsCalib = GearsCalib_done,

    _first_CalibZ,
    CalibZ = _first_CalibZ,
    _last_CalibZ = CalibZ,

    _first_Axis,
    Axis = _first_Axis,
    _last_Axis = Axis,

    _first_Heaters,
    Heaters = _first_Heaters,
    HeatersDisabledDialog,
    _last_Heaters = HeatersDisabledDialog,

    _first_SpecifyHotEnd,
    SpecifyHotEnd = _first_SpecifyHotEnd,
    SpecifyHotEnd_sock,
    SpecifyHotEnd_nozzle_type,
    SpecifyHotEnd_retry,
    _last_SpecifyHotEnd = SpecifyHotEnd_retry,

    _first_FirstLayer,
    FirstLayer_mbl = _first_FirstLayer,
    FirstLayer_print,
    _last_FirstLayer = FirstLayer_print,

    _first_FirstLayerQuestions,
    FirstLayer_filament_known_and_not_unsensed = _first_FirstLayerQuestions,
    FirstLayer_filament_not_known_or_unsensed,
    FirstLayer_calib,
    FirstLayer_use_val,
    FirstLayer_start_print,
    FirstLayer_reprint,
    FirstLayer_clean_sheet,
    FirstLayer_failed,
    _last_FirstLayerQuestions = FirstLayer_failed,

    _first_Dock,
    Dock_needs_calibration = _first_Dock,
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
    _last_Dock = Dock_calibration_success,

    _first_Tool_Offsets,
    ToolOffsets_wait_user_confirm_start = _first_Tool_Offsets,
    ToolOffsets_wait_user_clean_nozzle_cold,
    ToolOffsets_wait_user_clean_nozzle_hot,
    ToolOffsets_wait_user_install_sheet,
    ToolOffsets_pin_install_prepare,
    ToolOffsets_wait_user_install_pin,
    ToolOffsets_wait_stable_temp,
    ToolOffsets_wait_calibrate,
    ToolOffsets_wait_move_away,
    ToolOffsets_wait_user_remove_pin,
    _last_Tool_Offsets = ToolOffsets_wait_user_remove_pin,

    _first_Result,
    Result = _first_Result,
    _last_Result = Result,

    _first_WizardEpilogue_ok,
    WizardEpilogue_ok = _first_WizardEpilogue_ok, // ok is after result
    _last_WizardEpilogue_ok = WizardEpilogue_ok,

    _first_WizardEpilogue_nok,
    WizardEpilogue_nok = _first_WizardEpilogue_nok, // nok is before result
    _last_WizardEpilogue_nok = WizardEpilogue_nok,

    _last = _last_WizardEpilogue_nok
};

enum class PhasesESP : uint16_t {
    _first = static_cast<uint16_t>(PhasesSelftest::_last) + 1,
    _none = _first,

    _first_ESP,
    ESP_instructions = _first_ESP,
    ESP_USB_not_inserted, // must be before ask_gen/ask_gen_overwrite, because it depends on file existence
    ESP_ask_gen,
    ESP_ask_gen_overwrite,
    ESP_makefile_failed,
    ESP_eject_USB,
    ESP_insert_USB,
    ESP_invalid,
    ESP_uploading_config,
    ESP_enabling_WIFI,
    _last_ESP = ESP_enabling_WIFI,

    _first_ESP_progress,
    ESP_progress_info = _first_ESP_progress,
    ESP_progress_upload,
    ESP_progress_passed,
    ESP_progress_failed,
    _last_ESP_progress = ESP_progress_failed,

    _first_ESP_qr,
    ESP_qr_instructions_flash = _first_ESP_qr,
    ESP_qr_instructions,
    _last_ESP_qr = ESP_qr_instructions,

    _last = _last_ESP_qr,
};

enum class PhasesCrashRecovery : uint16_t {
    _first = static_cast<uint16_t>(PhasesESP::_last) + 1,
    check_X = _first, // in this case is safe to have check_X == _first
    check_Y,
    home,
    axis_NOK, //< just for unification of the two below
    axis_short,
    axis_long,
    repeated_crash,
    home_fail, //< Homing failed, ask to retry
    tool_recovery, //< Toolchanger recovery, tool fell off
    _last = tool_recovery
};

enum class PhasesQuickPause : uint16_t {
    _first = static_cast<uint16_t>(PhasesCrashRecovery::_last) + 1,
    QuickPaused = _first,
    _last = QuickPaused
};

// static class for work with fsm responses (like button click)
// encode responses - get them from marlin client, to marlin server and decode them again
class ClientResponses {
    ClientResponses() = delete;
    ClientResponses(ClientResponses &) = delete;

    // declare 2d arrays of single buttons for radio buttons
    static constexpr PhaseResponses LoadUnloadResponses[] = {
        {}, //_first
        {}, // ChangingTool,
        { Response::Stop }, // Parking_stoppable
        {}, // Parking_unstoppable,
        { Response::Stop }, // WaitingTemp_stoppable,
        {}, // WaitingTemp_unstoppable,
        { Response::Stop }, // PreparingToRam_stoppable,
        {}, // PreparingToRam_unstoppable
        { Response::Stop }, // Ramming_stoppable,
        {}, // Ramming_unstoppable,
        { Response::Stop }, // Unloading_stoppable,
        {}, // Unloading_unstoppable,
        { Response::Filament_removed }, // RemoveFilament,
        { Response::Yes, Response::No }, // IsFilamentUnloaded,
        {}, // FilamentNotInFS
        { Response::Continue, Response::Retry }, // ManualUnload,
        { Response::Continue, Response::Stop }, // UserPush_stoppable,
        { Response::Continue }, // UserPush_unstoppable,
        { Response::Stop }, // MakeSureInserted_stoppable,
        {}, // MakeSureInserted_unstoppable,
        { Response::Stop }, // Inserting_stoppable,
        {}, // Inserting_unstoppable,
        { Response::Yes, Response::No }, // IsFilamentInGear,
        { Response::Stop }, // Ejecting_stoppable,
        {}, // Ejecting_unstoppable,
        { Response::Stop }, // Loading_stoppable,
        {}, // Loading_unstoppable,
        { Response::Stop }, // Purging_stoppable,
        {}, // Purging_unstoppable,
        { Response::Yes, Response::Purge_more, Response::Retry }, // IsColor,
        { Response::Yes, Response::Purge_more }, // IsColorPurge
        {}, // Unparking
#if HAS_LOADCELL()
        { Response::Unload }, // FilamentStuck
#endif
#if HAS_MMU2()
        { Response::Continue }, // LoadFilamentIntoMMU,

        {}, // MMU_EngagingIdler,
        {}, // MMU_DisengagingIdler,
        {}, // MMU_UnloadingToFinda,
        {}, // MMU_UnloadingToPulley,
        {}, // MMU_FeedingToFinda,
        {}, // MMU_FeedingToBondtech,
        {}, // MMU_FeedingToNozzle,
        {}, // MMU_AvoidingGrind,
        {}, // MMU_FinishingMoves,
        {}, // MMU_ERRDisengagingIdler,
        {}, // MMU_ERREngagingIdler,
        { Response::Retry, Response::Slowly, Response::Continue, Response::Restart,
            Response::Unload, Response::Stop, Response::MMU_disable }, // MMU_ERRWaitingForUser,
        {}, // MMU_ERRInternal,
        {}, // MMU_ERRHelpingFilament,
        {}, // MMU_ERRTMCFailed,
        {}, // MMU_UnloadingFilament,
        {}, // MMU_LoadingFilament,
        {}, // MMU_SelectingFilamentSlot,
        {}, // MMU_PreparingBlade,
        {}, // MMU_PushingFilament,
        {}, // MMU_PerformingCut,
        {}, // MMU_ReturningSelector,
        {}, // MMU_ParkingSelector,
        {}, // MMU_EjectingFilament,
        {}, // MMU_RetractingFromFinda,
        {}, // MMU_Homing,
        {}, // MMU_MovingSelector,
        {}, // MMU_FeedingToFSensor,
        {}, // MMU_HWTestBegin,
        {}, // MMU_HWTestIdler,
        {}, // MMU_HWTestSelector,
        {}, // MMU_HWTestPulley,
        {}, // MMU_HWTestCleanup,
        {}, // MMU_HWTestExec,
        {}, // MMU_HWTestDisplay,
        {}, // MMU_ErrHwTestFailed,
#endif
    };
    static_assert(std::size(ClientResponses::LoadUnloadResponses) == CountPhases<PhasesLoadUnload>());

    static constexpr PhaseResponses PreheatResponses[] = {
        {}, //_first
        { Response::Abort, Response::Cooldown, Response::PLA, Response::PETG,
#if PRINTER_IS_PRUSA_iX
            Response::PETG_NH,
#endif
            Response::ASA, Response::ABS, Response::PC, Response::FLEX, Response::HIPS, Response::PP, Response::PVB, Response::PA }, // UserTempSelection
    };
    static_assert(std::size(ClientResponses::PreheatResponses) == CountPhases<PhasesPreheat>());

    static constexpr PhaseResponses PrintPreviewResponses[] = {
        {}, // loading
        { Response::Quit }, // download_wait
        {
#if PRINTER_IS_PRUSA_XL
            Response::Continue,
#elif defined(USE_ST7789)
            Response::PRINT,
#else
            Response::Print,
#endif
            Response::Back }, // main_dialog,
        { Response::Continue, Response::Abort }, // unfinished_selftest
        { Response::Continue }, // new_firmware_available
        { Response::Abort, Response::PRINT }, // wrong_printer
        { Response::Abort }, // wrong_printer_abort
        { Response::Yes, Response::No, Response::FS_disable }, // filament_not_inserted
        { Response::Yes, Response::No }, // mmu_filament_inserted
        { Response::Back, Response::Filament, Response::PRINT }, // tools_mapping
        {
#if !PRINTER_IS_PRUSA_XL
            Response::Change,
#endif
            Response::Ok, Response::Abort }, // wrong_filament
        { Response::Abort }, // file_error
    };
    static_assert(std::size(ClientResponses::PrintPreviewResponses) == CountPhases<PhasesPrintPreview>());

    static constexpr PhaseResponses SelftestResponses[] = {
        {}, // _none == _first

        { Response::Continue, Response::Cancel }, // WizardPrologue_ask_run
        { Response::Continue, Response::Cancel
#if not PRINTER_IS_PRUSA_MINI
            ,
            Response::Ignore
#endif
        }, // WizardPrologue_ask_run_dev
        { Response::Continue, Response::Cancel }, // WizardPrologue_info
        { Response::Continue, Response::Cancel }, // WizardPrologue_info_detailed

        {}, // Fans

        {}, // Loadcell_prepare
        {}, // Loadcell_move_away
        {}, // Loadcell_tool_select
        { Response::Abort }, // Loadcell_cooldown

        { Response::Continue, Response::Abort }, // Loadcell_user_tap_ask_abort
        {}, // Loadcell_user_tap_countdown
        {}, // Loadcell_user_tap_check
        {}, // Loadcell_user_tap_ok
        {}, // Loadcell_fail

        { Response::Continue, Response::Unload, Response::Abort }, // FSensor_ask_unload
        {}, // FSensor_wait_tool_pick
        { Response::Yes, Response::No }, // FSensor_unload_confirm
        {}, // FSensor_calibrate
        { Response::Abort_invalidate_test }, // FSensor_insertion_wait
        { Response::Continue, Response::Abort_invalidate_test }, // FSensor_insertion_ok
        { Response::Abort_invalidate_test }, // FSensor_insertion_calibrate
        { Response::Abort_invalidate_test }, // Fsensor_enforce_remove
        {}, // FSensor_done
        {}, // FSensor_fail

        { Response::Continue, Response::Skip }, // GearsCalib_filament_check
        { Response::Unload, Response::Abort }, // GearsCalib_filament_loaded_ask_unload
        { Response::Continue, Response::Unload, Response::Abort }, // GearsCalib_filament_unknown_ask_unload
        { Response::Continue, Response::Skip }, // GearsCalib_release_screws
        {}, // GearsCalib_alignment
        { Response::Continue }, // GearsCalib_tighten
        { Response::Continue }, // GearsCalib_done

        {}, // CalibZ

        {}, // Axis

        {}, // Heaters
        { Response::Ok }, // HeatersDisabledDialog

        { Response::Adjust, Response::Skip }, // SpecifyHotEnd
        { Response::Yes, Response::No }, // SpecifyHotEnd_sock
        { Response::PrusaStock, Response::HighFlow }, // SpecifyHotEnd_nozzle_type
        { Response::Yes, Response::No }, // SpecifyHotEnd_retry

        {}, // FirstLayer_mbl
        {}, // FirstLayer_print

        { Response::Next, Response::Unload }, // FirstLayer_filament_known_and_not_unsensed = _first_FirstLayerQuestions
        { Response::Next, Response::Load, Response::Unload }, // FirstLayer_filament_not_known_or_unsensed
        { Response::Next }, // FirstLayer_calib
        { Response::Yes, Response::No }, // FirstLayer_use_val
        { Response::Next }, // FirstLayer_start_print
        { Response::Yes, Response::No }, // FirstLayer_reprint
        { Response::Next }, // FirstLayer_clean_sheet
        { Response::Next }, // FirstLayer_failed

        { Response::Continue, Response::Abort }, // Dock_needs_calibartion
        {}, // Dock_move_away
        { Response::Continue, Response::Abort }, // Dock_wait_user_park1
        { Response::Continue, Response::Abort }, // Dock_wait_user_park2
        { Response::Continue, Response::Abort }, // Dock_wait_user_park3
        { Response::Continue, Response::Abort }, // Dock_wait_user_remove_pins
        { Response::Continue, Response::Abort }, // Dock_wait_user_loosen_pillar
        { Response::Continue, Response::Abort }, // Dock_wait_user_lock_tool
        { Response::Continue, Response::Abort }, // Dock_wait_user_tighten_top_screw
        { Response::Abort }, // Dock_measure
        { Response::Continue, Response::Abort }, // Dock_wait_user_tighten_bottom_screw
        { Response::Continue, Response::Abort }, // Dock_wait_user_install_pins
        { Response::Abort }, // Dock_selftest_park_test
        {}, // Dock_selftest_failed
        { Response::Continue }, // Dock_calibration_success

        { Response::Continue, Response::Abort }, // ToolOffsets_wait_user_confirm_start
        { Response::Heatup, Response::Continue }, // ToolOffsets_wait_user_clean_nozzle_cold
        { Response::Cooldown, Response::Continue }, // ToolOffsets_wait_user_clean_nozzle_hot
        { Response::Continue }, // ToolOffsets_wait_user_install_sheet
        {}, // ToolOffsets_pin_install_prepare
        { Response::Continue }, // ToolOffsets_wait_user_install_pin
        {}, // ToolOffsets_wait_stable_temp
        {}, // ToolOffsets_wait_calibrate
        {}, // ToolOffsets_state_final_park
        { Response::Continue }, // ToolOffsets_wait_user_remove_pin

        { Response::Next }, // Result

        { Response::Continue }, // WizardEpilogue_ok
        { Response::Continue }, // WizardEpilogue_nok
    };
    static_assert(std::size(ClientResponses::SelftestResponses) == CountPhases<PhasesSelftest>());

    static constexpr PhaseResponses ESPResponses[] = {
        {}, // _none == _first

        { Response::Continue, Response::Abort }, // ESP_instructions
        { Response::Yes, Response::Skip }, // ESP_USB_not_inserted
        { Response::Yes, Response::Skip }, // ESP_ask_gen
        { Response::Yes, Response::Skip }, // ESP_ask_gen_overwrite
        { Response::Yes, Response::Skip }, // ESP_makefile_failed
        { Response::Continue }, // ESP_eject_USB
        { Response::Continue, Response::Abort }, // ESP_insert_USB
        { Response::Retry, Response::Abort }, // ESP_invalid
        { Response::Abort }, // ESP_uploading_config
        { Response::Yes, Response::No }, // ESP_enabling_WIFI

        { Response::Continue, Response::Abort }, // ESP_progress_info
        { Response::Abort }, // ESP_progress_upload
        { Response::Continue }, // ESP_progress_passed
        { Response::Continue }, // ESP_progress_failed
        { Response::Continue, Response::NotNow
#if not PRINTER_IS_PRUSA_MINI
            ,
            Response::Never
#endif
        }, // ESP_qr_instructions_flash
        { Response::Continue, Response::Abort }, // ESP_qr_instructions
    };
    static_assert(std::size(ClientResponses::ESPResponses) == CountPhases<PhasesESP>());

    static constexpr PhaseResponses CrashRecoveryResponses[] = {
        {}, // check X == _first
        {}, // check Y
        {}, // home
        { Response::Retry, Response::Pause, Response::Resume }, // axis NOK
        {}, // axis short
        {}, // axis long
        { Response::Resume, Response::Pause }, // repeated crash
        { Response::Retry }, // home_fail
        { Response::Continue }, // toolchanger recovery
    };
    static_assert(std::size(ClientResponses::CrashRecoveryResponses) == CountPhases<PhasesCrashRecovery>());

    static constexpr PhaseResponses QuickPauseResponses[] = {
        { Response::Resume }, // QuickPaused
    };
    static_assert(std::size(ClientResponses::QuickPauseResponses) == CountPhases<PhasesQuickPause>());

    // methods to "bind" button array with enum type
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesLoadUnload phase) { return LoadUnloadResponses[static_cast<size_t>(phase)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesPreheat phase) { return PreheatResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesPreheat::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesPrintPreview phase) { return PrintPreviewResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesPrintPreview::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesSelftest phase) { return SelftestResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesSelftest::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesESP phase) { return ESPResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesESP::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesCrashRecovery phase) { return CrashRecoveryResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesCrashRecovery::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesQuickPause phase) { return QuickPauseResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesQuickPause::_first)]; }

public:
    // get index of single response in PhaseResponses
    // return -1 (maxval) if does not exist
    template <class T>
    static uint8_t GetIndex(T phase, Response response) {
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        for (size_t i = 0; i < MAX_RESPONSES; ++i) {
            if (cmds[i] == response) {
                return i;
            }
        }
        return -1;
    }

    // get response from PhaseResponses by index
    template <class T>
    static const Response &GetResponse(const T phase, const uint8_t index) {
        if (index >= MAX_RESPONSES) {
            return ResponseNone;
        }
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        return cmds[index];
    }

    // get all responses accepted in phase
    template <class T>
    static constexpr const PhaseResponses &GetResponses(const T phase) {
        return getResponsesInPhase(phase);
    }
    template <class T>
    static bool HasButton(const T phase) {
        return GetResponse(phase, 0) != Response::_none; // this phase has no responses
    }

    // encode phase and client response (in GUI radio button and clicked index) into int
    // use on client side
    // return -1 (maxval) if does not exist
    template <class T>
    static uint32_t Encode(T phase, Response response) {
        uint8_t clicked_index = GetIndex(phase, response);
        if (clicked_index >= MAX_RESPONSES) {
            return -1; // this phase does not have response with this index
        }
        return ((static_cast<uint32_t>(phase)) << RESPONSE_BITS) + uint32_t(clicked_index);
    }
};

enum class SelftestParts {
    WizardPrologue,
    Axis,
    Fans,
#if HAS_LOADCELL()
    Loadcell,
#endif
    CalibZ,
    Heaters,
    SpecifyHotEnd,
#if FILAMENT_SENSOR_IS_ADC()
    FSensor,
#endif
#if PRINTER_IS_PRUSA_MK4
    GearsCalib,
#endif
    FirstLayer,
    FirstLayerQuestions,
    Result,
    WizardEpilogue_ok,
    WizardEpilogue_nok,
#if HAS_TOOLCHANGER()
    Dock,
    ToolOffsets,
#endif
    _none, // cannot be created, must have same index as _count
    _count = _none
};

enum class ESPParts {
    ESP,
    ESP_progress,
    ESP_qr,
    _none, // cannot be created, must have same index as _count
    _count = _none
};

static constexpr PhasesSelftest SelftestGetFirstPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return PhasesSelftest::_first_WizardPrologue;
    case SelftestParts::Axis:
        return PhasesSelftest::_first_Axis;
    case SelftestParts::Fans:
        return PhasesSelftest::_first_Fans;
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return PhasesSelftest::_first_Loadcell;
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
        return PhasesSelftest::_first_FSensor;
#endif
#if PRINTER_IS_PRUSA_MK4
    case SelftestParts::GearsCalib:
        return PhasesSelftest::_first_GearsCalib;
#endif
    case SelftestParts::CalibZ:
        return PhasesSelftest::_first_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_first_Heaters;
    case SelftestParts::SpecifyHotEnd:
        return PhasesSelftest::_first_SpecifyHotEnd;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_first_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_first_FirstLayerQuestions;
#if HAS_TOOLCHANGER()
    case SelftestParts::Dock:
        return PhasesSelftest::_first_Dock;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_first_Tool_Offsets;
#endif
    case SelftestParts::Result:
        return PhasesSelftest::_first_Result;
    case SelftestParts::WizardEpilogue_ok:
        return PhasesSelftest::_first_WizardEpilogue_ok;
    case SelftestParts::WizardEpilogue_nok:
        return PhasesSelftest::_first_WizardEpilogue_nok;
    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

static constexpr PhasesESP ESPGetFirstPhaseFromPart(ESPParts part) {
    switch (part) {
    case ESPParts::ESP:
        return PhasesESP::_first_ESP;
    case ESPParts::ESP_progress:
        return PhasesESP::_first_ESP_progress;
    case ESPParts::ESP_qr:
        return PhasesESP::_first_ESP_qr;
    case ESPParts::_none:
        break;
    }
    return PhasesESP::_none;
}

static constexpr PhasesSelftest SelftestGetLastPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return PhasesSelftest::_last_WizardPrologue;
    case SelftestParts::Axis:
        return PhasesSelftest::_last_Axis;
    case SelftestParts::Fans:
        return PhasesSelftest::_last_Fans;
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return PhasesSelftest::_last_Loadcell;
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
        return PhasesSelftest::_last_FSensor;
#endif
#if PRINTER_IS_PRUSA_MK4
    case SelftestParts::GearsCalib:
        return PhasesSelftest::_last_GearsCalib;
#endif
    case SelftestParts::CalibZ:
        return PhasesSelftest::_last_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_last_Heaters;
    case SelftestParts::SpecifyHotEnd:
        return PhasesSelftest::_last_SpecifyHotEnd;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_last_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_last_FirstLayerQuestions;
#if BOARD_IS_XLBUDDY
    case SelftestParts::Dock:
        return PhasesSelftest::_last_Dock;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_last_Tool_Offsets;
#endif
    case SelftestParts::Result:
        return PhasesSelftest::_last_Result;
    case SelftestParts::WizardEpilogue_ok:
        return PhasesSelftest::_last_WizardEpilogue_ok;
    case SelftestParts::WizardEpilogue_nok:
        return PhasesSelftest::_last_WizardEpilogue_nok;
    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

static constexpr PhasesESP ESPGetLastPhaseFromPart(ESPParts part) {
    switch (part) {
    case ESPParts::ESP:
        return PhasesESP::_last_ESP;
    case ESPParts::ESP_progress:
        return PhasesESP::_last_ESP_progress;
    case ESPParts::ESP_qr:
        return PhasesESP::_last_ESP_qr;
    case ESPParts::_none:
        break;
    }
    return PhasesESP::_none;
}

static constexpr bool SelftestPartContainsPhase(SelftestParts part, PhasesSelftest ph) {
    const uint16_t ph_u16 = uint16_t(ph);

    return (ph_u16 >= uint16_t(SelftestGetFirstPhaseFromPart(part))) && (ph_u16 <= uint16_t(SelftestGetLastPhaseFromPart(part)));
}

static constexpr SelftestParts SelftestGetPartFromPhase(PhasesSelftest ph) {
    for (size_t i = 0; i < size_t(SelftestParts::_none); ++i) {
        if (SelftestPartContainsPhase(SelftestParts(i), ph)) {
            return SelftestParts(i);
        }
    }

    if (SelftestPartContainsPhase(SelftestParts::WizardPrologue, ph)) {
        return SelftestParts::WizardPrologue;
    }

    if (SelftestPartContainsPhase(SelftestParts::Fans, ph)) {
        return SelftestParts::Fans;
    }

#if HAS_LOADCELL()
    if (SelftestPartContainsPhase(SelftestParts::Loadcell, ph)) {
        return SelftestParts::Loadcell;
    }
#endif
#if FILAMENT_SENSOR_IS_ADC()
    if (SelftestPartContainsPhase(SelftestParts::FSensor, ph)) {
        return SelftestParts::FSensor;
    }
#endif
#if PRINTER_IS_PRUSA_MK4
    if (SelftestPartContainsPhase(SelftestParts::GearsCalib, ph)) {
        return SelftestParts::GearsCalib;
    }
#endif
    if (SelftestPartContainsPhase(SelftestParts::Axis, ph)) {
        return SelftestParts::Axis;
    }

    if (SelftestPartContainsPhase(SelftestParts::Heaters, ph)) {
        return SelftestParts::Heaters;
    }

    if (SelftestPartContainsPhase(SelftestParts::SpecifyHotEnd, ph)) {
        return SelftestParts::SpecifyHotEnd;
    }

    if (SelftestPartContainsPhase(SelftestParts::CalibZ, ph)) {
        return SelftestParts::CalibZ;
    }

    if (SelftestPartContainsPhase(SelftestParts::WizardEpilogue_ok, ph)) {
        return SelftestParts::WizardEpilogue_ok;
    }

    if (SelftestPartContainsPhase(SelftestParts::WizardEpilogue_nok, ph)) {
        return SelftestParts::WizardEpilogue_nok;
    }

    if (SelftestPartContainsPhase(SelftestParts::Result, ph)) {
        return SelftestParts::Result;
    }

#if BOARD_IS_XLBUDDY
    if (SelftestPartContainsPhase(SelftestParts::Dock, ph)) {
        return SelftestParts::Dock;
    }

#endif
    return SelftestParts::_none;
};

static constexpr bool ESPPartContainsPhase(ESPParts part, PhasesESP ph) {
    const uint16_t ph_u16 = uint16_t(ph);

    return (ph_u16 >= uint16_t(ESPGetFirstPhaseFromPart(part))) && (ph_u16 <= uint16_t(ESPGetLastPhaseFromPart(part)));
}

static constexpr ESPParts ESPGetPartFromPhase(PhasesESP ph) {
    for (size_t i = 0; i < size_t(ESPParts::_none); ++i) {
        if (ESPPartContainsPhase(ESPParts(i), ph)) {
            return ESPParts(i);
        }
    }
    return ESPParts::_none;
}

enum class FSM_action {
    no_action,
    create,
    destroy,
    change
};

#include <optional>

/**
 * @brief determine if correct state of fsm is se
 * useful for FSM with no data
 *
 * @tparam T          - type of current / wanted state
 * @param current     - current state
 * @param should_be   - wanted state
 * @return FSM_action - what action needs to be done
 */
template <class T>
FSM_action IsFSM_action_needed(std::optional<T> current, std::optional<T> should_be) {
    if (!current && !should_be) {
        return FSM_action::no_action;
    }

    if (!current && should_be) {
        return FSM_action::create;
    }

    if (current && !should_be) {
        return FSM_action::destroy;
    }

    // current && should_be
    if (*current == *should_be) {
        return FSM_action::no_action;
    }

    return FSM_action::change;
}
