#pragma once

#include "DialogStateful.hpp"
#include "window_icon.hpp"

class DialogSelftestFans : public IDialogMarlin {
    window_text_t text_fan_test;
    window_numberless_progress_t progress_fan;
    window_text_t text_extruder_fan;
    WindowIcon_OkNg icon_extruder_fan;
    window_text_t text_print_fan;
    WindowIcon_OkNg icon_print_fan;

protected:
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override;

public:
    DialogSelftestFans();
};
