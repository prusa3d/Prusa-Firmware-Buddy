#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(rect_ui16_t rc)
    : IDialog(nullptr, rc) {
}

IDialog::IDialog(window_t *child, rect_ui16_t rc)
    : window_frame_t(rc, child) //use dialog ctor
    , id_capture(GetCapturedWindow()) {
    gui_reset_jogwheel(); //todo do I need this?
    Enable();
    SetCapture();
}

IDialog::~IDialog() {
    if (id_capture)
        id_capture->SetCapture();
}
