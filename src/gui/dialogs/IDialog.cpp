#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc)
    : window_frame_t(Screens::Access()->Get(), rc, is_dialog_t::yes) //use dialog ctor
    , id_capture(GetCapturedWindow()) {
    gui_reset_jogwheel(); //todo do I need this?
    Enable();
    SetCapture();
}

IDialog::~IDialog() {
    if (id_capture)
        id_capture->SetCapture();
}

void create_blocking_dialog_from_normal_window(window_t &dlg) {
    window_t *id_capture = window_t::GetCapturedWindow();

    dlg.SetCapture(); //set capture to dlg, events for list are forwarded in window_dlg_preheat_event

    gui_reset_jogwheel();
    //gui_invalidate();

    while (!Screens::Access()->ConsumeClose()) {
        gui_loop();
    }

    if (id_capture)
        id_capture->SetCapture();
}

void IDialog::MakeBlocking(void (*action)()) const {
    gui_reset_jogwheel();
    //gui_invalidate();

    while (!Screens::Access()->ConsumeClose()) {
        gui_loop();
        action();
    }
}
