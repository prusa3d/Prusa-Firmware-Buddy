/**
 * @file screen_test_msgbox.hpp
 * @brief test of message boxes
 */
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_header.hpp"
#include "screen.hpp"

struct ScreenTestMSGBox : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_text_t back;
    window_text_button_t tst_ok;
    window_text_button_t tst_ico_error;
    window_text_button_t tst_ico_question;
    window_text_button_t tst_ico_warning;
    window_text_button_t tst_ico_info;
    window_text_button_t tst_icon;
    window_text_button_t tst_strong_hotend_fan;
    window_text_button_t tst_strong_print_fan;
    window_text_button_t tst_strong_heater;
    window_text_button_t tst_strong_usb_error;

public:
    ScreenTestMSGBox();
};
