#pragma once

#include "DialogStateful.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"

class DialogSelftestFans : public IDialogMarlin {
    window_text_t text_fan_test;
    window_wizard_progress_t progress;
    window_text_t text_hotend_fan;
    WindowIcon_OkNg icon_hotend_fan;
    window_text_t text_print_fan;
    WindowIcon_OkNg icon_print_fan;

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;

public:
    DialogSelftestFans();
};
