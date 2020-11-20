//window_event.cpp

#include "window_event.hpp"
#include "dbg.h"

EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {
    bool print = false;

    // clang-format off
        // uncomment debug options
        if (GUI_event_IsKnob(event)) print = true;
        if (GUI_event_IsWindowKnobReaction(event)) print = true;
        //if (GUI_event_IsAnyButLoop(event)) print = true;
    // clang-format on

    if (print) {
        _dbg("%s ptr: %p, event %s\n", event_method_name, sender, GUI_event_prt(event));
    }
} //ctor must be private
