#include "mmu2_fsm.hpp"
#include "pause_stubbed.hpp"
#include "mmu2_error_converter.h"

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

static constexpr uint8_t StepOf(uint32_t step, uint32_t total) {
    return (step * 100U / total);
}

// this requires knowledge of the state machines inside of the MMU and the consequence of steps they run through
static fsm::PhaseData ProgressCodeToPercentage(CommandInProgress cip, uint16_t ec) {
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
            // current sequence reported from the MMU:
            //T1 A*27
            //T1 P3*d1
            //T1 P2*c4
            //T1 P5*af
            //T1 P6*90
            //T1 P1c*45
            //T1 P2*c4
            //T1 F0*31
            switch (ec) {
            case (uint16_t)ProgressCode::UnloadingToFinda:
                p = StepOf(1, 5);
                break;
            case (uint16_t)ProgressCode::FeedingToFinda:
                p = StepOf(2, 5);
                break;
            case (uint16_t)ProgressCode::FeedingToBondtech:
                p = StepOf(3, 5);
                break;
            case (uint16_t)ProgressCode::FeedingToFSensor:
                p = StepOf(4, 5);
                break;
                //            case (uint16_t)ProgressCode::DisengagingIdler: // disengaging idler comes 2x at different spots, not necessary for visualization of progress
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

    fsm::PhaseData ret = { { p } };
    return ret;
}

static constexpr PhasesLoadUnload ProgressCodeToPhasesLoadUnload(uint16_t pc) {
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
    case (uint16_t)ProgressCode::FeedingToNozzle:
        return PhasesLoadUnload::MMU_FeedingToNozzle;
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
    case (uint16_t)ProgressCode::Homing:
        return PhasesLoadUnload::MMU_Homing;
    case (uint16_t)ProgressCode::MovingSelector:
        return PhasesLoadUnload::MMU_MovingSelector;
    case (uint16_t)ProgressCode::FeedingToFSensor:
        return PhasesLoadUnload::MMU_FeedingToFSensor;
    case (uint16_t)ProgressCode::HWTestBegin:
        return PhasesLoadUnload::MMU_HWTestBegin;
    case (uint16_t)ProgressCode::HWTestIdler:
        return PhasesLoadUnload::MMU_HWTestIdler;
    case (uint16_t)ProgressCode::HWTestSelector:
        return PhasesLoadUnload::MMU_HWTestSelector;
    case (uint16_t)ProgressCode::HWTestPulley:
        return PhasesLoadUnload::MMU_HWTestPulley;
    case (uint16_t)ProgressCode::HWTestCleanup:
        return PhasesLoadUnload::MMU_HWTestCleanup;
    case (uint16_t)ProgressCode::HWTestExec:
        return PhasesLoadUnload::MMU_HWTestExec;
    case (uint16_t)ProgressCode::HWTestDisplay:
        return PhasesLoadUnload::MMU_HWTestDisplay;
    case (uint16_t)ProgressCode::ErrHwTestFailed:
        return PhasesLoadUnload::MMU_ErrHwTestFailed;
    default:
        return PhasesLoadUnload::MMU_ERRWaitingForUser; // How to report unknown progress? Should not really happen, but who knows?
    }
}

bool Fsm::IsActive() const {
    return created_this || Pause::IsFsmActive();
};

// wait until conditions are right
// no fsm is open
// idle loop can cause to call MMU and it can cause to call idle
// mmu fsm lock??
void Fsm::Loop() {
    if (!IsActive()) {
        if (Fsm::Instance().reporter.HasReport()) {
            if (reporter.GetErrorSource() != MMU2::ErrorSource::ErrorSourceNone) {
                Activate(); // just open now, next loop will handle error
                // TODO we might need to modify pause.cpp tho handle this state
            } else {
                log_warning(MMU2, "Cannot report due closed FSM");
            }
        }
        return;
    }

    std::optional<Reporter::Report> report = Fsm::Instance().reporter.ConsumeReport();
    if (!report)
        return;

    switch (report->type) {
    case Reporter::Type::error:
        // An error always causes one specific screen to occur
        // Its content is given by the error code translated into Prusa-Error-Codes MMU
        // That needs to be coded into the context data passed to the screen
        // - in this case the raw pointer to error description

        if (report->error.errorCode != ErrorCode::MMU_NOT_RESPONDING) {
            log_info(MMU2, "Report error =% " PRIu16, report->error.errorCode);
            FSM_CHANGE_WITH_DATA__LOGGING(Load_unload,
                PhasesLoadUnload::MMU_ERRWaitingForUser,
                fsm::PointerSerializer<MMUErrDesc>(ConvertMMUErrorCode(uint16_t(report->error.errorCode))).Serialize());
        }
        break;
    case Reporter::Type::progress:
        log_info(MMU2, "Report progress =% " PRIu16, report->progress.progressCode);
        FSM_CHANGE_WITH_DATA__LOGGING(Load_unload, ProgressCodeToPhasesLoadUnload(uint16_t(report->progress.progressCode)), ProgressCodeToPercentage(report->commandInProgress, uint16_t(report->progress.progressCode)));
        break;
    }
}

Response Fsm::GetResponse() const {
    return IsActive() ? marlin_server::ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::MMU_ERRWaitingForUser) : Response::_none;
}

bool Fsm::Activate() {
    if (Pause::IsFsmActive())
        return false; // FSM not ours, avoid setting the created_this flag
    if (created_this)
        return false; // already created by us, skip repeated fsm_create
    created_this = true;
    FSM_CREATE__LOGGING(Load_unload);
    return true;
}

bool Fsm::Deactivate() {
    if (Pause::IsFsmActive())
        return false; // FSM not ours, avoid killing the FSM even if created_this == true
    if (!created_this)
        return false; // not created at all, avoid caling fsm_destroy
    created_this = false;
    FSM_DESTROY__LOGGING(Load_unload);
    return true;
}

} // namespace MMU2
