#pragma once

#include "DialogStateful.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<5> {
public:
    typedef enum {
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
    } states_t;

    DialogLoadUnload(const char *name)
        : DialogStateful<5>(name) {}
    //virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {}
};
