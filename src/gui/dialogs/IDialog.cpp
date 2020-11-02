#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc, IsStrong strong)
    : AddSuperWindow<window_frame_t>(Screens::Access()->Get(), rc, strong == IsStrong::yes ? win_type_t::strong_dialog : win_type_t::dialog) //use dialog ctor
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
    window_frame_t *const ActiveScreen = Screens::Access()->Get();
    //parent pointer must exist and must point to screen
    if (GetParent() && ActiveScreen == GetParent()) {

        WinFilterDialogCapture filter(this);
        window_t *dialog_holding_this = findFirst(reinterpret_cast<window_frame_t *>(GetParent())->GetFirst(), nullptr, filter);

        // can be unregistered normally
        if (dialog_holding_this == nullptr) {
            if (prev_capture) {
                prev_capture->SetCapture();
            }
        }
        // if an dialog has stored capture to this one, locally stored capture must be passed to it
        // it must pass its saved capture to next dialog, even if it is null_ptr
        else {
            IDialog *NextDialog = reinterpret_cast<IDialog *>(dialog_holding_this);
            NextDialog->ModifyStoredCapture(prev_capture);
        }
    }
    clearCapture();
}
void IDialog::clearCapture() {
    prev_capture = nullptr;
}

void IDialog::StoreCapture() {
    prev_capture = GetCapturedWindow();
}

void IDialog::ModifyStoredCapture(window_t *capture) {
    prev_capture = capture;
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
