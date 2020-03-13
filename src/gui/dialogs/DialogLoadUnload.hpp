#pragma once

#include "DialogStateful.hpp"
#include "dialog_commands.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<CountPhases<PhasesLoadUnload>()> {
public:
    /* typedef enum {
        Parking,
        WaitingTemp,
        PreparingToRam,
        Ramming,
        Unloading,
        Unloading2,
        UserPush,
        MakeSureInserted,
        Inserting,
        Loading,
        Purging,
        Purging2,
        IsColor,
        Purging3
    } states_t;*/

    DialogLoadUnload(const char *name)
        : DialogStateful<CountPhases<PhasesLoadUnload>()>(name) {}
    //virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {}
};
