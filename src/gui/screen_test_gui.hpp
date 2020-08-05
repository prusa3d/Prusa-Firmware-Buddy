//screen_test_gui.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_spin.hpp"
#include "window_list.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"

struct screen_test_gui_data_t : public window_frame_t {
    window_icon_t logo_prusa_mini;
    window_text_t text0;
    window_text_t text1;
    window_text_t text2;
    window_numb_t numb0;
    window_spin_t spin0;
    window_spin_t spin1;
    window_list_t list;
    window_icon_t icon0;
    window_icon_t icon1;
    window_icon_t icon2;
    window_progress_t progress;
    window_text_t text_terminal;

public:
    screen_test_gui_data_t();
};
