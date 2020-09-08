#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc)
    : window_frame_t(Screens::Access()->Get(), rc, is_dialog_t::yes) //use dialog ctor
    , prev_capture(GetCapturedWindow()) {
    Enable();
    SetCapture();
}

IDialog::~IDialog() {
    releaseCapture();
}

bool IDialog::consumeCloseFlag() const {
    return Screens::Access()->ConsumeClose();
}

void IDialog::guiLoop() const {
    gui_loop();
}

void IDialog::releaseCapture() {
    if (prev_capture)
        prev_capture->SetCapture();
    clearCapture();
}
void IDialog::clearCapture() {
    prev_capture = nullptr;
}

void IDialog::StoreCapture() {
    prev_capture = GetCapturedWindow();
}

void create_blocking_dialog_from_normal_window(window_t &dlg) {
    window_t *prev_capture = window_t::GetCapturedWindow();

    //if dialog or its child window has capture, it must handle its release itsefl
    if (prev_capture && (prev_capture == &dlg || prev_capture->IsChildOf(&dlg))) {
        prev_capture = nullptr;
    } else {
        dlg.SetCapture(); //set capture to dlg, events for list are forwarded in window_dlg_preheat_event
    }
    //gui_invalidate();

    while (!Screens::Access()->ConsumeClose()) {
        gui_loop();
    }

    //if dialog or its child window has capture, it must handle its release itsefl
    if (prev_capture)
        prev_capture->SetCapture();
}
