#include "DialogLoadUnload.hpp"
#include "window_dlg_statemachine.h"

int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;

static const PhaseTexts txt_stop = { "STOP", "", "", "" };
static const PhaseTexts txt_cont = { "CONTINUE", "", "", "" };
static const PhaseTexts txt_disa = { "DISABLE SENSOR", "", "", "" };
static const PhaseTexts txt_none = { "", "", "", "" };
static const PhaseTexts txt_yesno = { "YES", "NO", "", "" };

static const char *txt_parking = "Parking";
static const char *txt_wait_temp = "Waiting for temp.";
static const char *txt_prep_ram = "Preparing to ram";
static const char *txt_ram = "Ramming";
static const char *txt_unload = "Unloading";
static const char *txt_push_fil = "Press CONTINUE and\npush filament into\nthe extruder.     ";
static const char *txt_make_sure_inserted = "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ";
static const char *txt_inserting = "Inserting";
static const char *txt_loading = "Loading to nozzle";
static const char *txt_purging = "Purging";
static const char *txt_is_color = "Is color correct?";

static const RadioButton::window_t radio_win = { gui_defaults.font_big, gui_defaults.color_back, IDialogStateful::get_radio_button_size() };

static DialogLoadUnload::States LoadUnloadFactory() {
    DialogLoadUnload::States ret = {
        DialogLoadUnload::State { txt_parking, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Parking), txt_stop, true) },
        DialogLoadUnload::State { txt_wait_temp, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::WaitingTemp), txt_stop, true) },
        DialogLoadUnload::State { txt_prep_ram, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::PreparingToRam), txt_stop, false) },
        DialogLoadUnload::State { txt_ram, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Ramming), txt_stop, false) },
        DialogLoadUnload::State { txt_unload, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Unloading), txt_stop, false) },
        DialogLoadUnload::State { txt_unload, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Unloading2), txt_stop, false) },
        DialogLoadUnload::State { txt_push_fil, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::UserPush), txt_cont, true) },
        DialogLoadUnload::State { txt_make_sure_inserted, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::MakeSureInserted), txt_cont, false) },
        DialogLoadUnload::State { txt_inserting, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Inserting), txt_stop, false) },
        DialogLoadUnload::State { txt_loading, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Loading), txt_stop, false) },
        DialogLoadUnload::State { txt_purging, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging), txt_stop, false) },
        DialogLoadUnload::State { txt_purging, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging2), txt_none, false) },
        DialogLoadUnload::State { txt_is_color, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::IsColor), txt_yesno, true) },
        DialogLoadUnload::State { txt_purging, RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging3), txt_yesno, false) },
    };
    return ret;
}

DialogLoadUnload::DialogLoadUnload(const char *name)
    : DialogStateful<CountPhases<PhasesLoadUnload>()>(name, WINDOW_CLS_DLG_LOADUNLOAD, LoadUnloadFactory()) {}

const window_class_dlg_statemachine_t window_class_dlg_statemachine = {
    {
        WINDOW_CLS_USER,
        sizeof(DialogLoadUnload),
#warning is this right?
        0, //(window_init_t *)window_dlg_statemachine_init,
        0,
        (window_draw_t *)DialogLoadUnload::draw,
        (window_event_t *)DialogLoadUnload::event,
    },
};
