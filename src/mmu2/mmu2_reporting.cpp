/// @file mmu2_reporting.cpp

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include "../common/marlin_server.hpp"
#include "../common/client_response.hpp"
#include "../common/fsm_base_types.hpp"
#include "../../lib/Prusa-Firmware-MMU/src/logic/progress_codes.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "mmu2_error_converter.h"

namespace MMU2 {

void ReportErrorHook(CommandInProgress cip, uint16_t ec) {
    // An error always causes one specific screen to occur
    // Its content is given by the error code translated into Prusa-Error-Codes MMU
    // That needs to be coded into the context data passed to the screen
    // - in this case the raw pointer to error description

    if (ec != (uint16_t)ErrorCode::MMU_NOT_RESPONDING) {
        FSM_CHANGE_WITH_DATA__LOGGING(Load_unload,
            PhasesLoadUnload::MMU_ERRWaitingForUser,
            fsm::PointerSerializer<MMUErrorDesc>(ConvertMMUErrorCode(ec)).Serialize());
    }
}

PhasesLoadUnload ProgressCodeToPhasesLoadUnload(uint16_t pc) {
    switch (pc) {
    case (uint16_t)ProgressCode::EngagingIdler:
        return PhasesLoadUnload::MMU_EngagingIdler;
    case (uint16_t)ProgressCode::DisengagingIdler:
        return PhasesLoadUnload::MMU_DisengagingIdler;
    case (uint16_t)ProgressCode::UnloadingToFinda:
        return PhasesLoadUnload::MMU_UnloadingToFinda;
    case (uint16_t)ProgressCode::UnloadingToPulley:
        return PhasesLoadUnload::MMU_UnloadingToPulley;
    case (uint16_t)ProgressCode::FeedingToFinda:
        return PhasesLoadUnload::MMU_FeedingToFinda;
    case (uint16_t)ProgressCode::FeedingToBondtech:
        return PhasesLoadUnload::MMU_FeedingToBondtech;
    case (uint16_t)ProgressCode::AvoidingGrind:
        return PhasesLoadUnload::MMU_AvoidingGrind;
    case (uint16_t)ProgressCode::OK:
    case (uint16_t)ProgressCode::FinishingMoves:
        return PhasesLoadUnload::MMU_FinishingMoves;
    case (uint16_t)ProgressCode::ERRDisengagingIdler:
        return PhasesLoadUnload::MMU_ERRDisengagingIdler;
    case (uint16_t)ProgressCode::ERREngagingIdler:
        return PhasesLoadUnload::MMU_ERREngagingIdler;
        //    case (uint16_t)ProgressCode::ERRWaitingForUser: return PhasesLoadUnload::MMU_ErrWaitForUser; // this never happens, instead the MMU reports the error
        //    case (uint16_t)ProgressCode::ERRInternal:
    case (uint16_t)ProgressCode::ERRHelpingFilament:
        return PhasesLoadUnload::MMU_ERRHelpingFilament;
        //    case (uint16_t)ProgressCode::ERRTMCFailed:
    case (uint16_t)ProgressCode::UnloadingFilament:
        return PhasesLoadUnload::MMU_UnloadingFilament;
    case (uint16_t)ProgressCode::LoadingFilament:
        return PhasesLoadUnload::MMU_LoadingFilament;
    case (uint16_t)ProgressCode::SelectingFilamentSlot:
        return PhasesLoadUnload::MMU_SelectingFilamentSlot;
    case (uint16_t)ProgressCode::PreparingBlade:
        return PhasesLoadUnload::MMU_PreparingBlade;
    case (uint16_t)ProgressCode::PushingFilament:
        return PhasesLoadUnload::MMU_PushingFilament;
    case (uint16_t)ProgressCode::PerformingCut:
        return PhasesLoadUnload::MMU_PerformingCut;
    case (uint16_t)ProgressCode::ReturningSelector:
        return PhasesLoadUnload::MMU_ReturningSelector;
    case (uint16_t)ProgressCode::ParkingSelector:
        return PhasesLoadUnload::MMU_ParkingSelector;
    case (uint16_t)ProgressCode::EjectingFilament:
        return PhasesLoadUnload::MMU_EjectingFilament;
    case (uint16_t)ProgressCode::RetractingFromFinda:
        return PhasesLoadUnload::MMU_RetractingFromFinda;
    default:
        return PhasesLoadUnload::MMU_ERRWaitingForUser; // How to report unknown progress? Should not really happen, but who knows?
    }
}

constexpr uint8_t StepOf(uint32_t step, uint32_t total) {
    return (step * 100U / total);
}

// this requires knowledge of the state machines inside of the MMU and the consequence of steps they run through
fsm::PhaseData ProgressCodeToPercentage(CommandInProgress cip, uint16_t ec) {
    uint8_t p = 0;

    // error states will report 50% progress as we don't know what will come up next
    switch (ec) {
    case (uint16_t)ProgressCode::ERRDisengagingIdler:
    case (uint16_t)ProgressCode::ERRWaitingForUser:
    case (uint16_t)ProgressCode::ERREngagingIdler:
    case (uint16_t)ProgressCode::ERRHelpingFilament:
        p = 50;
        break;
    }

    if (p == 0) { // no error caught yet
        switch (cip) {
        case CutFilament:
            switch (ec) {
            case (uint16_t)ProgressCode::UnloadingToFinda:
                p = StepOf(1, 11);
                break;
            case (uint16_t)ProgressCode::RetractingFromFinda:
                p = StepOf(2, 11);
                break;
            case (uint16_t)ProgressCode::DisengagingIdler:
                p = StepOf(3, 11);
                break;
            case (uint16_t)ProgressCode::SelectingFilamentSlot:
                p = StepOf(4, 11);
                break;
            case (uint16_t)ProgressCode::FeedingToFinda:
                p = StepOf(5, 11);
                break;
            case (uint16_t)ProgressCode::UnloadingToPulley:
                p = StepOf(6, 11);
                break;
            case (uint16_t)ProgressCode::PreparingBlade:
                p = StepOf(7, 11);
                break;
            case (uint16_t)ProgressCode::PushingFilament:
                p = StepOf(8, 11);
                break;
            case (uint16_t)ProgressCode::PerformingCut:
                p = StepOf(9, 11);
                break;
            case (uint16_t)ProgressCode::ReturningSelector:
                p = StepOf(10, 11);
                break;
            }
            break;
        case EjectFilament:
            switch (ec) {
            case (uint16_t)ProgressCode::UnloadingToFinda:
                p = StepOf(1, 7);
                break;
            case (uint16_t)ProgressCode::RetractingFromFinda:
                p = StepOf(2, 7);
                break;
            case (uint16_t)ProgressCode::DisengagingIdler:
                p = StepOf(3, 7);
                break; // this depends on the sequences - disengaging happens twice
            case (uint16_t)ProgressCode::ParkingSelector:
                p = StepOf(4, 7);
                break;
            case (uint16_t)ProgressCode::EjectingFilament:
                p = StepOf(5, 7);
                break;
            }
            break;
        case Homing:
            p = 50;
            break;
        case LoadFilament:
            switch (ec) {
            case (uint16_t)ProgressCode::FeedingToFinda:
                p = StepOf(1, 4);
                break;
            case (uint16_t)ProgressCode::RetractingFromFinda:
                p = StepOf(2, 4);
                break;
            case (uint16_t)ProgressCode::DisengagingIdler:
                p = StepOf(3, 4);
                break;
            }
            break;
        case Reset:
            p = 50;
            break;
        case ToolChange:
            switch (ec) {
            case (uint16_t)ProgressCode::UnloadingToFinda:
                p = StepOf(1, 6);
                break;
            case (uint16_t)ProgressCode::RetractingFromFinda:
                p = StepOf(2, 6);
                break;
            case (uint16_t)ProgressCode::DisengagingIdler:
                p = StepOf(3, 6);
                break;
            case (uint16_t)ProgressCode::FeedingToFinda:
                p = StepOf(4, 6);
                break;
            case (uint16_t)ProgressCode::FeedingToBondtech:
                p = StepOf(5, 6);
                break;
            }
            break;
        case UnloadFilament:
            switch (ec) {
            case (uint16_t)ProgressCode::UnloadingToFinda:
                p = StepOf(1, 4);
                break;
            case (uint16_t)ProgressCode::RetractingFromFinda:
                p = StepOf(2, 4);
                break;
            case (uint16_t)ProgressCode::DisengagingIdler:
                p = StepOf(3, 4);
                break;
            }
            break;
        default:
            break;
        }
    }
    return ProgressSerializer(p).Serialize();
}

void ReportProgressHook(CommandInProgress cip, uint16_t ec) {
    FSM_CHANGE_WITH_DATA__LOGGING(Load_unload, ProgressCodeToPhasesLoadUnload(ec), ProgressCodeToPercentage(cip, ec));
}

void BeginReport(CommandInProgress /*cip*/, uint16_t /*ec*/) {
    FSM_CREATE__LOGGING(Load_unload);
}

void EndReport(CommandInProgress /*cip*/, uint16_t /*ec*/) {
    FSM_DESTROY__LOGGING(Load_unload);
}

/// @returns true if the MMU is communicating and available
/// can change at runtime
bool MMUAvailable() { return true; }

// k tomu globalni flag ve FW/EEPROM
// meni uzivatel z menu (chce/nechce MMU)
bool UseMMU() { return true; }

Buttons ButtonPressed(uint16_t ec) {
    const Response rsp = ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::MMU_ERRWaitingForUser);

    if (rsp == Response::_none) {
        return NoButton; // no button
    }

    const MMUErrorDesc &ed = ConvertMMUErrorCode(ec);

    // The list of responses which occur in mmu error dialogs
    // Return button index or perform some action on the MK404 by itself (like restart MMU)
    // Based on Prusa-Error-Codes errors_list.h
    // So far hardcoded, but shall be generated in the future
    switch (ed.err_num) {
    case ERR_MECHANICAL_FINDA_DIDNT_TRIGGER:
    case ERR_MECHANICAL_FINDA_DIDNT_SWITCH_OFF:
    case ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER:
    case ERR_MECHANICAL_FSENSOR_DIDNT_SWITCH_OFF:
        switch (rsp) {
        case Response::Slowly: // "Slow load"
            return Left;
        case Response::Retry: // "Repeat action"
            return Middle;
        case Response::Continue: // "Continue"
            return Right;
        default:
            break;
        }
        break;
    case ERR_MECHANICAL_SELECTOR_CANNOT_HOME:
    case ERR_MECHANICAL_IDLER_CANNOT_HOME:
    case ERR_MECHANICAL_PULLEY_STALLED:
        switch (rsp) {
        // may be allow move selector right and left in the future
        case Response::Retry: // "Repeat action"
            return Middle;
        default:
            break;
        }
        break;

    case ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_WARN:
    case ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_WARN:
    case ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_WARN:
        switch (rsp) {
        case Response::Continue: // "Continue"
            return Left;
        case Response::Restart: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;

    case ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_ERROR:
    case ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_ERROR:
    case ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_ERROR:

    case ERR_ELECTRICAL_TMC_PULLEY_DRIVER_ERROR:
    case ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_ERROR:
    case ERR_ELECTRICAL_TMC_IDLER_DRIVER_ERROR:

    case ERR_ELECTRICAL_TMC_PULLEY_DRIVER_RESET:
    case ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_RESET:
    case ERR_ELECTRICAL_TMC_IDLER_DRIVER_RESET:

    case ERR_ELECTRICAL_TMC_PULLEY_UNDERVOLTAGE_ERROR:
    case ERR_ELECTRICAL_TMC_SELECTOR_UNDERVOLTAGE_ERROR:
    case ERR_ELECTRICAL_TMC_IDLER_UNDERVOLTAGE_ERROR:

    case ERR_ELECTRICAL_TMC_PULLEY_DRIVER_SHORTED:
    case ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_SHORTED:
    case ERR_ELECTRICAL_TMC_IDLER_DRIVER_SHORTED:

    case ERR_CONNECT_MMU_NOT_RESPONDING:
    case ERR_CONNECT_COMMUNICATION_ERROR:

    case ERR_SYSTEM_QUEUE_FULL:
    case ERR_SYSTEM_RUNTIME_ERROR:
        switch (rsp) {
        case Response::Restart: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;

    case ERR_SYSTEM_FILAMENT_ALREADY_LOADED:
        switch (rsp) {
        case Response::Unload: // "Unload"
            return Left;
        case Response::Continue: // "Proceed/Continue"
            return Right;
        case Response::Restart: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;

    case ERR_SYSTEM_INVALID_TOOL:
        switch (rsp) {
        case Response::Stop: // "Stop print"
            return StopPrint;
        case Response::Restart: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return NoButton;
}

} // namespace
