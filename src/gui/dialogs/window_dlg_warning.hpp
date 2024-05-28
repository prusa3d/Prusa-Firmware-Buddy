#pragma once

#include "IDialogMarlin.hpp"
#include "client_response.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "radio_button_fsm.hpp"
#include "img_resources.hpp"
#include <find_error.hpp>
#include <enum_array.hpp>

#include <option/has_dwarf.h>
#include <option/has_modularbed.h>

static_assert(sizeof(fsm::PhaseData) == sizeof(WarningType), "If this does not hold, we need to revise how we send the type through teh fsm machinery.");
class DialogWarning : public IDialogMarlin {
    window_icon_t icon;
    window_text_t text;
    RadioButtonFsm<PhasesWarning> button;

public:
    DialogWarning(fsm::BaseData);
};
