/**
 * @file screen_touch_error.hpp
 * @brief error screen when touch failed during
 */

#pragma once

#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"

class ScreenTouchError : public AddSuperWindow<screen_t> {
    window_header_t header; // Header must remain here, screen without a window does not support dialogs
                            // and I use internally message box
                            // it is known bug, but i am not about to fix it
                            // because is a really edge case and it would complicate code a lot
    bool event_in_progress = false;

public:
    ScreenTouchError();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
