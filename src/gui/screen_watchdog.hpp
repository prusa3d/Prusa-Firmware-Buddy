//screen_watchdog.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"

class screen_watchdog_data_t : public window_frame_t {
    window_text_t text0;
    window_text_t text1;

public:
    screen_watchdog_data_t();

private:
    virtual int event(window_t *sender, uint8_t event, void *param) override;
};
