#include "radio_button_fsm.hpp"

RadioButtonFSM::RadioButtonFSM(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase)
    : RadioButton(parent, rect, ClientResponses::get_fsm_responses(fsm_phase.fsm, fsm_phase.phase))
    , fsm_phase_(fsm_phase) {}

void RadioButtonFSM::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        marlin_client::FSM_encoded_response(EncodedFSMResponse {
            .response = FSMResponseVariant::make(Click()),
            .encoded_phase = fsm_phase_.phase,
            .encoded_fsm = std::to_underlying(fsm_phase_.fsm),
        });
        break;

    default:
        IRadioButton::windowEvent(sender, event, param);
        break;
    }
}
