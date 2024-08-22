#pragma once

#include "IDialogMarlin.hpp"
#include "client_response.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "radio_button_fsm.hpp"
#include <gui/qr.hpp>

static_assert(sizeof(fsm::PhaseData) == sizeof(WarningType), "If this does not hold, we need to revise how we send the type through teh fsm machinery.");
class DialogWarning : public IDialogMarlin {
    window_icon_t icon;
    window_icon_t phone;
    QRErrorUrlWindow qr;
    window_text_t text;
    RadioButtonFsm<PhasesWarning> button;

    DialogWarning(PhasesWarning, WarningType);
    DialogWarning(PhasesWarning, WarningType, const ErrDesc &);

public:
    DialogWarning(fsm::BaseData);
};
