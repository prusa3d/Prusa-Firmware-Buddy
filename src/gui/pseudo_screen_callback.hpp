#pragma once

#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"

/// A "pseudo" screen - the only thing it does is calling the callback and then closing itself
class PseudoScreenCallback : public screen_t {
public:
    using Callback = void (*)();

public:
    PseudoScreenCallback(Callback callback);

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    // Header must remain here, screen without a window does not support dialogs
    // and I use internally message box
    // it is known bug, but i am not about to fix it
    // because is a really edge case and it would complicate code a lot
    window_header_t header;

private:
    const Callback callback;

    bool callback_called = false;
};
