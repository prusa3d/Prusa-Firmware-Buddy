#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "screen.hpp"

struct screen_test_dlg_data_t : public AddSuperWindow<screen_t> {
    window_text_t tst;
    window_text_t back;
    window_text_button_t tst_usb_error;
    window_text_button_t tst_fan_error;
    window_text_button_t tst_safety_timer;

public:
    screen_test_dlg_data_t();
};
