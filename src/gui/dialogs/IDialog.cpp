#include "IDialog.hpp"
#include "gui.hpp"
#include <stdint.h>

IDialog::IDialog()
    : window_t(nullptr, gui_defaults.scr_body_sz) {
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    Enable();
}

void IDialog::c_draw(window_t *win) {
    win->Draw();
}

void IDialog::c_event(window_t *win, uint8_t event, void *param) {
    win->Event(win, event, param);
}
