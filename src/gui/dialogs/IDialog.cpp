#include "IDialog.hpp"
#include "gui.h"
#include <stdint.h>

static window_t winCreate(int16_t WINDOW_CLS_) {
    window_t ret;
    window_create_ptr(WINDOW_CLS_, 0, gui_defaults.scr_body_sz, &ret);
    return ret;
}

IDialog::IDialog(int16_t WINDOW_CLS_)
    : window_t(winCreate(WINDOW_CLS_))
    , WINDOW_CLS(WINDOW_CLS_) {
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    flg |= WINDOW_FLG_ENABLED; //enabled by default
}

IDialog *IDialog::cast(window_t *win_addr) {
    //ugly hack to retype window_t* to IDialog*
    //dialog_addr->cls is first member of cstruct window_t
    IDialog *dialog_addr = nullptr;
    window_t *dialg_win_addr = (window_t *)(&(dialog_addr->cls));
    IDialog *ret = reinterpret_cast<IDialog *>(reinterpret_cast<uintptr_t>(win_addr) - reinterpret_cast<uintptr_t>(dialg_win_addr));
    return ret;
}
