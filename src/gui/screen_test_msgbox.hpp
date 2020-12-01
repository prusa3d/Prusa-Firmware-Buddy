//screen_test_graph.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_temp_graph.hpp"
#include "screen.hpp"

struct screen_test_msgbox_data_t : public AddSuperWindow<screen_t> {
    window_text_t tst;
    window_text_t back;
    window_text_button_t tst_ok;
    window_text_button_t tst_okcancel;
    window_text_button_t tst_ico_error;
    window_text_button_t tst_ico_question;
    window_text_button_t tst_ico_warning;
    window_text_button_t tst_ico_info;
    window_text_button_t tst_icon;

public:
    screen_test_msgbox_data_t();
};
