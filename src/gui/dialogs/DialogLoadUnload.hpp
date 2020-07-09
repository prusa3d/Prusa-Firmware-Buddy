#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<PhasesLoadUnload> {
public:
    DialogLoadUnload(const char *name);
    static void phaseAlertSound();
};
