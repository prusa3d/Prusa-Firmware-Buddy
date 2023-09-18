// screen_test_gui.hpp
#pragma once
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"
#include "screen.hpp"

struct screen_test_gui_data_t : public AddSuperWindow<screen_t> {
    img::ResourceSingleFile img_printer;
    window_icon_t logo_prusa_printer;
    window_text_t text0;
    window_text_t text1;
    window_text_t text2;
    window_numb_t numb0;
    window_icon_t icon0;
    window_icon_t icon1;
    window_icon_t icon2;
    window_progress_t progress;
    window_text_t text_terminal;

public:
    screen_test_gui_data_t();
};
