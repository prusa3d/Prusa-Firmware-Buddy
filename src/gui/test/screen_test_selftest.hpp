/**
 * @file screen_test_selftest.hpp
 * @brief test of selftest dialogs
 */
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_header.hpp"
#include "screen.hpp"

class ScreenTestSelftest : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_text_t back;
    window_text_button_t btn_run;

public:
    ScreenTestSelftest();
};
