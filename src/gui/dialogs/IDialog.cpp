#include "IDialog.hpp"
#include "gui.hpp"
#include <stdint.h>

IDialog::IDialog()
    : window_t(gui_defaults.scr_body_sz) //use dialog ctor
{
    if (rect_empty_ui16(rect)) //use display rect if current rect is empty
        rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    //Enable();
}
