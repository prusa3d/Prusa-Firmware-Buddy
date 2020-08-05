#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<PhasesLoadUnload> {
public:
    DialogLoadUnload(string_view_utf8 name);
    static void phaseAlertSound();
};
