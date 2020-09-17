#pragma once

#include "DialogStateful.hpp"

class DialogSelftestFansAxis : public IDialogMarlin {
    window_text_t text_fan_test;
    window_numberless_progress_t progress_fan;
    window_text_t text_extruder_fan;
    window_icon_t icon_extruder_fan;
    window_text_t text_print_fan;
    window_icon_t icon_print_fan;
    window_text_t text_checking_axis;
    window_numberless_progress_t progress_axis;
    window_text_t text_x_axis;
    window_icon_t icon_x_axis;
    window_text_t text_y_axis;
    window_icon_t icon_y_axis;
    window_text_t text_z_axis;
    window_icon_t icon_z_axis;

protected:
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override { return true; };

public:
    DialogSelftestFansAxis();
};
