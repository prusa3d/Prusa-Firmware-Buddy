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

enum { RESPONSE_BITS = 4,                   // number of bits used to encode response
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

#if HAS_MMU2
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

    _last = MMU_RetractingFromFinda
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
    main_dialog = _first,
    wrong_printer,
    wrong_printer_abort,
    filament_not_inserted,
    mmu_filament_inserted,
    wrong_filament,
    _last = wrong_filament
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
    ESP_uploaded,
    _last_ESP = ESP_uploaded,

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
    FSensor_ask_have_filament = _first_FSensor,
    FSensor_wait_tool_pick,
    FSensor_ask_unload,
    FSensor_unload,
    FSensor_calibrate,
    FSensor_insertion_check,
    FSensor_insertion_ok,
    Fsensor_enforce_remove,
    FSensor_fail,
    _last_FSensor = FSensor_fail,

    _first_CalibZ,
    CalibZ = _first_CalibZ,
    _last_CalibZ = CalibZ,

    _first_Axis,
    Axis = _first_Axis,
    _last_Axis = Axis,

    _first_Heaters,
    Heaters = _first_Heaters,
    _last_Heaters = Heaters,

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

    _first_Kennel,
    Kennel_needs_calibration = _first_Kennel,
    Kennel_wait_user_park1,
    Kennel_wait_user_park2,
    Kennel_wait_user_park3,
    Kennel_wait_user_remove_pins,
    Kennel_wait_user_loosen_pillar,
    Kennel_wait_user_lock_tool,
    Kennel_wait_user_tighten_top_screw,
    Kennel_measure,
    Kennel_wait_user_install_pins,
    Kennel_wait_user_tighten_bottom_screw,
    Kennel_selftest_park_test,
    _last_Kennel = Kennel_selftest_park_test,

    _first_Tool_Offsets,
    ToolOffsets_wait_user_confirm_start = _first_Tool_Offsets,
    ToolOffsets_wait_user_clean_nozzle_cold,
    ToolOffsets_wait_user_clean_nozzle_hot,
    ToolOffsets_wait_user_install_sheet,
    ToolOffsets_pin_install_prepare,
    ToolOffsets_wait_user_install_pin,
    ToolOffsets_wait_calibrate,
    ToolOffsets_wait_final_park,
    ToolOffsets_wait_user_remove_pin,
    _last_Tool_Offsets = ToolOffsets_wait_user_remove_pin,

    _first_Result,
    Result = _first_Result,
    _last_Result = Result,

    _first_WizardEpilogue,
    WizardEpilogue_ok = _first_WizardEpilogue, // ok is after result
    WizardEpilogue_nok,                        // nok is before result
    _last_WizardEpilogue = WizardEpilogue_nok,

    _last = _last_WizardEpilogue
};

enum class PhasesCrashRecovery : uint16_t {
    _first = static_cast<uint16_t>(PhasesSelftest::_last) + 1,
    check_X = _first, // in this case is safe to have check_X == _first
    check_Y,
    home,
    axis_NOK, //< just for unification of the two below
    axis_short,
    axis_long,
    repeated_crash,
    tool_recovery, //< Toolchanger recovery, tool fell off
    _last = tool_recovery
};

// static class for work with fsm responses (like button click)
// encode responses - get them from marlin client, to marlin server and decode them again
class ClientResponses {
    ClientResponses() = delete;
    ClientResponses(ClientResponses &) = delete;

    // declare 2d arrays of single buttons for radio buttons
    static constexpr PhaseResponses LoadUnloadResponses[] = {
        {},                                                       //_first
        { Response::Stop },                                       // Parking_stoppable
        {},                                                       // Parking_unstoppable,
        { Response::Stop },                                       // WaitingTemp_stoppable,
        {},                                                       // WaitingTemp_unstoppable,
        { Response::Stop },                                       // PreparingToRam_stoppable,
        {},                                                       // PreparingToRam_unstoppable
        { Response::Stop },                                       // Ramming_stoppable,
        {},                                                       // Ramming_unstoppable,
        { Response::Stop },                                       // Unloading_stoppable,
        {},                                                       // Unloading_unstoppable,
        { Response::Filament_removed },                           // RemoveFilament,
        { Response::Yes, Response::No },                          // IsFilamentUnloaded,
        {},                                                       // FilamentNotInFS
        { Response::Continue },                                   // ManualUnload,
        { Response::Continue, Response::Stop },                   // UserPush_stoppable,
        { Response::Continue },                                   // UserPush_unstoppable,
        { Response::Stop },                                       // MakeSureInserted_stoppable,
        {},                                                       // MakeSureInserted_unstoppable,
        { Response::Stop },                                       // Inserting_stoppable,
        {},                                                       // Inserting_unstoppable,
        { Response::Yes, Response::No },                          // IsFilamentInGear,
        { Response::Stop },                                       // Ejecting_stoppable,
        {},                                                       // Ejecting_unstoppable,
        { Response::Stop },                                       // Loading_stoppable,
        {},                                                       // Loading_unstoppable,
        { Response::Stop },                                       // Purging_stoppable,
        {},                                                       // Purging_unstoppable,
        { Response::Yes, Response::Purge_more, Response::Retry }, // IsColor,
        { Response::Yes, Response::Purge_more },                  // IsColorPurge
        {},                                                       // Unparking

#if HAS_MMU2
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
        {},                                                            // MMU_ERRInternal,
        {},                                                            // MMU_ERRHelpingFilament,
        {},                                                            // MMU_ERRTMCFailed,
        {},                                                            // MMU_UnloadingFilament,
        {},                                                            // MMU_LoadingFilament,
        {},                                                            // MMU_SelectingFilamentSlot,
        {},                                                            // MMU_PreparingBlade,
        {},                                                            // MMU_PushingFilament,
        {},                                                            // MMU_PerformingCut,
        {},                                                            // MMU_ReturningSelector,
        {},                                                            // MMU_ParkingSelector,
        {},                                                            // MMU_EjectingFilament,
        {},                                                            // MMU_RetractingFromFinda,
#endif
    };
    static_assert(std::size(ClientResponses::LoadUnloadResponses) == CountPhases<PhasesLoadUnload>());

    static constexpr PhaseResponses PreheatResponses[] = {
        {}, //_first
        { Response::Abort, Response::Cooldown, Response::PLA, Response::PETG,
#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)
            Response::PETG_NH,
#endif
            Response::ASA, Response::ABS, Response::PC, Response::FLEX, Response::HIPS, Response::PP, Response::PVB, Response::PA }, // UserTempSelection
    };
    static_assert(std::size(ClientResponses::PreheatResponses) == CountPhases<PhasesPreheat>());

    static constexpr PhaseResponses PrintPreviewResponses[] = {
        { Response::Print, Response::Back },                   // main_dialog,
        { Response::Abort, Response::Ignore },                 // wrong_printer
        { Response::Abort },                                   // wrong_printer_abort
        { Response::Yes, Response::No, Response::FS_disable }, // filament_not_inserted
        { Response::Yes, Response::No },                       // mmu_filament_inserted
        {
#if PRINTER_TYPE != PRINTER_PRUSA_XL
            Response::Change,
#endif
            Response::Ok, Response::Abort } // wrong_filament
    };
    static_assert(std::size(ClientResponses::PrintPreviewResponses) == CountPhases<PhasesPrintPreview>());

    static constexpr PhaseResponses SelftestResponses[] = {
        {}, // _none == _first

        { Response::Continue, Response::Cancel },                   // WizardPrologue_ask_run
        { Response::Continue, Response::Cancel, Response::Ignore }, // WizardPrologue_ask_run_dev
        { Response::Continue, Response::Cancel },                   // WizardPrologue_info
        { Response::Continue, Response::Cancel },                   // WizardPrologue_info_detailed

        { Response::Continue, Response::Abort }, // ESP_instructions
        { Response::Yes, Response::Skip },       // ESP_USB_not_inserted
        { Response::Yes, Response::Skip },       // ESP_ask_gen
        { Response::Yes, Response::Skip },       // ESP_ask_gen_overwrite
        { Response::Yes, Response::Skip },       // ESP_makefile_failed
        { Response::Continue },                  // ESP_eject_USB
        { Response::Continue, Response::Abort }, // ESP_insert_USB
        { Response::Retry, Response::Abort },    // ESP_invalid
        { Response::Abort },                     // ESP_uploading_config
        { Response::Continue },                  // ESP_enabling_WIFI
        { Response::Continue },                  // ESP_uploaded

        { Response::Continue, Response::Abort }, // ESP_progress_info
        { Response::Abort },                     // ESP_progress_upload
        { Response::Continue },                  // ESP_progress_passed
        { Response::Continue },                  // ESP_progress_failed

        { Response::Continue, Response::NotNow, Response::Never }, // ESP_qr_instructions_flash
        { Response::Continue, Response::Abort },                   // ESP_qr_instructions

        {}, // Fans

        {},                  // Loadcell_prepare
        {},                  // Loadcell_move_away
        {},                  // Loadcell_tool_select
        { Response::Abort }, // Loadcell_cooldown

        { Response::Continue, Response::Abort }, // Loadcell_user_tap_ask_abort
        {},                                      // Loadcell_user_tap_countdown
        {},                                      // Loadcell_user_tap_check
        {},                                      // Loadcell_user_tap_ok
        {},                                      // Loadcell_fail

        { Response::Yes, Response::No },                         // FSensor_ask_have_filament
        {},                                                      // FSensor_wait_tool_pick
        { Response::Unload, Response::Continue },                // FSensor_ask_unload
        { Response::Continue },                                  // FSensor_unload
        {},                                                      // FSensor_calibrate
        { Response::Abort_invalidate_test },                     // FSensor_insertion_check
        { Response::Continue, Response::Abort_invalidate_test }, // FSensor_insertion_ok
        { Response::Abort_invalidate_test },                     // Fsensor_enforce_remove
        {},                                                      // FSensor_fail

        {}, // CalibZ

        {}, // Axis

        {}, // Heaters

        {}, // FirstLayer_mbl
        {}, // FirstLayer_print

        { Response::Next, Response::Unload },                 // FirstLayer_filament_known_and_not_unsensed = _first_FirstLayerQuestions
        { Response::Next, Response::Load, Response::Unload }, // FirstLayer_filament_not_known_or_unsensed
        { Response::Next },                                   // FirstLayer_calib
        { Response::Yes, Response::No },                      // FirstLayer_use_val
        { Response::Next },                                   // FirstLayer_start_print
        { Response::Yes, Response::No },                      // FirstLayer_reprint
        { Response::Next },                                   // FirstLayer_clean_sheet
        { Response::Next },                                   // FirstLayer_failed

        { Response::Continue, Response::Abort }, // Kennel_needs_calibartion
        { Response::Continue, Response::Abort }, // Kennel_wait_user_park1
        { Response::Continue, Response::Abort }, // Kennel_wait_user_park2
        { Response::Continue, Response::Abort }, // Kennel_wait_user_park3
        { Response::Continue, Response::Abort }, // Kennel_wait_user_remove_pins
        { Response::Continue, Response::Abort }, // Kennel_wait_user_loosen_pillar
        { Response::Continue, Response::Abort }, // Kennel_wait_user_lock_tool
        { Response::Continue, Response::Abort }, // Kennel_wait_user_tighten_top_screw
        { Response::Abort },                     // Kennel_measure
        { Response::Continue, Response::Abort }, // Kennel_wait_user_install_pins
        { Response::Continue, Response::Abort }, // Kennel_wait_user_tighten_bottom_screw
        { Response::Abort },                     // Kennel_selftest_park_test

        { Response::Continue, Response::Abort },    // ToolOffsets_wait_user_confirm_start
        { Response::Heatup, Response::Continue },   // ToolOffsets_wait_user_clean_nozzle_cold
        { Response::Cooldown, Response::Continue }, // ToolOffsets_wait_user_clean_nozzle_hot
        { Response::Continue },                     // ToolOffsets_wait_user_install_sheet
        {},                                         // ToolOffsets_pin_install_prepare
        { Response::Continue },                     // ToolOffsets_wait_user_install_pin
        {},                                         // ToolOffsets_wait_calibrate
        {},                                         // ToolOffsets_state_final_park
        { Response::Continue },                     // ToolOffsets_wait_user_remove_pin

        { Response::Next }, // Result

        { Response::Continue }, // WizardEpilogue_ok
        { Response::Continue }, // WizardEpilogue_nok
    };
    static_assert(std::size(ClientResponses::SelftestResponses) == CountPhases<PhasesSelftest>());

    static constexpr PhaseResponses CrashRecoveryResponses[] = {
        {},                                                     // check X == _first
        {},                                                     // check Y
        {},                                                     // home
        { Response::Retry, Response::Pause, Response::Resume }, // axis NOK
        {},                                                     // axis short
        {},                                                     // axis long
        { Response::Resume, Response::Pause },                  // repeated crash
        { Response::Continue },                                 // toolchanger recovery
    };
    static_assert(std::size(ClientResponses::CrashRecoveryResponses) == CountPhases<PhasesCrashRecovery>());

    // methods to "bind" button array with enum type
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesLoadUnload phase) { return LoadUnloadResponses[static_cast<size_t>(phase)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesPreheat phase) { return PreheatResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesPreheat::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesPrintPreview phase) { return PrintPreviewResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesPrintPreview::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesSelftest phase) { return SelftestResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesSelftest::_first)]; }
    static constexpr const PhaseResponses &getResponsesInPhase(const PhasesCrashRecovery phase) { return CrashRecoveryResponses[static_cast<size_t>(phase) - static_cast<size_t>(PhasesCrashRecovery::_first)]; }

public:
    // get index of single response in PhaseResponses
    // return -1 (maxval) if does not exist
    template <class T>
    static uint8_t GetIndex(T phase, Response response) {
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        for (size_t i = 0; i < MAX_RESPONSES; ++i) {
            if (cmds[i] == response)
                return i;
        }
        return -1;
    }

    // get response from PhaseResponses by index
    template <class T>
    static const Response &GetResponse(const T phase, const uint8_t index) {
        if (index >= MAX_RESPONSES)
            return ResponseNone;
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        return cmds[index];
    }

    // get all responses accepted in phase
    template <class T>
    static const PhaseResponses &GetResponses(const T phase) {
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
        if (clicked_index >= MAX_RESPONSES)
            return -1; // this phase does not have response with this index
        return ((static_cast<uint32_t>(phase)) << RESPONSE_BITS) + uint32_t(clicked_index);
    }
};

enum class SelftestParts {
    WizardPrologue,
    ESP,
    ESP_progress,
    ESP_qr,
    Axis,
    Fans,
#if HAS_LOADCELL()
    Loadcell,
#endif
    CalibZ,
    Heaters,
#if FILAMENT_SENSOR_IS_ADC()
    FSensor,
#endif
    FirstLayer,
    FirstLayerQuestions,
    Result,
    WizardEpilogue,
#if BOARD_IS_XLBUDDY
    Kennel,
    ToolOffsets,
#endif
    _none, // cannot be created, must have same index as _count
    _count = _none
};

static constexpr PhasesSelftest SelftestGetFirstPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return PhasesSelftest::_first_WizardPrologue;
    case SelftestParts::ESP:
        return PhasesSelftest::_first_ESP;
    case SelftestParts::ESP_progress:
        return PhasesSelftest::_first_ESP_progress;
    case SelftestParts::ESP_qr:
        return PhasesSelftest::_first_ESP_qr;
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
    case SelftestParts::CalibZ:
        return PhasesSelftest::_first_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_first_Heaters;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_first_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_first_FirstLayerQuestions;
#if BOARD_IS_XLBUDDY
    case SelftestParts::Kennel:
        return PhasesSelftest::_first_Kennel;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_first_Tool_Offsets;
#endif
    case SelftestParts::Result:
        return PhasesSelftest::_first_Result;
    case SelftestParts::WizardEpilogue:
        return PhasesSelftest::_first_WizardEpilogue;
    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

static constexpr PhasesSelftest SelftestGetLastPhaseFromPart(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return PhasesSelftest::_last_WizardPrologue;
    case SelftestParts::ESP:
        return PhasesSelftest::_last_ESP;
    case SelftestParts::ESP_progress:
        return PhasesSelftest::_last_ESP_progress;
    case SelftestParts::ESP_qr:
        return PhasesSelftest::_last_ESP_qr;
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
    case SelftestParts::CalibZ:
        return PhasesSelftest::_last_CalibZ;
    case SelftestParts::Heaters:
        return PhasesSelftest::_last_Heaters;
    case SelftestParts::FirstLayer:
        return PhasesSelftest::_last_FirstLayer;
    case SelftestParts::FirstLayerQuestions:
        return PhasesSelftest::_last_FirstLayerQuestions;
#if BOARD_IS_XLBUDDY
    case SelftestParts::Kennel:
        return PhasesSelftest::_last_Kennel;
    case SelftestParts::ToolOffsets:
        return PhasesSelftest::_last_Tool_Offsets;
#endif
    case SelftestParts::Result:
        return PhasesSelftest::_last_Result;
    case SelftestParts::WizardEpilogue:
        return PhasesSelftest::_last_WizardEpilogue;
    case SelftestParts::_none:
        break;
    }
    return PhasesSelftest::_none;
}

static constexpr bool SelftestPartContainsPhase(SelftestParts part, PhasesSelftest ph) {
    const uint16_t ph_u16 = uint16_t(ph);

    return (ph_u16 >= uint16_t(SelftestGetFirstPhaseFromPart(part))) && (ph_u16 <= uint16_t(SelftestGetLastPhaseFromPart(part)));
}

static constexpr SelftestParts SelftestGetPartFromPhase(PhasesSelftest ph) {
    for (size_t i = 0; i < size_t(SelftestParts::_none); ++i) {
        if (SelftestPartContainsPhase(SelftestParts(i), ph))
            return SelftestParts(i);
    }

    if (SelftestPartContainsPhase(SelftestParts::WizardPrologue, ph))
        return SelftestParts::WizardPrologue;

    if (SelftestPartContainsPhase(SelftestParts::ESP, ph))
        return SelftestParts::ESP;
    if (SelftestPartContainsPhase(SelftestParts::ESP_progress, ph))
        return SelftestParts::ESP_progress;
    if (SelftestPartContainsPhase(SelftestParts::ESP_qr, ph))
        return SelftestParts::ESP_qr;

    if (SelftestPartContainsPhase(SelftestParts::Fans, ph))
        return SelftestParts::Fans;

#if HAS_LOADCELL()
    if (SelftestPartContainsPhase(SelftestParts::Loadcell, ph))
        return SelftestParts::Loadcell;
#endif
#if FILAMENT_SENSOR_IS_ADC()
    if (SelftestPartContainsPhase(SelftestParts::FSensor, ph))
        return SelftestParts::FSensor;
#endif
    if (SelftestPartContainsPhase(SelftestParts::Axis, ph))
        return SelftestParts::Axis;

    if (SelftestPartContainsPhase(SelftestParts::Heaters, ph))
        return SelftestParts::Heaters;

    if (SelftestPartContainsPhase(SelftestParts::CalibZ, ph))
        return SelftestParts::CalibZ;

    if (SelftestPartContainsPhase(SelftestParts::WizardEpilogue, ph))
        return SelftestParts::WizardEpilogue;

    if (SelftestPartContainsPhase(SelftestParts::Result, ph))
        return SelftestParts::Result;

#if BOARD_IS_XLBUDDY
    if (SelftestPartContainsPhase(SelftestParts::Kennel, ph))
        return SelftestParts::Kennel;

#endif
    return SelftestParts::_none;
};

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
    if (!current && !should_be)
        return FSM_action::no_action;

    if (!current && should_be)
        return FSM_action::create;

    if (current && !should_be)
        return FSM_action::destroy;

    // current && should_be
    if (*current == *should_be)
        return FSM_action::no_action;

    return FSM_action::change;
}
