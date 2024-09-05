#include "radio_button_fsm.hpp"

RadioButtonFSM::RadioButtonFSM(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase)
    : RadioButton(parent, rect, ClientResponses::get_fsm_responses(fsm_phase.fsm, fsm_phase.phase))
    , fsm_and_phase_(fsm_phase) {}

void RadioButtonFSM::set_fsm_and_phase(FSMAndPhase target) {
    if (fsm_and_phase_ == target) {
        return;
    }

    RadioButton::Change(ClientResponses::get_fsm_responses(target.fsm, target.phase));
    fsm_and_phase_ = target;
}

void RadioButtonFSM::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        marlin_client::FSM_response(fsm_and_phase(), Click());
        break;

    default:
        IRadioButton::windowEvent(sender, event, param);
        break;
    }
}
