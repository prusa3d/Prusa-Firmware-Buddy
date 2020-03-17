#pragma once

#include "DialogStateful.hpp"
#include "dialog_commands.hpp"

constexpr size_t DialogLoadUnloadPhases = CountPhases<PhasesLoadUnload>();

#pragma pack(push)
#pragma pack(1)
//load unload and change filament dialog
class DialogLoadUnload : public DialogStateful<DialogLoadUnloadPhases> {
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

    DialogLoadUnload(const char *name);
    static void c_draw(window_t *win);
    static void c_event(window_t *win, uint8_t event, void *param);
};
#pragma pack(pop)
