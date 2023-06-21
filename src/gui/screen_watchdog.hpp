// screen_watchdog.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "screen_reset_error.hpp"

class screen_watchdog_data_t : public AddSuperWindow<screen_reset_error_data_t> {
    window_text_t text;
    // window_text_t exit;

public:
    screen_watchdog_data_t();

protected:
    // start after click is disabled because splash screen waits for start event
    // and will freeze, because start event is consumed by this screen

    // TODO uncomment to enable start after click
    // virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
