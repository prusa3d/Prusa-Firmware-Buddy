/**
 * @file marlin_print_preview.cpp
 */

#include "marlin_print_preview.hpp"
#include "marlin_server.hpp"
#include "timing.h"
#include "filament_sensor_api.hpp"
#include "filament.hpp"
#include "M70X.hpp"

// would be nice to have option leave phase as it was
// something like std::pair<enum {delete, leave, has_value },PhasesPrintPreview>
// but not currently needed
std::optional<PhasesPrintPreview> IPrintPreview::getCorrespondingPhase(IPrintPreview::State state) {
    switch (state) {
    case State::inactive:
        return std::nullopt;

    case State::preview_wait_user:
        return PhasesPrintPreview::main_dialog;

    case State::wrong_printer_wait_user:
        return PhasesPrintPreview::wrong_printer;

    case State::filament_not_inserted_wait_user:
    case State::filament_not_inserted_load:
        return PhasesPrintPreview::filament_not_inserted;

    case State::mmu_filament_inserted_wait_user:
    case State::mmu_filament_inserted_unload:
        return PhasesPrintPreview::mmu_filament_inserted;

    case State::wrong_filament_wait_user:
    case State::wrong_filament_change:
        return PhasesPrintPreview::wrong_filament;

    case State::done:
        return std::nullopt;
    }
    return std::nullopt;
}

void IPrintPreview::ChangeState(State s) {
    state = s;
    setFsm(getCorrespondingPhase(state));
}

void IPrintPreview::setFsm(std::optional<PhasesPrintPreview> wantedPhase) {
    FSM_action action = IsFSM_action_needed(phase, wantedPhase);
    switch (action) {
    case FSM_action::no_action:
        break;
    case FSM_action::create:
        fsm_create(ClientFSM::PrintPreview);
        if (wantedPhase && *wantedPhase != PhasesPrintPreview::_first) {
            fsm_change(ClientFSM::PrintPreview, *wantedPhase);
        }
        break;
    case FSM_action::destroy:
        fsm_destroy(ClientFSM::PrintPreview);
        break;
    case FSM_action::change:
        fsm_change(ClientFSM::PrintPreview, *wantedPhase); // wantedPhase is not nullopt, FSM_action would not be change otherwise
        break;
    }
    phase = wantedPhase;
}

Response IPrintPreview::GetResponse() {
    return phase ? ClientResponseHandler::GetResponseFromPhase(*phase) : Response::_none;
}

IPrintPreview::State PrintPreview::stateFromFilamentPresence() const {
    return FSensors_instance().HasMMU() ? (FSensors_instance().CanStartPrint() ? State::done : State::mmu_filament_inserted_wait_user) : (FSensors_instance().CanStartPrint() ? stateFromFilamentType() : State::filament_not_inserted_wait_user);
}

static bool is_same(const char *curr_filament, const GCodeInfo::filament_buff &filament_type) {
    return strncmp(curr_filament, filament_type.begin(), filament_type.size()) == 0;
}
static bool filament_known(const char *curr_filament) {
    return strncmp(curr_filament, "---", 3) != 0;
}

IPrintPreview::State PrintPreview::stateFromFilamentType() const {
    const char *curr_filament = Filaments::Current().name;
    return (filament_described && filament_known(curr_filament) && !is_same(curr_filament, filament_type)) ? State::wrong_filament_wait_user : State::done;
}

PrintPreview::Result PrintPreview::Loop() {
    if (GetState() == State::inactive)
        return Result::Inactive;

    uint32_t time = ticks_ms();
    if ((time - last_run) < max_run_period_ms)
        return stateToResult();
    last_run = time;
    const Response response = GetResponse();

    switch (GetState()) {
    case State::inactive: // cannot be, but have it defined to enumerate all states
        return Result::Inactive;
    case State::preview_wait_user:
        switch (response) {
        case Response::Print:
            ChangeState(evaluateStateOnPrintClick());
            break;
        case Response::Back:
            ChangeState(State::inactive);
            return Result::Abort;
        default:
            break;
        }
        break;

    case State::wrong_printer_wait_user:
        switch (response) {
        case Response::Ignore:
            ChangeState(stateFromFilamentPresence());
            break;
        case Response::Abort:
            ChangeState(State::inactive);
            return Result::Abort;
        default:
            break;
        }
        break;

    case State::filament_not_inserted_wait_user:
        switch (response) {
        case Response::FS_disable:
            FSensors_instance().Disable();
            ChangeState(State::done);
            break;
        case Response::No:
            ChangeState(State::inactive);
            return Result::Abort;
        case Response::Yes:
            ChangeState(State::filament_not_inserted_load);
            marlin_server_enqueue_gcode("M701 W2"); // load, return option
            break;
        default:
            break;
        }
        break;

    case State::filament_not_inserted_load:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(State::done);
        }
        break;

    case State::mmu_filament_inserted_wait_user:
        switch (response) {
        case Response::No:
            ChangeState(State::inactive);
            return Result::Inactive;
        case Response::Yes:
            ChangeState(State::mmu_filament_inserted_unload);
            marlin_server_enqueue_gcode("M702 W0"); // load, no return or cooldown
            break;
        default:
            break;
        }
        break;

    case State::mmu_filament_inserted_unload:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(State::done);
        }
        break;

    case State::wrong_filament_wait_user: // change / ignore / abort
        switch (response) {
        case Response::Ignore:
            ChangeState(State::done);
            break;
        case Response::Ok:
            ChangeState(State::inactive);
            return Result::Abort;
        case Response::Change:
            ChangeState(State::wrong_filament_change);
            marlin_server_enqueue_gcode("M1600 R"); // change, return option
            break;
        default:
            break;
        }
        break;

    case State::wrong_filament_change:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(State::done);
        }
        //DialogBlockingChangeLoad is handling return .. might want to do it too
        break;

    case State::done:
        ChangeState(State::inactive);
        return Result::Print;
    }
    return stateToResult();
}

PrintPreview::Result PrintPreview::stateToResult() const {
    switch (GetState()) {
    case State::preview_wait_user:
        return Result::Image;
    case State::wrong_printer_wait_user:
    case State::wrong_filament_change:
    case State::wrong_filament_wait_user:
    case State::filament_not_inserted_load:
    case State::filament_not_inserted_wait_user:
    case State::mmu_filament_inserted_unload:
    case State::mmu_filament_inserted_wait_user:
        return Result::Questions;
    case State::inactive:
    case State::done:
        return Result::Inactive;
    }
    return Result::Inactive;
}

void PrintPreview::Init(const char *path) {
    auto f = fopen(path, "rb");
    if (!f)
        return;
    // I don't need this information
    GCodeInfo::time_buff printing_time;
    unsigned filament_used_g;
    unsigned filament_used_mm;

    GCodeInfo::PreviewInit(*f, printing_time, filament_type,
        filament_used_g, filament_used_mm, filament_described, valid_printer_settings);

    fclose(f);
    ChangeState(skip_if_able ? evaluateStateOnPrintClick() : State::preview_wait_user);
}

IPrintPreview::State PrintPreview::evaluateStateOnPrintClick() {
    return valid_printer_settings ? stateFromFilamentPresence() : State::wrong_printer_wait_user;
}
