// window_event.cpp

#include "window_event.hpp"
#include "log.h"
#include "gui_time.hpp"

LOG_COMPONENT_REF(GUI);

EventLock::EventLock([[maybe_unused]] const char *event_method_name, [[maybe_unused]] window_t *sender, GUI_event_t event) {
    //
    // Log Events
    //
    static uint32_t current_loop_counter = 0;
    static uint64_t logged_events = 0;

    // reset the mask when we enter a new gui loop
    if (current_loop_counter != gui::GetLoopCounter()) {
        logged_events = 0;
        current_loop_counter = gui::GetLoopCounter();
    }

    // log the event (if we didn't already within this loop)
    if (!(logged_events & (1 << static_cast<int>(event)))) {
        logged_events |= 1 << static_cast<int>(event);

        if (event != GUI_event_t::LOOP && event != GUI_event_t::TEXT_ROLL
            && event != GUI_event_t::GUI_STARTUP && event != GUI_event_t::TIMER) {
            log_info(GUI, "Distributing event %s", GUI_event_prt(event));
        }
    }
} // ctor must be private
