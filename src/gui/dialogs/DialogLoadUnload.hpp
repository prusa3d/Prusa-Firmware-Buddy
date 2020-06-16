#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<PhasesLoadUnload> {
public:
    DialogLoadUnload(const char *name);
    static void c_draw(window_t *win);
    static void c_event(window_t *win, uint8_t event, void *param);
    // phase callbacks
    static void phaseAlertSound();
};
