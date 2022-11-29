//screen_reset_perror.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "screen.hpp"

class screen_reset_error_data_t : public AddSuperWindow<screen_t> {

public:
    screen_reset_error_data_t();

protected:
    /// starts sound and avoids repetitive starting
    void start_sound();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    bool volume_changed;

private:
    bool sound_started;
    int prev_volume;
};
