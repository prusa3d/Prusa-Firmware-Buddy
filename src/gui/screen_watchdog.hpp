//screen_watchdog.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"

class screen_watchdog_data_t : public window_frame_t {
    window_text_t text;
    window_text_t exit;

public:
    screen_watchdog_data_t();
};
