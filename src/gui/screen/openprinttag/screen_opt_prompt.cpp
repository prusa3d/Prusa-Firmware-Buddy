/// @file
#include "screen_opt_prompt.hpp"

#include <gui/ScreenHandler.hpp>
#include <common/marlin_vars.hpp>

namespace buddy::openprinttag {

[[nodiscard]] bool ScreenOPTPrompt::can_popup() {

    return
        // Don't bother the user while printing
        marlin_client::is_idle()
        // Or when processing any gcode (especially during loads/unloads!)
        && !marlin_vars().is_processing;
}

ScreenOPTPrompt::ScreenOPTPrompt(const char *title)
    : header_ { this, _(title) } {

    close_timer_.restart(ticks_ms());
}

void ScreenOPTPrompt::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::LOOP:
        if (close_timer_.check(ticks_ms()) || !can_popup()) {
            Screens::Access()->close_screen_and_dialog();
            return;
        }
        break;

    default:
        break;
    }

    screen_t::screenEvent(sender, event, param);
}

} // namespace buddy::openprinttag
