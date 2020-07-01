#include "IDialog.hpp"
#include "gui.hpp"
#include <stdint.h>

static window_t winCreate(int16_t WINDOW_CLS_) {
    window_t ret;
    window_create_ptr(WINDOW_CLS_, 0, gui_defaults.scr_body_sz, &ret);
    ret.SetBackColor(gui_defaults.color_back);
    return ret;
}

IDialog::IDialog(int16_t WINDOW_CLS_)
    : window_t(winCreate(WINDOW_CLS_))
    , WINDOW_CLS(WINDOW_CLS_) {
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    flg |= WINDOW_FLG_ENABLED; //enabled by default
}

void IDialog::c_draw(window_t *win) {
    win->Draw();
}

void IDialog::c_event(window_t *win, uint8_t event, void *param) {
    win->Event(win, event, param);
}
