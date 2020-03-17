#include "IDialog.hpp"
#include "display.hpp"

IDialog::IDialog(window_t win)
    : window_t(win) {
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display->w, display->h);
    flg |= WINDOW_FLG_ENABLED; //enabled by default
}
