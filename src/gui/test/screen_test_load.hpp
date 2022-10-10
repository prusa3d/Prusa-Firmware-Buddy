/**
 * @file screen_test_load.hpp
 * @brief test of MMU dialog
 */
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_header.hpp"
#include "screen.hpp"

class ScreenTestMMU : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_text_t back;
    window_text_button_t tst_load;

public:
    ScreenTestMMU();
};
