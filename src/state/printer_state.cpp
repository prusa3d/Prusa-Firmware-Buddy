#include "printer_state.hpp"
#include "client_fsm_types.hpp"
#include <option/has_crash_detection.h>
#include <option/has_dwarf.h>
#include <option/has_power_panic.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_serial_print.h>
#include <option/has_tool_offset_pin_calibration.h>
#include "custom_uint31_t.hpp"

#include <fsm_states.hpp>
#include <client_response.hpp>
#include <marlin_vars.hpp>
#include <fsm/safety_timer_phases.hpp>
#include <fsm/filament_change_phases.hpp>
#include <fsm/print_preview_phases.hpp>
#include <option/has_esp.h>
#include <option/has_chamber_vents.h>
#include <option/has_gearbox_alignment.h>
#include <option/has_mmu2.h>
#include <option/has_tool_mapping.h>
#include <option/has_toolchanger.h>
#include <option/has_dwarf.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_phase_stepping_calibration.h>
#include <option/xl_enclosure_support.h>
#include <option/has_uneven_bed_prompt.h>
#include <config_store/store_instance.hpp>
#include <option/has_remote_bed.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_indx.h>
#include <option/has_heaters_selftest_gcode.h>
#include <option/has_door_sensor_calibration.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_side_fsensor.h>
#include <option/has_bed_fan.h>
#include <option/has_psu_fan.h>
#include <option/has_human_interactions.h>
#include <option/has_tool_crash_recovery.h>
#include <option/has_extruder_fsensor.h>
#include <option/has_tool_offset_sensor.h>
#include <printers.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_anfc.h>
#include <option/has_ceiling_clearance.h>
#include <option/has_chamber_api.h>
#include <option/has_coldpull.h>
#include <option/has_emergency_stop.h>
#include <option/has_ht_hotend.h>
#include <option/has_loadcell.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_selftest.h>
#include <fsm/print_preview_mapper.hpp>
#include <bsod/bsod.h>

#if HAS_LOADCELL()
    #include <fsm/nozzle_cleaning_failed_phases.hpp>
    #include <fsm/nozzle_cleaning_failed_mapper.hpp>
#endif
#if HAS_INDX()
    #include <fsm/nozzle_mismatch_phases.hpp>
    #include <fsm/nozzle_mismatch_mapper.hpp>
#endif

using namespace marlin_server;
using namespace printer_state;
using std::make_tuple;
using std::nullopt;
using std::optional;
using std::tuple;

namespace {

#if HAS_CRASH_DETECTION()
// FIXME: these are also caught by the switch statement above, is there any
// harm in having it in both places? Maybe couple more bytes of flash will
// be used, so should we just remove it and let the get_print_state handle
// this one, or is this better, because it's more robust?
optional<ErrCode> crash_recovery_attention(const PhasesCrashRecovery &phase) {
    switch (phase) {
    case PhasesCrashRecovery::axis_long:
        return ErrCode::CONNECT_CRASH_RECOVERY_AXIS_LONG;
    case PhasesCrashRecovery::axis_short:
        return ErrCode::CONNECT_CRASH_RECOVERY_AXIS_SHORT;
    case PhasesCrashRecovery::repeated_crash:
        return ErrCode::CONNECT_CRASH_RECOVERY_REPEATED_CRASH;
    case PhasesCrashRecovery::home_fail:
        return ErrCode::CONNECT_CRASH_RECOVERY_HOME_FAIL;
    #if HAS_TOOL_CRASH_RECOVERY() && HAS_DWARF()
    case PhasesCrashRecovery::tool_recovery:
        return ErrCode::CONNECT_CRASH_RECOVERY_TOOL_PICKUP;
    #endif
    default:
        return nullopt;
    }
}
#endif

optional<ErrCode> attention_while_printpreview(const PhasesPrintPreview preview_phases) {
    return map_print_preview_phase_to_error_code(preview_phases);
}

bool is_warning_attention(const fsm::BaseData &data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    const ErrCode code(warning_type_to_error_code(wtype));
    switch (code) {
    // Note: We don't consider these attention, so just note the dialog code and slap
    // it on whatever state we decide, that the printer is in later.
    case ErrCode::ERR_MECHANICAL_FILAMENT_SENSORS_DISABLED:
    case ErrCode::CONNECT_NOZZLE_TIMEOUT:
    case ErrCode::CONNECT_HEATERS_TIMEOUT:
#if _DEBUG
    case ErrCode::CONNECT_STEPPERS_TIMEOUT:
#endif
#if HAS_SELFTEST()
    case ErrCode::ERR_SYSTEM_ACTION_SELFTEST_REQUIRED:
#endif
#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
        // Local issue, do not report to connect
    case ErrCode::ERR_ELECTRO_DISPLAY_PROBLEM_DETECTED:
#endif
#if PRINTER_IS_PRUSA_XL()
        // Boot-time variant detection messages (incl. the suspected-wiring
        // one — the printer still boots and works); do not raise attention
    case ErrCode::ERR_MECHANICAL_PRINTER_DETECTED_AS_XLS:
    case ErrCode::ERR_MECHANICAL_PRINTER_DETECTED_AS_XL:
    case ErrCode::ERR_MECHANICAL_XL_CAN_WIRING_SUSPECTED:
#endif
        return false;
    default:
        return true;
    }
}

tuple<ErrCode, const Response *> warning_dialog(const fsm::BaseData &data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    auto phase = GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase());
    const Response *buttons = ClientResponses::get_available_responses(phase).data();
    const ErrCode code(warning_type_to_error_code(wtype));
    return make_tuple(code, buttons);
}

// fsm unused on printers, that do not have MMU.
optional<ErrCode> load_unload_attention_while_printing([[maybe_unused]] const fsm::BaseData &data) {
#if HAS_MMU2()
    if (config_store().mmu2_enabled.get()) {
        // distinguish between regular progress of MMU Load/Unload and a real attention/MMU error screen (which is only one particular FSM state)
        switch (GetEnumFromPhaseIndex<PhasesLoadUnload>(data.GetPhase())) {
        case PhasesLoadUnload::MMU_ERRWaitingForUser:
            return ErrCode::CONNECT_MMU_LOAD_UNLOAD_ERROR;
            // Some other questions... they are the same(ish) as with the non-MMU case, so "rounding up" into the same "error".
        case PhasesLoadUnload::LoadFilamentIntoMMU:
        case PhasesLoadUnload::IsColor:
        case PhasesLoadUnload::IsColorPurge:
            return ErrCode::CONNECT_FILAMENT_RUNOUT;
    #if HAS_LOADCELL()
        case PhasesLoadUnload::FilamentStuck:
            return ErrCode::ERR_MECHANICAL_STUCK_FILAMENT_DETECTED;
    #endif
    #if HAS_SIDE_FSENSOR()
        case PhasesLoadUnload::LoadingObstruction_stoppable:
        case PhasesLoadUnload::LoadingObstruction_unstoppable:
            return ErrCode::ERR_MECHANICAL_LOADING_OBSTRUCTION;
    #endif
        default:
            return nullopt;
        }
    }
#endif
    // MMU not supported or not active -> all load/unload during print is really attention.
    switch (GetEnumFromPhaseIndex<PhasesLoadUnload>(data.GetPhase())) {
#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
    case PhasesLoadUnload::FilamentStuck:
        return ErrCode::ERR_MECHANICAL_STUCK_FILAMENT_DETECTED;
#endif
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    case PhasesLoadUnload::LoadingObstruction_stoppable:
    case PhasesLoadUnload::LoadingObstruction_unstoppable:
        return ErrCode::ERR_MECHANICAL_LOADING_OBSTRUCTION;
#endif
    default:
        return ErrCode::CONNECT_FILAMENT_RUNOUT;
    }
}

bool state_is_active(DeviceState state) {
    switch (state) {
    case DeviceState::Busy:
    case DeviceState::Paused:
    case DeviceState::Printing:
        return true;
    default:
        return false;
    }
}
} // namespace

namespace printer_state {

DeviceState get_state(bool ready) {
    std::optional<fsm::States::Top> top;
    bool is_changing_filament, is_printing, is_preheating;
    marlin_vars().peek_fsm_states([&](const auto &states) {
        top = states.get_top();
        is_changing_filament = states.is_active(ClientFSM::Load_unload);
        is_printing = states.is_active(ClientFSM::Printing);
        is_preheating = states.is_active(ClientFSM::Preheat);
    });

    const DeviceState busy_state = is_printing ? DeviceState::Printing : DeviceState::Busy;

    State state = marlin_vars().print_state;
    if (!top) {
        // No FSM present...
        return get_print_state(state, ready);
    }

    const fsm::BaseData &data = top->data;
    switch (top->fsm_type) {
    case ClientFSM::PrintPreview: {
        auto phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(data.GetPhase());
        if (attention_while_printpreview(phase)) {
            return DeviceState::Attention;
        }
        break;
    }
    case ClientFSM::Printing:
        // NOTE: handled in get_print_state, it can be Printing, Paused or Stopped
        break;
    case ClientFSM::Load_unload:
        if (is_printing) {
            if (load_unload_attention_while_printing(top->data)) {
                return DeviceState::Attention;
            } else {
                return DeviceState::Printing;
            }
        } else {
            return DeviceState::Busy;
        }
#if HAS_CRASH_DETECTION()
    case ClientFSM::CrashRecovery:
        if (crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase()))) {
            return DeviceState::Attention;
        }
        break;
#endif
    case ClientFSM::QuickPause:
        return DeviceState::Paused;
#if HAS_LOADCELL()
    case ClientFSM::NozzleCleaningFailed:
        if (nozzle_cleaning_failed_phase_error_code_mapper(GetEnumFromPhaseIndex<PhaseNozzleCleaningFailed>(data.GetPhase()))) {
            return DeviceState::Attention;
        }
        break;
#endif
#if HAS_SELFTEST()
    case ClientFSM::Selftest:
    case ClientFSM::FansSelftest:
    case ClientFSM::SelftestFSensors:
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
    case ClientFSM::HeatersSelftest:
#endif
#if HAS_ESP()
    case ClientFSM::NetworkSetup:
#endif
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
#if HAS_MANUAL_BELT_TUNING()
    case ClientFSM::ManualBeltTuning:
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
    case ClientFSM::PhaseSteppingCalibration:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_GEARBOX_ALIGNMENT()
    case ClientFSM::GearboxAlignment:
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
    case ClientFSM::DoorSensorCalibration:
#endif
#if HAS_INDX()
    case ClientFSM::DockCalibration:
    case ClientFSM::NozzleCleanerCalibration:
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    case ClientFSM::ToolOffsetsCalibration:
#endif
#if HAS_SERIAL_PRINT()
    case ClientFSM::Serial_printing:
#endif
        // FIXME: BFW-3893 Sadly there is no way (without saving state in this function)
        //  to distinguish between preheat from main screen,
        // which would be Idle, and preheat in the middle of filament load/unload,
        // so it is probably better to take it as busy, given we want to decide
        // to allow or not allow remote printing based on this, but this will cause
        // preheat menu to be the only menu screen to not be Idle... :-(
    case ClientFSM::Preheat:
    case ClientFSM::Wait:
        return busy_state;
#if HAS_INDX()
    case ClientFSM::NozzleMismatch:
        return DeviceState::Attention;
#endif

    case ClientFSM::SafetyTimer:
        return safety_timer_is_phase_attention.test(data.GetPhase()) ? DeviceState::Attention : busy_state;

    case ClientFSM::Warning: {
        auto result = get_print_state(state, ready);
        // Some warnings are "soft" (eg. heaters timeouts). They probably
        // require something when printing (paused / busy / ...), so we report
        // them as Attention, but if they happen on Idle (or Finished / Stopped
        // / ...), they can be safely ignored and a new print can be started
        // without dealing with them.
        //
        // We still do send the dialog, we just don't mark the printer as
        // requiring attention.
        //
        // Note that the get_printer_state looks only the data from marlin
        // server, not at full FSM states and can't detect some things - in
        // particular, load / unload from menu is not detectable by this (if
        // "covered" by the warning).
        if (state_is_active(result) || is_warning_attention(data) || is_changing_filament || is_preheating) {
            return DeviceState::Attention;
        } else {
            return result;
        }
    }
    case ClientFSM::_none:
        break;
    }
    return get_print_state(state, ready);
}

DeviceState get_print_state(State state, bool ready) {
    switch (state) {
    case State::PrintPreviewQuestions:
        // Should never happen, we catch this before with FSM states,
        // so that we can distinquish between various questions.
        // Nevertheless it has been seen to happen in connect somehow,
        // so make it Attention, so it in that rate occurrence still
        // kind of make sense.
        return DeviceState::Attention;
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
    case State::CrashRecovery_HOMEFAIL:
        return DeviceState::Attention;
#if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup:
        return DeviceState::Attention;
#endif
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
        return DeviceState::Attention;
#endif
    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintInit:
    case State::Exit:
        if (ready) {
            return DeviceState::Ready;
        } else {
            return DeviceState::Idle;
        }
    case State::Printing:
    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_UnloadFilament:
    case State::Aborting_ParkHead:
    case State::Aborting_Preview:
    case State::Finishing_WaitIdle:
    case State::Finishing_UnloadFilament:
    case State::Finishing_ParkHead:
    case State::PrintPreviewConfirmed:
    case State::SerialPrintInit:
        return DeviceState::Printing;

    case State::PowerPanic_acFault:
    case State::PowerPanic_Resume:
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
#if HAS_INDX()
    case State::PowerPanic_FinishIndxToolchange:
#endif
    case State::CrashRecovery_XY_Measure:
    case State::CrashRecovery_XY_HOME:
        return DeviceState::Busy;

    case State::Pausing_Begin:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
    case State::Paused:
    case State::PowerPanic_AwaitingResume:
    case State::Resuming_BufferData:
    case State::MediaErrorRecovery_BufferData:
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_ExecutingGCodeInterrupt:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
        return DeviceState::Paused;
    case State::Finished:
        if (ready) {
            return DeviceState::Ready;
        } else {
            return DeviceState::Finished;
        }
    case State::Aborted:
        if (ready) {
            return DeviceState::Ready;
        } else {
            return DeviceState::Stopped;
        }
    }
    return DeviceState::Unknown;
}

StateWithDialog get_state_with_dialog(bool ready) {
    // Get the state and slap top FSM dialog on top of it, if any
    DeviceState state = get_state(ready);

    std::optional<fsm::States::Top> top;
    fsm::StateId fsm_gen;
    bool is_printing;
    marlin_vars().peek_fsm_states([&](const fsm::States &states) {
        top = states.get_top();
        fsm_gen = states.get_state_id();
        is_printing = states.is_active(ClientFSM::Printing);
    });

    if (!top) {
        return state;
    }

    const auto &data = top->data;
    switch (top->fsm_type) {
    case ClientFSM::Load_unload:
        if (is_printing) {
            if (auto attention_code = load_unload_attention_while_printing(top->data); attention_code.has_value()) {
                const Response *buttons = ClientResponses::get_available_responses(GetEnumFromPhaseIndex<PhasesLoadUnload>(data.GetPhase())).data();
                return { state, attention_code, fsm_gen, buttons };
            }
        } // TODO: handle normal load unload
        break;
    case ClientFSM::QuickPause: {
        const Response *available_responses = ClientResponses::get_available_responses(GetEnumFromPhaseIndex<PhasesQuickPause>(data.GetPhase())).data();
        return { state, ErrCode::CONNECT_QUICK_PAUSE, fsm_gen, available_responses };
        break;
    }
#if HAS_CRASH_DETECTION()
    case ClientFSM::CrashRecovery:
        if (auto attention_code = crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase())); attention_code.has_value()) {
            const Response *available_responses = ClientResponses::get_available_responses(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase())).data();
            return { state, attention_code, fsm_gen, available_responses };
        }
        break;
#endif
    case ClientFSM::Warning: {
        auto [code, response] = warning_dialog(data);
        return { state, code, fsm_gen, response };
    }
    case ClientFSM::PrintPreview: {
        auto phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(data.GetPhase());
        if (auto attention_code = attention_while_printpreview(phase); attention_code.has_value()) {
            const Response *available_responses = ClientResponses::get_available_responses(phase).data();
            return { state, attention_code, fsm_gen, available_responses };
        }
        break;
    }
#if HAS_LOADCELL()
    case ClientFSM::NozzleCleaningFailed: {
        auto phase = GetEnumFromPhaseIndex<PhaseNozzleCleaningFailed>(data.GetPhase());
        if (auto attention_code = nozzle_cleaning_failed_phase_error_code_mapper(phase); attention_code.has_value()) {
            const Response *available_responses = ClientResponses::get_available_responses(phase).data();
            return { state, attention_code, fsm_gen, available_responses };
        }
        break;
    }
#endif
#if HAS_INDX()
    case ClientFSM::NozzleMismatch: {
        auto phase = GetEnumFromPhaseIndex<PhaseNozzleMismatch>(data.GetPhase());
        if (auto attention_code = nozzle_mismatch_phase_error_code_mapper(phase); attention_code.has_value()) {
            const Response *available_responses = ClientResponses::get_available_responses(phase).data();
            return { state, attention_code, fsm_gen, available_responses };
        }
        break;
    }
#endif

    // These have no buttons or phase
    case ClientFSM::Wait:
    case ClientFSM::Printing:
#if HAS_SERIAL_PRINT()
    case ClientFSM::Serial_printing:
#endif
        break;

#if HAS_SELFTEST()
    case ClientFSM::Selftest:
    case ClientFSM::FansSelftest:
    case ClientFSM::SelftestFSensors:
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
    case ClientFSM::HeatersSelftest:
#endif
#if HAS_ESP()
    case ClientFSM::NetworkSetup:
#endif
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
#if HAS_MANUAL_BELT_TUNING()
    case ClientFSM::ManualBeltTuning:
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
    case ClientFSM::PhaseSteppingCalibration:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_GEARBOX_ALIGNMENT()
    case ClientFSM::GearboxAlignment:
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
    case ClientFSM::DoorSensorCalibration:
#endif
#if HAS_INDX()
    case ClientFSM::DockCalibration:
    case ClientFSM::NozzleCleanerCalibration:
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    case ClientFSM::ToolOffsetsCalibration:
#endif
    case ClientFSM::Preheat:
    case ClientFSM::SafetyTimer:
        // TODO: On some future sunny day, we want to cover all of these
        // and ESP flashing with actual dialogs too; currently we only show a
        // possible warning dialog sitting _on top_ of these.
        //
        // But that's a lot of work, complex, etc, and may need some „special“
        // dialogs too. Leaving it out for now.
        break;

    case ClientFSM::_none:
        // Not possible, we would already return, if no FSM is set, here just to satisfy compiler, that it is handled
        break;
    }

    return state;
}

bool remote_print_ready(bool preview_only) {
    auto &print_state = marlin_vars().print_state;
    if (print_state == State::PrintPreviewInit || print_state == State::PrintPreviewImage) {
        return !preview_only;
    }

    auto state = get_state_with_dialog(false);

    switch (state.device_state) {
    case DeviceState::Idle:
    case DeviceState::Ready:
    case DeviceState::Stopped:
    case DeviceState::Finished:
        return true;
    default:
        return false;
    }
}

bool has_job() {
    switch (get_state(false)) {
    case DeviceState::Printing:
    case DeviceState::Paused:
        return true;
    case DeviceState::Attention: {
        // Attention while printing or one of these questions before print(eg. wrong filament)
        return marlin_vars().peek_fsm_states([](const auto &states) { return states[ClientFSM::Printing] || states[ClientFSM::PrintPreview]; });
    }
    default:
        return false;
    }
}

const char *to_str(DeviceState state) {
    switch (state) {
    case DeviceState::Idle:
        return "IDLE";
    case DeviceState::Printing:
        return "PRINTING";
    case DeviceState::Paused:
        return "PAUSED";
    case DeviceState::Finished:
        return "FINISHED";
    case DeviceState::Stopped:
        return "STOPPED";
    case DeviceState::Ready:
        return "READY";
    case DeviceState::Error:
        return "ERROR";
    case DeviceState::Busy:
        return "BUSY";
    case DeviceState::Attention:
        return "ATTENTION";
    case DeviceState::Unknown:
    default:
        return "UNKNOWN";
    }
}

ErrCode warning_type_to_error_code(WarningType wtype) {
    switch (wtype) {
    case WarningType::HotendFanError:
        return ErrCode::CONNECT_HOTEND_FAN_ERROR;
    case WarningType::PrintFanError:
        return ErrCode::CONNECT_PRINT_FAN_ERROR;
#if HAS_INDX()
    case WarningType::DockFanError:
        return ErrCode::CONNECT_DOCK_FAN_ERROR;
#endif
    case WarningType::HotendTempDiscrepancy:
        return ErrCode::CONNECT_HOTEND_TEMP_DISCREPANCY;
    case WarningType::HeatersTimeout:
        return ErrCode::CONNECT_HEATERS_TIMEOUT;
    case WarningType::NozzleTimeout:
        return ErrCode::CONNECT_NOZZLE_TIMEOUT;
    case WarningType::USBFlashDiskError:
        return ErrCode::CONNECT_USB_FLASH_DISK_ERROR;
    case WarningType::USBDriveUnsupportedFileSystem:
        return ErrCode::WARNING_USB_DRIVE_UNSUPPORTED_FILE_SYSTEM;
    case WarningType::HeatBreakThermistorFail:
        return ErrCode::CONNECT_HEATBREAK_THERMISTOR_FAIL;
#if HAS_SELFTEST()
    case WarningType::ActionSelftestRequired:
        return ErrCode::ERR_SYSTEM_ACTION_SELFTEST_REQUIRED;
#endif
#if HAS_POWER_PANIC()
    case WarningType::HeatbedColdAfterPP:
        return ErrCode::CONNECT_POWER_PANIC_COLD_BED;
#endif
#if HAS_TOOL_OFFSET_PIN_CALIBRATION()
    case WarningType::NozzleDoesNotHaveRoundSection:
        return ErrCode::CONNECT_NOZZLE_DOES_NOT_HAVE_ROUND_SECTION;
#endif
    case WarningType::NotDownloaded:
        return ErrCode::CONNECT_NOT_DOWNLOADED;
    case WarningType::BuddyMCUMaxTemp:
        return ErrCode::CONNECT_BUDDY_MCU_MAX_TEMP;
#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
    case WarningType::DisplayProblemDetected:
        return ErrCode::ERR_ELECTRO_DISPLAY_PROBLEM_DETECTED;
#endif
#if HAS_DWARF()
    case WarningType::DwarfMCUMaxTemp:
        return ErrCode::CONNECT_DWARF_MCU_MAX_TEMP;
#endif
#if HAS_REMOTE_BED()
    case WarningType::BedMCUMaxTemp:
        // TODO Rename this from "modular bed" to just "bed" in Prusa error codes.
        return ErrCode::CONNECT_MOD_BED_MCU_MAX_TEMP;
#endif
    case WarningType::ProbingFailed:
        return ErrCode::CONNECT_PROBING_FAILED;
    case WarningType::FilamentSensorStuckHelp:
        return ErrCode::ERR_MECHANICAL_FILAMENT_SENSOR_STUCK_HELP;
#if HAS_MMU2()
    case WarningType::FilamentSensorStuckHelpMMU:
        return ErrCode::ERR_MECHANICAL_FILAMENT_SENSOR_STUCK_HELP_MMU;
    case WarningType::MaintenanceWarningFails:
        return ErrCode::ERR_MECHANICAL_MAINTENANCE_WARNING_FAILS;
    case WarningType::MaintenanceWarningChanges:
        return ErrCode::ERR_MECHANICAL_MAINTENANCE_WARNING_CHANGES;
#endif
    case WarningType::FilamentSensorsDisabled:
        return ErrCode::ERR_MECHANICAL_FILAMENT_SENSORS_DISABLED;
#if _DEBUG
    case WarningType::SteppersTimeout:
        return ErrCode::CONNECT_STEPPERS_TIMEOUT;
#endif
#if XL_ENCLOSURE_SUPPORT()
    case WarningType::EnclosureFanError:
        return ErrCode::CONNECT_ENCLOSURE_FAN_ERROR;
#endif
#if HAS_CHAMBER_FILTRATION_API()
    case WarningType::EnclosureFilterExpirWarning:
        return ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION_WARNING;
    case WarningType::EnclosureFilterExpiration:
        return ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION;
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
    case WarningType::NozzleCleanerFull:
    case WarningType::NozzleCleanerFullInfo:
        return ErrCode::CONNECT_NOZZLE_CLEANER_FULL;
    case WarningType::NozzleCleanerManualEmpty:
        return ErrCode::CONNECT_NOZZLE_CLEANER_EMPTY;
#endif

#if HAS_PRINT_SHEET_DETECTION()
    case WarningType::SteelSheetNotDetected:
        return ErrCode::ERR_MECHANICAL_STEEL_SHEET_NOT_DETECTED;
#endif

    case WarningType::GcodeCorruption:
        return ErrCode::ERR_SYSTEM_GCODE_CORRUPTION;
    case WarningType::GcodeCropped:
        return ErrCode::ERR_SYSTEM_GCODE_CROPPED;

    case WarningType::MetricsConfigChangePrompt:
        return ErrCode::ERR_CONNECT_GCODE_METRICS_CONFIG_CHANGE;
    case WarningType::AccelerometerCommunicationFailed:
        return ErrCode::ERR_ELECTRO_ACCELEROMETER_COMMUNICATION_FAILED;
    case WarningType::FilamentLoadingTimeout:
        return ErrCode::CONNECT_FILAMENT_LOADING_TIMEOUT;

#if HAS_EMERGENCY_STOP()
    case WarningType::DoorOpen:
        return ErrCode::ERR_MECHANICAL_DOOR_OPEN;
#endif
#if HAS_HT_HOTEND()
    case WarningType::HotendBurnRisk:
        return ErrCode::ERR_MECHANICAL_HOTEND_BURN_RISK;
#endif
#if HAS_CHAMBER_API()
    case WarningType::FailedToReachChamberTemperature:
        return ErrCode::ERR_TEMPERATURE_CHAMBER_FAILED_TO_REACH_TEMP;
#endif

#if HAS_UNEVEN_BED_PROMPT()
    case WarningType::BedUnevenAlignmentPrompt:
        return ErrCode::ERR_MECHANICAL_UNEVEN_BED_ALIGN_PROMPT;
#endif
#if HAS_CHAMBER_API()
    case WarningType::ChamberOverheatingTemperature:
        return ErrCode::ERR_TEMPERATURE_CHAMBER_OVERHEATING_TEMP;
    case WarningType::ChamberCriticalTemperature:
        return ErrCode::ERR_TEMPERATURE_CHAMBER_CRITICAL_TEMP;
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD() || XL_ENCLOSURE_SUPPORT()
    case WarningType::ChamberFiltrationFanError:
        return ErrCode::CONNECT_CHAMBER_FILTRATION_FAN_ERROR;
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    case WarningType::ChamberCoolingFanError:
        return ErrCode::CONNECT_CHAMBER_COOLING_FAN_ERROR;
#endif

#if HAS_CHAMBER_VENTS()
    case WarningType::OpenChamberVents:
        return ErrCode::CONNECT_OPEN_CHAMBER_VENTS;
    case WarningType::CloseChamberVents:
        return ErrCode::CONNECT_CLOSE_CHAMBER_VENTS;
#endif

#if HAS_CEILING_CLEARANCE()
    case WarningType::CeilingClearanceViolation:
        return ErrCode::ERR_MECHANICAL_CEILING_CLEARANCE_VIOLATION;
#endif

#if HAS_PRECISE_HOMING_COREXY()
    case WarningType::HomingCalibrationNeeded:
        return ErrCode::ERR_MECHANICAL_HOMING_CALIBRATION_NEEDED;

    case WarningType::HomingRefinementFailed:
        return ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED;

    case WarningType::HomingCalibrationFromMenuNeeded:
        return ErrCode::ERR_MECHANICAL_HOMING_CALIBRATION_FROM_MENU_NEEDED;
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    case WarningType::ToolOffsetCalibrationFailed:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_CALIBRATION_FAILED;

    case WarningType::HotendOffsetUnsafeZDeviation:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_Z_DEVIATION;

    case WarningType::HotendOffsetUnsafeXyDeviation:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_XY;
    case WarningType::HotendOffsetUnsafeSensorXY:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_SENSOR_XY;
#endif

#if HAS_SELFTEST()
    case WarningType::SelftestNotSuccessfullyCompleted:
        return ErrCode::CONNECT_UNFINISHED_SELFTEST;
#endif

#if HAS_ANFC()
    case WarningType::OpenPrintTagCannotTrack:
        return ErrCode::ERR_CONNECT_OPENPRINTTAG_CANNOT_TRACK;

    case WarningType::OpenPrintTagUsageWriteFailed:
        return ErrCode::ERR_CONNECT_OPENPRINTTAG_USAGE_WRITE_FAILED;
#endif

#if PRINTER_IS_PRUSA_XL()
    case WarningType::PrinterDetectedAsXLS:
        return ErrCode::ERR_MECHANICAL_PRINTER_DETECTED_AS_XLS;

    case WarningType::PrinterDetectedAsXL:
        return ErrCode::ERR_MECHANICAL_PRINTER_DETECTED_AS_XL;

    case WarningType::XlCanWiringSuspected:
        return ErrCode::ERR_MECHANICAL_XL_CAN_WIRING_SUSPECTED;
#endif

    case WarningType::_cnt:
        // Fallthrough to unreachable
        break;
    }

    debug_assert(false);
    return ErrCode::ERR_UNDEF;
}

} // namespace printer_state
