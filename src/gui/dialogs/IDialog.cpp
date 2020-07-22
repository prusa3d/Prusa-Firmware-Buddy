#include "IDialog.hpp"
#include "gui.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog()
    : window_t(gui_defaults.scr_body_sz) //use dialog ctor
    , id_capture(GetCapturedWindow()) {
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    gui_reset_jogwheel(); //todo do I need this?
    Enable();
    SetCapture();
}

IDialog::~IDialog() {
    if (id_capture)
        id_capture->SetCapture();

    //todo this should be automatic
    // window_t *pWin = GetParent();//Screens::Access()->Get();
    // if (pWin)
    //     pWin->Invalidate();
}
