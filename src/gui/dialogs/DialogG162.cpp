#include "DialogG162.hpp"
#include "DialogG162.h"
#include "gui.hpp"    //resource_font
#include "resource.h" //IDR_FNT_BIG
#include "i18n.h"

int16_t WINDOW_CLS_DLG_G162 = 0;

//all buttons share same Window, thus it must be static
static const RadioButton::Window radio_win = { resource_font(IDR_FNT_BIG), gui_defaults.color_back, IDialogStateful::get_radio_button_size() };

//shorter creation of single state
inline RadioButton btn(PhasesG162 phase, const PhaseTexts &texts) {
    return RadioButton(radio_win, ClientResponses::GetResponses(phase), texts);
}

/*****************************************************************************/
// clang-format off
//todo move button texts
static const char *txt_none   = "";
static const char *txt_stop   = N_("STOP");

static const PhaseTexts ph_txt_stop    = { txt_stop,   txt_none, txt_none,  txt_none };
static const PhaseTexts ph_txt_none    = { txt_none,   txt_none, txt_none,  txt_none };

static const char *txt_first              = N_("Finishing         \nbuffered gcodes.  \n");
static const char *txt_parking            = N_("Parking");

static DialogG162::States Factory() {
    DialogG162::States ret = {
        DialogG162::State { txt_first,              btn(PhasesG162::_first,           ph_txt_none) },
        DialogG162::State { txt_parking,            btn(PhasesG162::Parking,          ph_txt_stop) },
    };
    return ret;
}
// clang-format on
/*****************************************************************************/

DialogG162::DialogG162(const char *name)
    : DialogStateful<PhasesG162>(name, WINDOW_CLS_DLG_G162, Factory()) {}

const window_class_dlg_g162_t window_class_dlg_g162 = {
    {
        WINDOW_CLS_USER,
        sizeof(DialogG162),
        nullptr,
        nullptr,
        (window_draw_t *)DialogG162::c_draw,
        (window_event_t *)DialogG162::c_event,
    },
};
