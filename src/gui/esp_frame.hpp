#pragma once

#include "window_frame.hpp"
#include <common/fsm_base_types.hpp>
#include "radio_button.hpp"
#include "radio_button_fsm.hpp"
#include <guiconfig/wizard_config.hpp>
#include "window_text.hpp"
#include "window_wizard_icon.hpp"

class ESPFrame : public AddSuperWindow<window_frame_t> {
protected:
    PhasesESP phase_current;
    PhasesESP phase_previous;
    fsm::PhaseData data_current;
    fsm::PhaseData data_previous;
    RadioButtonFsm<PhasesESP> radio;

    virtual void change() {};

public:
    ESPFrame(window_t *parent, PhasesESP ph, fsm::PhaseData data);
    void Change(PhasesESP ph, fsm::PhaseData data);
};
