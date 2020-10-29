#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<PhasesLoadUnload> {
public:
    DialogLoadUnload(string_view_utf8 name);

    static bool is_M600_phase;

    static void phaseAlertSound();
    static void phaseWaitSound();
    static void phaseStopSound();
};
