//screen_test_graph.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_temp_graph.hpp"

struct screen_test_msgbox_data_t : public window_frame_t {
    window_text_t tst;
    window_text_button_close_screent back;
    window_text_button_close_screent tst_ok;
    window_text_button_close_screent tst_okcancel;
    window_text_button_close_screent tst_abortretryignore;
    window_text_button_close_screent tst_yesnocancel;
    window_text_button_close_screent tst_yesno;
    window_text_button_close_screent tst_retrycancel;
    window_text_button_close_screent tst_ico_custom;
    window_text_button_close_screent tst_ico_error;
    window_text_button_close_screent tst_ico_question;
    window_text_button_close_screent tst_ico_warning;
    window_text_button_close_screent tst_ico_info;

public:
    screen_test_msgbox_data_t();
};
