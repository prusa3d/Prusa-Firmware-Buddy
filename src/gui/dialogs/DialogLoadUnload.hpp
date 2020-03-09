#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<5> {
public:
    DialogLoadUnload(const char *name);
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress);
};
