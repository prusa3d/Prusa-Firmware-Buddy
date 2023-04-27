// screen_reset_error.cpp

#include "screen_reset_error.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"

screen_reset_error_data_t::screen_reset_error_data_t()
    : AddSuperWindow<screen_t>()
    , sound_started(false) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    SetBackColor(COLOR_RED);
    start_sound();
}

void screen_reset_error_data_t::start_sound() {
    if (!sound_started) {
        /// avoid collision of sounds
        Sound_Stop();
        Sound_Play(eSOUND_TYPE::CriticalAlert);
        sound_started = true;
    }
}

void screen_reset_error_data_t::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::ENC_UP:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::CLICK:
    case GUI_event_t::HOLD:
        Sound_Stop();
    default:
        break;
    }
}
