#pragma once

#include "DialogStateful.hpp"
#include "dialog_commands.hpp"
extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;

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
    //virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {}
};
#pragma pack(pop)
