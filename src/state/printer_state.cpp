#include "printer_state.hpp"

#include <fsm_states.hpp>
#include <client_response.hpp>
#include <marlin_vars.hpp>
#include <option/has_mmu2.h>
#include <option/has_dwarf.h>
#include <option/has_input_shaper_calibration.h>
#include <option/xl_enclosure_support.h>
#include <config_store/store_instance.hpp>

using namespace marlin_server;
using namespace printer_state;
using std::make_tuple;
using std::nullopt;
using std::optional;
using std::tuple;

namespace {

#if ENABLED(CRASH_RECOVERY)
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
    #if HAS_TOOLCHANGER()
    case PhasesCrashRecovery::tool_recovery:
        return ErrCode::CONNECT_CRASH_RECOVERY_TOOL_PICKUP;
    #endif
    default:
        return nullopt;
    }
}
#endif

optional<ErrCode> attention_while_printpreview(const PhasesPrintPreview preview_phases) {
    switch (preview_phases) {
    case PhasesPrintPreview::unfinished_selftest:
        return ErrCode::CONNECT_PRINT_PREVIEW_UNFINISHED_SELFTEST;
    case PhasesPrintPreview::new_firmware_available:
        return ErrCode::CONNECT_PRINT_PREVIEW_NEW_FW;
    case PhasesPrintPreview::wrong_printer:
        // This one can mean a lot of things, type of printer, nozzle diameter, wrong number of tools etc.
        // Eventually we want to distinquish between them, to do so we will need to somehow mimic the
        // logic in window_msgbox_wrong_printer.cpp using GCodeInfo::ValidPrinterSettings
        return ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER;
    case PhasesPrintPreview::filament_not_inserted:
        return ErrCode::CONNECT_PRINT_PREVIEW_NO_FILAMENT;
    case PhasesPrintPreview::wrong_filament:
        return ErrCode::CONNECT_PRINT_PREVIEW_WRONG_FILAMENT;
    case PhasesPrintPreview::file_error:
        return ErrCode::CONNECT_PRINT_PREVIEW_FILE_ERROR;
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case PhasesPrintPreview::tools_mapping:
        return ErrCode::CONNECT_PRINT_PREVIEW_TOOLS_MAPPING;
#endif
#if HAS_MMU2()
    case PhasesPrintPreview::mmu_filament_inserted:
        return ErrCode::CONNECT_PRINT_PREVIEW_MMU_FILAMENT_INSERTED;
#endif
    default:
        return nullopt;
    }
}

bool is_warning_attention(const fsm::BaseData &data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    const ErrCode code(warning_type_to_error_code(wtype));
    switch (code) {
    // Note: We don't consider these attention, so just note the dialog code and slap
    // it on whatever state we decide, that the printer is in later.
    case ErrCode::CONNECT_NOZZLE_TIMEOUT:
    case ErrCode::CONNECT_HEATERS_TIMEOUT:
#if _DEBUG
    case ErrCode::CONNECT_STEPPERS_TIMEOUT:
#endif
        return false;
    default:
        return true;
    }
}

tuple<ErrCode, const Response *> warning_dialog(const fsm::BaseData &data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    auto phase = GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase());
    const Response *buttons = ClientResponses::GetResponses(phase).data();
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
        default:
            return nullopt;
        }
    }
#endif
    // MMU not supported or not active -> all load/unload during print is really attention.
    switch (GetEnumFromPhaseIndex<PhasesLoadUnload>(data.GetPhase())) {
#if HAS_LOADCELL()
    case PhasesLoadUnload::FilamentStuck:
        return ErrCode::ERR_MECHANICAL_STUCK_FILAMENT_DETECTED;
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
    const auto &fsm_states = marlin_vars().get_fsm_states();
    const auto &top = fsm_states.get_top();
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
        if (fsm_states.is_active(ClientFSM::Printing)) {
            if (load_unload_attention_while_printing(*fsm_states[ClientFSM::Load_unload])) {
                return DeviceState::Attention;
            } else {
                return DeviceState::Printing;
            }
        } else {
            return DeviceState::Busy;
        }
#if ENABLED(CRASH_RECOVERY)
    case ClientFSM::CrashRecovery:
        if (crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase()))) {
            return DeviceState::Attention;
        }
        break;
#endif
    case ClientFSM::QuickPause:
        return DeviceState::Paused;
    case ClientFSM::Selftest:
    case ClientFSM::FansSelftest:
    case ClientFSM::NetworkSetup:
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_BELT_TUNING()
    case ClientFSM::BeltTuning:
#endif
    case ClientFSM::Serial_printing:
        // FIXME: BFW-3893 Sadly there is no way (without saving state in this function)
        //  to distinguish between preheat from main screen,
        // which would be Idle, and preheat in the middle of filament load/unload,
        // so it is probably better to take it as busy, given we want to decide
        // to allow or not allow remote printing based on this, but this will cause
        // preheat menu to be the only menu screen to not be Idle... :-(
    case ClientFSM::Preheat:
    case ClientFSM::Wait:
        return DeviceState::Busy;

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
        if (state_is_active(result) || is_warning_attention(data) || fsm_states.is_active(ClientFSM::Load_unload) || fsm_states.is_active(ClientFSM::Preheat)) {
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
    case State::PowerPanic_AwaitingResume:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
    case State::CrashRecovery_HOMEFAIL:
        return DeviceState::Attention;
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
        return DeviceState::Attention;
#endif
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
        return DeviceState::Attention;
#endif
    case State::Idle:
    case State::WaitGui:
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
    case State::CrashRecovery_XY_Measure:
    case State::CrashRecovery_XY_HOME:
        return DeviceState::Busy;

    case State::Pausing_Begin:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
    case State::Paused:

    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Pausing_Failed_Code:
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
    const auto &fsm_states = marlin_vars().get_fsm_states();
    const auto &fsm_gen = fsm_states.generation;
    const auto &top = fsm_states.get_top();
    if (!top) {
        return state;
    }

    const auto &data = top->data;
    switch (top->fsm_type) {
    case ClientFSM::Load_unload:
        if (fsm_states.is_active(ClientFSM::Printing)) {
            if (auto attention_code = load_unload_attention_while_printing(*fsm_states[ClientFSM::Load_unload]); attention_code.has_value()) {
                const Response *responses = ClientResponses::GetResponses(GetEnumFromPhaseIndex<PhasesLoadUnload>(data.GetPhase())).data();
                return { state, attention_code, fsm_gen, responses };
            }
        } // TODO: handle normal load unload
        break;
    case ClientFSM::QuickPause: {
        const Response *responses = ClientResponses::GetResponses(GetEnumFromPhaseIndex<PhasesQuickPause>(data.GetPhase())).data();
        return { state, ErrCode::CONNECT_QUICK_PAUSE, fsm_gen, responses };
        break;
    }
#if ENABLED(CRASH_RECOVERY)
    case ClientFSM::CrashRecovery:
        if (auto attention_code = crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase())); attention_code.has_value()) {
            const Response *responses = ClientResponses::GetResponses(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase())).data();
            return { state, attention_code, fsm_gen, responses };
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
            const Response *responses = ClientResponses::GetResponses(phase).data();
            return { state, attention_code, fsm_gen, responses };
        }
        break;
    }

    // These have no buttons or phase
    case ClientFSM::Wait:
    case ClientFSM::Printing:
    case ClientFSM::Serial_printing:
        break;

    case ClientFSM::Selftest:
    case ClientFSM::FansSelftest:
    case ClientFSM::NetworkSetup:
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_BELT_TUNING()
    case ClientFSM::BeltTuning:
#endif
    case ClientFSM::Preheat:
        // TODO: On some future sunny day, we want to cover all the selftests
        // and ESP flashing with actual dialogs too; currently we only show a
        // possible warning dialog sitting _on top_ of these.
        //
        // But that's a lot of work, complex, etc, and may need some „special“
        // dialogs too. Leaving it out for now.
        break;
        // Not possible, we would already return, if no FSM is set, here just to satisfy compiler, that it is handled
    case ClientFSM::_none:
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
        const auto &fsm_states = marlin_vars().get_fsm_states();
        // Attention while printing or one of these questions before print(eg. wrong filament)
        return (fsm_states[ClientFSM::Printing] || fsm_states[ClientFSM::PrintPreview]);
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
    case WarningType::HotendTempDiscrepancy:
        return ErrCode::CONNECT_HOTEND_TEMP_DISCREPANCY;
    case WarningType::HeatersTimeout:
        return ErrCode::CONNECT_HEATERS_TIMEOUT;
    case WarningType::NozzleTimeout:
        return ErrCode::CONNECT_NOZZLE_TIMEOUT;
    case WarningType::USBFlashDiskError:
        return ErrCode::CONNECT_USB_FLASH_DISK_ERROR;
    case WarningType::HeatBreakThermistorFail:
        return ErrCode::CONNECT_HEATBREAK_THERMISTOR_FAIL;
#if ENABLED(POWER_PANIC)
    case WarningType::HeatbedColdAfterPP:
        return ErrCode::CONNECT_POWER_PANIC_COLD_BED;
#endif
#if ENABLED(CALIBRATION_GCODE)
    case WarningType::NozzleDoesNotHaveRoundSection:
        return ErrCode::CONNECT_NOZZLE_DOES_NOT_HAVE_ROUND_SECTION;
#endif
    case WarningType::NotDownloaded:
        return ErrCode::CONNECT_NOT_DOWNLOADED;
    case WarningType::BuddyMCUMaxTemp:
        return ErrCode::CONNECT_BUDDY_MCU_MAX_TEMP;
#if HAS_DWARF()
    case WarningType::DwarfMCUMaxTemp:
        return ErrCode::CONNECT_DWARF_MCU_MAX_TEMP;
#endif
#if HAS_MODULARBED()
    case WarningType::ModBedMCUMaxTemp:
        return ErrCode::CONNECT_MOD_BED_MCU_MAX_TEMP;
#endif
#if HAS_BED_PROBE
    case WarningType::ProbingFailed:
        return ErrCode::CONNECT_PROBING_FAILED;
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    case WarningType::NozzleCleaningFailed:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED;
#endif
#if _DEBUG
    case WarningType::SteppersTimeout:
        return ErrCode::CONNECT_STEPPERS_TIMEOUT;
#endif
#if XL_ENCLOSURE_SUPPORT()
    case WarningType::EnclosureFanError:
        return ErrCode::CONNECT_ENCLOSURE_FAN_ERROR;
    case WarningType::EnclosureFilterExpirWarning:
        return ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION_WARNING;
    case WarningType::EnclosureFilterExpiration:
        return ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION;
#endif // XL_ENCLOSURE_SUPPORT

#if ENABLED(DETECT_PRINT_SHEET)
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
    }

    assert(false);
    return ErrCode::ERR_UNDEF;
}

} // namespace printer_state
