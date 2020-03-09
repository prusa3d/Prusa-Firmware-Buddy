#pragma once

#include "IDialog.hpp"
#include "window_dlg_statemachine.h"
//data structure defining 1 stateful state

extern window_t *window_1; //current popup window

struct StatefulState {
};

//parent for stateful dialogs dialog
template <size_t SZ>
class DialogStateful : public IDialog {
protected:
    int16_t id_capture;
    int16_t id;
    const char *_name;
    StatefulState states[SZ];
    window_dlg_statemachine_t dlg; //todo c remains
public:
    DialogStateful(const char *name);
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress);
    virtual ~DialogStateful();
};

//*****************************************************************************
//template definitions
template <size_t SZ>
DialogStateful<SZ>::DialogStateful(const char *name)
    : _name(name)
    , id_capture(window_capture())
    , id(window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg)) {

    window_1 = (window_t *)&dlg; //todo
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
}

template <size_t SZ>
DialogStateful<SZ>::~DialogStateful() {
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
}
