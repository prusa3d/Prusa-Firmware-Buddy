#include "printer_state.hpp"

#include <fsm_types.hpp>
#include <client_response.hpp>
#include <marlin_vars.hpp>
#include <option/has_mmu2.h>
#include <config_store/store_instance.hpp>

using namespace marlin_server;

namespace printer_state {
namespace {
    StateWithDialog get_print_state(State state, bool ready) {
        switch (state) {
        case State::PrintPreviewQuestions:
            // Should never happen, we catch this before with FSM states,
            // so that we can distinquish between various questions.
            return DeviceState::Unknown;
        case State::PowerPanic_AwaitingResume:
            return StateWithDialog::attention(ErrCode::CONNECT_POWER_PANIC_COLD_BED);
        case State::CrashRecovery_Axis_NOK:
            return StateWithDialog::attention(ErrCode::CONNECT_CRASH_RECOVERY_AXIS_NOK);
        case State::CrashRecovery_Repeated_Crash:
            return StateWithDialog::attention(ErrCode::CONNECT_CRASH_RECOVERY_REPEATED_CRASH);
        case State::CrashRecovery_HOMEFAIL:
            return StateWithDialog::attention(ErrCode::CONNECT_CRASH_RECOVERY_HOME_FAIL);
        case State::CrashRecovery_Tool_Pickup:
            return StateWithDialog::attention(ErrCode::CONNECT_CRASH_RECOVERY_TOOL_PICKUP);
        case State::PrintPreviewToolsMapping:
            return StateWithDialog::attention(ErrCode::CONNECT_PRINT_PREVIEW_TOOLS_MAPPING);
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

    // FIXME: these are also caught by the switch statement above, is there any
    // harm in having it in both places? Maybe couple more bytes of flash will
    // be used, so should we just remove it and let the get_print_state handle
    // this one, or is this better, because it's more robust?
    std::optional<ErrCode> crash_recovery_attention(const PhasesCrashRecovery &phase) {
        switch (phase) {
        case PhasesCrashRecovery::axis_NOK:
            return ErrCode::CONNECT_CRASH_RECOVERY_AXIS_NOK;
        case PhasesCrashRecovery::repeated_crash:
            return ErrCode::CONNECT_CRASH_RECOVERY_REPEATED_CRASH;
        case PhasesCrashRecovery::home_fail:
            return ErrCode::CONNECT_CRASH_RECOVERY_HOME_FAIL;
        case PhasesCrashRecovery::tool_recovery:
            return ErrCode::CONNECT_CRASH_RECOVERY_TOOL_PICKUP;
        default:
            return std::nullopt;
        }
    }

    std::optional<ErrCode> attention_while_printing(const fsm::Change &q1_change) {
        assert(q1_change.get_queue_index() == fsm::QueueIndex::q1);

        switch (q1_change.get_fsm_type()) {
        case ClientFSM::Load_unload:
#if HAS_MMU2()
            if (config_store().mmu2_enabled.get()) {
                // distinguish between regular progress of MMU Load/Unload and a real attention/MMU error screen (which is only one particular FSM state)
                if (GetEnumFromPhaseIndex<PhasesLoadUnload>(q1_change.get_data().GetPhase()) == PhasesLoadUnload::MMU_ERRWaitingForUser) {
                    return ErrCode::CONNECT_MMU_LOAD_UNLOAD_ERROR;
                } else {
                    return std::nullopt;
                }
            }
#endif
            // MMU not supported or not active -> all load/unload during print is really attention.
            return ErrCode::CONNECT_PRINT_PREVIEW_FILE_ERROR;
        case ClientFSM::CrashRecovery:
            return crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(q1_change.get_data().GetPhase()));
        default:
            return std::nullopt;
        }
    }

    std::optional<ErrCode> attention_while_printpreview(const PhasesPrintPreview preview_phases) {
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
        default:
            return std::nullopt;
        }
    }

    std::optional<ErrCode> warning_attention(marlin_vars_t::FSMChange &fsm_change) {
        fsm::Change *warning_change = nullptr;
        if (fsm_change.q0_change.get_fsm_type() == ClientFSM::Warning) {
            warning_change = &fsm_change.q0_change;
        } else if (fsm_change.q1_change.get_fsm_type() == ClientFSM::Warning) {
            warning_change = &fsm_change.q1_change;
        } else if (fsm_change.q2_change.get_fsm_type() == ClientFSM::Warning) {
            warning_change = &fsm_change.q2_change;
        }

        if (warning_change) {
            WarningType wtype = static_cast<WarningType>(*warning_change->get_data().GetData().data());
            switch (wtype) {
            case WarningType::HotendFanError:
                return ErrCode::CONNECT_WARNING_HOTEND_FAN_ERROR;
            case WarningType::PrintFanError:
                return ErrCode::CONNECT_WARNING_PRINT_FAN_ERROR;
            case WarningType::HotendTempDiscrepancy:
                return ErrCode::CONNECT_WARNING_HOTEND_TEMP_DISCREPANCY;
            case WarningType::HeatersTimeout:
            case WarningType::NozzleTimeout:
                return ErrCode::CONNECT_WARNING_HEATERS_TIMEOUT;
            case WarningType::USBFlashDiskError:
                return ErrCode::CONNECT_WARNING_USB_FLASH_DISK_ERROR;
            case WarningType::HeatBreakThermistorFail:
                return ErrCode::CONNECT_WARNING_HEATBREAK_THERMISTOR_FAIL;
            case WarningType::HeatbedColdAfterPP:
                return ErrCode::CONNECT_WARNING_HEATBED_COLD_AFTER_PP;
            case WarningType::NozzleDoesNotHaveRoundSection:
                return ErrCode::CONNECT_WARNING_NOZZLE_DOES_NOT_HAVE_ROUND_SECTION;
            case WarningType::NotDownloaded:
                return ErrCode::CONNECT_WARNING_NOT_DOWNLOADED;
            case WarningType::BuddyMCUMaxTemp:
                return ErrCode::CONNECT_WARNING_BUDDY_MCU_MAX_TEMP;
            case WarningType::DwarfMCUMaxTemp:
                return ErrCode::CONNECT_WARNING_DWARF_MCU_MAX_TEMP;
            case WarningType::ModBedMCUMaxTemp:
                return ErrCode::CONNECT_WARNING_MOD_BED_MCU_MAX_TEMP;
#if _DEBUG
            case WarningType::SteppersTimeout:
                return ErrCode::CONNECT_WARNING_STEPPERS_TIMEOUT;
#endif
            default:
                assert(false);
            }
        }

        return std::nullopt;
    }
} // namespace

DeviceState get_state(bool ready) {
    return get_state_with_dialog(ready).device_state;
}

StateWithDialog get_state_with_dialog(bool ready) {
    auto fsm_change = marlin_vars()->get_last_fsm_change();
    State state = marlin_vars()->print_state;

    if (auto attention_code = warning_attention(fsm_change); attention_code.has_value()) {
        return { DeviceState::Attention, attention_code.value() };
    }

    switch (fsm_change.q0_change.get_fsm_type()) {
    case ClientFSM::PrintPreview:
        if (auto attention_code = attention_while_printpreview(GetEnumFromPhaseIndex<PhasesPrintPreview>(fsm_change.q0_change.get_data().GetPhase())); attention_code.has_value()) {
            return StateWithDialog::attention(attention_code.value());
        }
        break;
    case ClientFSM::Printing:
        if (auto attention_code = attention_while_printing(fsm_change.q1_change); attention_code.has_value()) {
            return StateWithDialog::attention(attention_code.value());
        }
        break;
    case ClientFSM::Load_unload:
    case ClientFSM::Selftest:
    case ClientFSM::ESP:
    case ClientFSM::Serial_printing:
    // FIXME: BFW-3893 Sadly there is no way (without saving state in this function)
    //  to distinguish between preheat from main screen,
    // which would be Idle, and preheat in the middle of filament load/unload,
    // so it is probably better to take it as busy, given we want to decide
    // to allow or not allow remote printing based on this, but this will cause
    // preheat menu to be the only menu screen to not be Idle... :-(
    case ClientFSM::Preheat:
        return DeviceState::Busy;
    default:
        break;
    }

    return get_print_state(state, ready);
}

bool remote_print_ready(bool preview_only) {
    auto &print_state = marlin_vars()->print_state;
    if (print_state == State::PrintPreviewInit || print_state == State::PrintPreviewImage) {
        return !preview_only;
    }

    switch (get_state(false)) {
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
    case DeviceState::Attention:
        // has job only if attention while printing
        return marlin_vars()->get_last_fsm_change().q0_change.get_fsm_type() == ClientFSM::Printing;
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

} // namespace printer_state
