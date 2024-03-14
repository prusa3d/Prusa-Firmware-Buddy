#include "mmu2_fsm.hpp"
#include "pause_stubbed.hpp"
#include "mmu2_error_converter.h"
#include "fsm_loadunload_type.hpp"

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

static constexpr uint8_t StepOf(uint32_t step, uint32_t total) {
    return (step * 100U / total);
}

static constexpr bool is_error_code(ProgressCode ec) {
    switch (ec) {
    case ProgressCode::ERRDisengagingIdler:
    case ProgressCode::ERRWaitingForUser:
    case ProgressCode::ERREngagingIdler:
    case ProgressCode::ERRHelpingFilament:
        return true;
    default:
        return false;
    }
}

// this requires knowledge of the state machines inside of the MMU and the consequence of steps they run through
static constexpr uint8_t progress_code_to_percentage(CommandInProgress cip, ProgressCode ec) {
    // error states will report 50% progress as we don't know what will come up next
    if (is_error_code(ec)) {
        return 50;
    }

    switch (cip) {

    case CutFilament: {
        constexpr int step_count = 11;
        switch (ec) {
        case ProgressCode::UnloadingToFinda:
            return StepOf(1, step_count);
        case ProgressCode::RetractingFromFinda:
            return StepOf(2, step_count);
        case ProgressCode::DisengagingIdler:
            return StepOf(3, step_count);
        case ProgressCode::SelectingFilamentSlot:
            return StepOf(4, step_count);
        case ProgressCode::FeedingToFinda:
            return StepOf(5, step_count);
        case ProgressCode::UnloadingToPulley:
            return StepOf(6, step_count);
        case ProgressCode::PreparingBlade:
            return StepOf(7, step_count);
        case ProgressCode::PushingFilament:
            return StepOf(8, step_count);
        case ProgressCode::PerformingCut:
            return StepOf(9, step_count);
        case ProgressCode::ReturningSelector:
            return StepOf(10, step_count);
        default:
            return 0;
        }
        return 0;
    }

    case EjectFilament: {
        constexpr int step_count = 7;
        switch (ec) {
        case ProgressCode::UnloadingToFinda:
            return StepOf(1, step_count);
        case ProgressCode::RetractingFromFinda:
            return StepOf(2, step_count);
        case ProgressCode::DisengagingIdler:
            // this depends on the sequences - disengaging happens twice
            return StepOf(3, step_count);
        case ProgressCode::ParkingSelector:
            return StepOf(4, step_count);
        case ProgressCode::EjectingFilament:
            return StepOf(5, step_count);
        default:
            return 0;
        }
        return 0;
    }

    case Homing:
        return 50;

    case LoadFilament: {
        constexpr int step_count = 4;
        switch (ec) {
        case ProgressCode::FeedingToFinda:
            return StepOf(1, step_count);
        case ProgressCode::RetractingFromFinda:
            return StepOf(2, step_count);
        case ProgressCode::DisengagingIdler:
            return StepOf(3, step_count);
        default:
            return 0;
        }
        return 0;
    }

    case Reset:
        return 50;

    case TestLoad: // test load is almost the same like a toolchange, just different visualization
    case ToolChange: {
        // current sequence reported from the MMU:
        // T1 A*27
        // T1 P3*d1
        // T1 P2*c4
        // T1 P5*af
        // T1 P6*90
        // T1 P1c*45
        // T1 P2*c4
        // T1 F0*31

        constexpr int step_count = 5;
        switch (ec) {
        case ProgressCode::UnloadingToFinda:
            return StepOf(1, step_count);
        case ProgressCode::FeedingToFinda:
            return StepOf(2, step_count);
        case ProgressCode::FeedingToBondtech:
            return StepOf(3, step_count);
        case ProgressCode::FeedingToFSensor:
            return StepOf(4, step_count);
        case ProgressCode::DisengagingIdler:
            // disengaging idler comes 2x at different spots, not necessary for visualization of progress
            return 0;
        default:
            return 0;
        }
        return 0;
    }

    case UnloadFilament: {
        constexpr int step_count = 4;
        switch (ec) {
        case ProgressCode::UnloadingToFinda:
            return StepOf(1, step_count);
        case ProgressCode::RetractingFromFinda:
            return StepOf(2, step_count);
        case ProgressCode::DisengagingIdler:
            return StepOf(3, step_count);
        default:
            return 0;
        }
        return 0;
    }

    default:
        return 0;
    }
    return 0;
}

static constexpr LoadUnloadMode progress_code_to_mode(CommandInProgress cip, ProgressCode ec) {
    if (is_error_code(ec)) {
        return LoadUnloadMode::Load;
    }

    switch (cip) {
    case ToolChange:
        return LoadUnloadMode::Change;
    case UnloadFilament:
        return LoadUnloadMode::Unload;
    case TestLoad:
        return LoadUnloadMode::Test;
    default:
        return LoadUnloadMode::Load;
    }
    return LoadUnloadMode::Load;
}

static constexpr fsm::PhaseData progress_code_to_phase_data(CommandInProgress cip, ProgressCode ec) {
    LoadUnloadMode mode = progress_code_to_mode(cip, ec);
    uint8_t percentage = progress_code_to_percentage(cip, ec);
    return ProgressSerializerLoadUnload(mode, percentage).Serialize();
}

static constexpr PhasesLoadUnload ProgressCodeToPhasesLoadUnload(ProgressCode pc) {
    switch (pc) {
    case ProgressCode::EngagingIdler:
        return PhasesLoadUnload::MMU_EngagingIdler;
    case ProgressCode::DisengagingIdler:
        return PhasesLoadUnload::MMU_DisengagingIdler;
    case ProgressCode::UnloadingToFinda:
        return PhasesLoadUnload::MMU_UnloadingToFinda;
    case ProgressCode::UnloadingToPulley:
        return PhasesLoadUnload::MMU_UnloadingToPulley;
    case ProgressCode::FeedingToFinda:
        return PhasesLoadUnload::MMU_FeedingToFinda;
    case ProgressCode::FeedingToBondtech:
        return PhasesLoadUnload::MMU_FeedingToBondtech;
    case ProgressCode::FeedingToNozzle:
        return PhasesLoadUnload::MMU_FeedingToNozzle;
    case ProgressCode::AvoidingGrind:
        return PhasesLoadUnload::MMU_AvoidingGrind;

    case ProgressCode::OK:
    case ProgressCode::FinishingMoves:
        return PhasesLoadUnload::MMU_FinishingMoves;

    case ProgressCode::ERRDisengagingIdler:
        return PhasesLoadUnload::MMU_ERRDisengagingIdler;
    case ProgressCode::ERREngagingIdler:
        return PhasesLoadUnload::MMU_ERREngagingIdler;
        //    case ProgressCode::ERRWaitingForUser: return PhasesLoadUnload::MMU_ErrWaitForUser; // this never happens, instead the MMU reports the error
        //    case ProgressCode::ERRInternal:
    case ProgressCode::ERRHelpingFilament:
        return PhasesLoadUnload::MMU_ERRHelpingFilament;
        //    case ProgressCode::ERRTMCFailed:
    case ProgressCode::UnloadingFilament:
        return PhasesLoadUnload::MMU_UnloadingFilament;
    case ProgressCode::LoadingFilament:
        return PhasesLoadUnload::MMU_LoadingFilament;
    case ProgressCode::SelectingFilamentSlot:
        return PhasesLoadUnload::MMU_SelectingFilamentSlot;
    case ProgressCode::PreparingBlade:
        return PhasesLoadUnload::MMU_PreparingBlade;
    case ProgressCode::PushingFilament:
        return PhasesLoadUnload::MMU_PushingFilament;
    case ProgressCode::PerformingCut:
        return PhasesLoadUnload::MMU_PerformingCut;
    case ProgressCode::ReturningSelector:
        return PhasesLoadUnload::MMU_ReturningSelector;
    case ProgressCode::ParkingSelector:
        return PhasesLoadUnload::MMU_ParkingSelector;
    case ProgressCode::EjectingFilament:
        return PhasesLoadUnload::MMU_EjectingFilament;
    case ProgressCode::RetractingFromFinda:
        return PhasesLoadUnload::MMU_RetractingFromFinda;
    case ProgressCode::Homing:
        return PhasesLoadUnload::MMU_Homing;
    case ProgressCode::MovingSelector:
        return PhasesLoadUnload::MMU_MovingSelector;
    case ProgressCode::FeedingToFSensor:
        return PhasesLoadUnload::MMU_FeedingToFSensor;
    case ProgressCode::HWTestBegin:
        return PhasesLoadUnload::MMU_HWTestBegin;
    case ProgressCode::HWTestIdler:
        return PhasesLoadUnload::MMU_HWTestIdler;
    case ProgressCode::HWTestSelector:
        return PhasesLoadUnload::MMU_HWTestSelector;
    case ProgressCode::HWTestPulley:
        return PhasesLoadUnload::MMU_HWTestPulley;
    case ProgressCode::HWTestCleanup:
        return PhasesLoadUnload::MMU_HWTestCleanup;
    case ProgressCode::HWTestExec:
        return PhasesLoadUnload::MMU_HWTestExec;
    case ProgressCode::HWTestDisplay:
        return PhasesLoadUnload::MMU_HWTestDisplay;
    case ProgressCode::ErrHwTestFailed:
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
                // We cannot report due to a closed FSM, this can happen
                // (besides other errorneous states) e.g. when the FSM is
                // closed before the last (valid) report is proceccessed by
                // this Loop
                Fsm::Instance().reporter.ConsumeReport();
            }
        }
        return;
    }

    std::optional<Reporter::Report> report = Fsm::Instance().reporter.ConsumeReport();
    if (!report) {
        return;
    }

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
                fsm::PointerSerializer<MMUErrDesc>(ConvertMMUErrorCode(report->error.errorCode)).Serialize());
        }
        break;
    case Reporter::Type::progress:
        log_info(MMU2, "Report progress =% " PRIu16, report->progress.progressCode);
        FSM_CHANGE_WITH_DATA__LOGGING(
            Load_unload,
            ProgressCodeToPhasesLoadUnload(report->progress.progressCode),
            progress_code_to_phase_data(report->commandInProgress, report->progress.progressCode));
        break;
    }
}

Response Fsm::GetResponse() const {
    return IsActive() ? marlin_server::ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::MMU_ERRWaitingForUser) : Response::_none;
}

bool Fsm::Activate() {
    if (Pause::IsFsmActive()) {
        return false; // FSM not ours, avoid setting the created_this flag
    }

    if (created_this) {
        return false; // already created by us, skip repeated fsm_create
    }

    created_this = true;
    FSM_CREATE__LOGGING(Load_unload);
    return true;
}

bool Fsm::Deactivate() {
    if (Pause::IsFsmActive()) {
        return false; // FSM not ours, avoid killing the FSM even if created_this == true
    }

    if (!created_this) {
        return false; // not created at all, avoid caling fsm_destroy
    }

    created_this = false;
    FSM_DESTROY__LOGGING(Load_unload);
    return true;
}

} // namespace MMU2
