#include "DialogStateful.hpp"
#include "window_dlg_statemachine.h"
#include "gui.h"
//data structure defining 1 stateful state

extern window_t *window_1; //current popup window, C-code remain

extern const _dlg_state test_states[14];

const char *const test_title = "TEST";
static _cl_dlg cl_dlg = { test_title, test_states, 14, NULL, NULL, NULL, NULL }; //todo c remains

constexpr window_dlg_statemachine_t dlg_init() {
    window_dlg_statemachine_t ret = {};
    ret._ths = &cl_dlg;
    return ret;
}

static window_dlg_statemachine_t dlg = dlg_init(); //todo c remains

//*****************************************************************************

IDialogStateful::IDialogStateful(const char *name)
    : id_capture(window_capture())
    , _name(name)
    //  err dlg nezna stavy a count
    , id(window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg)) {

    window_1 = (window_t *)&dlg; //todo
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
}
#define DLG_BTN_CH 0x1000                                 // button changed
#define DLG_TXT_CH 0x2000                                 // text changed
#define DLG_PRO_CH 0x4000                                 // progress changed
#define DLG_PPR_CH 0x8000                                 // part progress changed
#define DLG_PRX_CH (DLG_PRO_CH | DLG_PPR_CH)              // some progress changed
#define DLG_PHA_CH (DLG_PRX_CH | DLG_BTN_CH | DLG_TXT_CH) // phase changed

void IDialogStateful::Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    dlg.vars.phase = phase;
    dlg.flags |= DLG_PHA_CH;
    gui_invalidate();
}

IDialogStateful::~IDialogStateful() {
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
}
