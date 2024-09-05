/**
 * @file radio_button_fsm.hpp
 */

#pragma once
#include "radio_button.hpp"
#include "marlin_client.hpp"

class RadioButtonFSM : public RadioButton {

public:
    RadioButtonFSM(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase);

    inline FSMAndPhase fsm_and_phase() const {
        return fsm_and_phase_;
    }

    void set_fsm_and_phase(FSMAndPhase target);

    // TODO: Removeme
    // Ugly, for backwards compatibility reasons
    inline void Change(FSMAndPhase target) {
        set_fsm_and_phase(target);
    }

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    FSMAndPhase fsm_and_phase_;
};
