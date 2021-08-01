#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc, IsStrong strong)
    : AddSuperWindow<window_frame_t>(strong == IsStrong::yes ? nullptr : Screens::Access()->Get(), rc, strong == IsStrong::yes ? win_type_t::strong_dialog : win_type_t::dialog) { //use dialog ctor
    Enable();
}

IDialog::IDialog(window_t *parent, Rect16 rc)
    : AddSuperWindow<window_frame_t>(parent, rc, win_type_t::dialog) {
    Enable();
}

bool IDialog::consumeCloseFlag() const {
    return Screens::Access()->ConsumeClose();
}

void IDialog::guiLoop() const {
    gui::TickLoop();
    gui_loop();
}

void create_blocking_dialog_from_normal_window(window_t &dlg) {
    while (!Screens::Access()->ConsumeClose()) {
        gui::TickLoop();
        gui_loop();
    }
}
