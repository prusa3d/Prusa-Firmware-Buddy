#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc)
    : AddSuperWindow<window_frame_t>(Screens::Access()->Get(), rc, win_type_t::dialog) {
    Enable();
}

IDialog::IDialog(window_t *parent, Rect16 rc)
    : AddSuperWindow<window_frame_t>(parent, rc, win_type_t::dialog) {
    Enable();
}
