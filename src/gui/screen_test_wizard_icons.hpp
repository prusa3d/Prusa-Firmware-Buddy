//screen_test_wizard_icons.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_temp_graph.hpp"

struct screen_test_wizard_icons : public window_frame_t {
    window_text_t tst;
    window_text_t back;
    window_text_t txt_na;
    window_text_t txt_ok;
    window_text_t txt_ng;
    window_text_t txt_ip0;
    window_text_t txt_ip1;
    window_text_t txt_hourglass;
    window_text_t txt_autohome;
    window_text_t txt_search;
    window_text_t txt_measure;

    window_icon_t ico_na;
    window_icon_t ico_ok;
    window_icon_t ico_ng;
    window_icon_t ico_ip0;
    window_icon_t ico_ip1;
    window_icon_t ico_hourglass;
    window_icon_t ico_autohome;
    window_icon_t ico_search;
    window_icon_t ico_measure;

public:
    screen_test_wizard_icons();
};
