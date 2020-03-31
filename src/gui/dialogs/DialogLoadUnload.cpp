#include "DialogLoadUnload.hpp"
#include "DialogLoadUnload.h"
#include "gui.h"      //resource_font
#include "resource.h" //IDR_FNT_BIG

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;
#ifdef __cplusplus
}
#endif //__cplusplus

//all buttons share same Window, thus it must be static
static const RadioButton::Window radio_win = { resource_font(IDR_FNT_BIG), gui_defaults.color_back, IDialogStateful::get_radio_button_size() };

//shorter creation of single state
inline RadioButton btn(PhasesLoadUnload phase, const PhaseTexts &texts) {
    return RadioButton(radio_win, ClientResponses::GetResponses(phase), texts);
}

/*****************************************************************************/
// clang-format off
//todo move button texts
static const char *txt_none   = "";
static const char *txt_stop   = "STOP";
static const char *txt_cont   = "CONTINUE";
static const char *txt_disa   = "DISABLE SENSOR";
static const char *txt_yes    = "YES";
static const char *txt_no     = "NO";
static const char *txt_reheat = "REHEAT";

static const PhaseTexts ph_txt_stop   = { txt_stop,   txt_none, txt_none, txt_none };
static const PhaseTexts ph_txt_cont   = { txt_cont,   txt_none, txt_none, txt_none };
static const PhaseTexts ph_txt_reheat = { txt_reheat, txt_none, txt_none, txt_none };
static const PhaseTexts ph_txt_disa   = { txt_disa,   txt_none, txt_none, txt_none };
static const PhaseTexts ph_txt_none   = { txt_none,   txt_none, txt_none, txt_none };
static const PhaseTexts ph_txt_yesno  = { txt_yes,    txt_no,   txt_none, txt_none };

static const char *txt_parking            = "Parking";
static const char *txt_unparking          = "Unparking";
static const char *txt_wait_temp          = "Waiting for temp.";
static const char *txt_prep_ram           = "Preparing to ram";
static const char *txt_ram                = "Ramming";
static const char *txt_unload             = "Unloading";
static const char *txt_push_fil           = "Press CONTINUE and\npush filament into\nthe extruder.     ";
static const char *txt_make_sure_inserted = "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ";
static const char *txt_inserting          = "Inserting";
static const char *txt_loading            = "Loading to nozzle";
static const char *txt_purging            = "Purging";
static const char *txt_is_color           = "Is color correct?";
static const char *txt_nozzle_cold        = "Nozzle is too cold.";

static DialogLoadUnload::States LoadUnloadFactory() {
    DialogLoadUnload::States ret = {
        DialogLoadUnload::State { txt_none,               btn(PhasesLoadUnload::_first,           ph_txt_none) },
        DialogLoadUnload::State { txt_parking,            btn(PhasesLoadUnload::Parking,          ph_txt_stop) },
        DialogLoadUnload::State { txt_wait_temp,          btn(PhasesLoadUnload::WaitingTemp,      ph_txt_stop) },
        DialogLoadUnload::State { txt_prep_ram,           btn(PhasesLoadUnload::PreparingToRam,   ph_txt_stop) },
        DialogLoadUnload::State { txt_ram,                btn(PhasesLoadUnload::Ramming,          ph_txt_stop) },
        DialogLoadUnload::State { txt_unload,             btn(PhasesLoadUnload::Unloading,        ph_txt_stop) },
        DialogLoadUnload::State { txt_unload,             btn(PhasesLoadUnload::Unloading2,       ph_txt_stop) },
        DialogLoadUnload::State { txt_push_fil,           btn(PhasesLoadUnload::UserPush,         ph_txt_cont) },
        DialogLoadUnload::State { txt_nozzle_cold,        btn(PhasesLoadUnload::NozzleTimeout,    ph_txt_reheat) },
        DialogLoadUnload::State { txt_make_sure_inserted, btn(PhasesLoadUnload::MakeSureInserted, ph_txt_cont) },
        DialogLoadUnload::State { txt_inserting,          btn(PhasesLoadUnload::Inserting,        ph_txt_stop) },
        DialogLoadUnload::State { txt_loading,            btn(PhasesLoadUnload::Loading,          ph_txt_stop) },
        DialogLoadUnload::State { txt_purging,            btn(PhasesLoadUnload::Purging,          ph_txt_stop) },
        DialogLoadUnload::State { txt_purging,            btn(PhasesLoadUnload::Purging2,         ph_txt_none) },
        DialogLoadUnload::State { txt_is_color,           btn(PhasesLoadUnload::IsColor,          ph_txt_yesno) },
        DialogLoadUnload::State { txt_unparking,          btn(PhasesLoadUnload::Unparking,        ph_txt_stop) },
    };
    return ret;
}
// clang-format on
/*****************************************************************************/

DialogLoadUnload::DialogLoadUnload(const char *name)
    : DialogStateful<PhasesLoadUnload>(name, WINDOW_CLS_DLG_LOADUNLOAD, LoadUnloadFactory()) {}

void DialogLoadUnload::c_draw(window_t *win) {
    IDialog *ptr = cast(win);
    DialogLoadUnload *ths = dynamic_cast<DialogLoadUnload *>(ptr);
    ths->draw();
}

void DialogLoadUnload::c_event(window_t *win, uint8_t event, void *param) {
    IDialog *ptr = cast(win);
    DialogLoadUnload *ths = dynamic_cast<DialogLoadUnload *>(ptr);
    ths->event(event, param);
}

const window_class_dlg_statemachine_t window_class_dlg_statemachine = {
    {
        WINDOW_CLS_USER,
        sizeof(DialogLoadUnload),
        0,
        0,
        (window_draw_t *)DialogLoadUnload::c_draw,
        (window_event_t *)DialogLoadUnload::c_event,
    },
};
