#pragma once

#include "DialogStateful.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"

class DialogSelftestAxis : public IDialogMarlin {
    window_text_t text_checking_axis;
    window_wizard_progress_t progress;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;
    window_text_t text_z_axis;
    WindowIcon_OkNg icon_z_axis;

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;

public:
    DialogSelftestAxis();
};
