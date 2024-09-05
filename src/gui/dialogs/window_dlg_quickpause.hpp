#pragma once

#include "IDialogMarlin.hpp"
#include "window_text.hpp"
#include "window_roll_text.hpp"
#include "window_icon.hpp"
#include "client_fsm_types.h"
#include "radio_button_fsm.hpp"

class DialogQuickPause : public IDialogMarlin {
    window_icon_t icon;
    window_text_t text;
    window_roll_text_t gcode_name;
    RadioButtonFSM radio;

public:
    DialogQuickPause(fsm::BaseData data);
};
