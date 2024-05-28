#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"

IScreenMenu::IScreenMenu(window_t *parent, string_view_utf8 label, EFooter FOOTER)
    : screen_t(parent, parent != nullptr ? win_type_t::dialog : win_type_t::normal)
    , header(this)
    , menu(this, FOOTER == EFooter::On ? GuiDefaults::RectScreenBody : GuiDefaults::RectScreenNoHeader)
    , footer(this) {

    header.SetText(label);

    FOOTER == EFooter::On ? footer.Show() : footer.Hide();

    CaptureNormalWindow(menu); // set capture to list
}

void IScreenMenu::InitState(screen_init_variant var) {
    if (!var.GetMenuPosition()) {
        return;
    }
    menu.menu.restore_state(*(var.GetMenuPosition()));
}

screen_init_variant IScreenMenu::GetCurrentState() const {
    screen_init_variant ret;
    ret.SetMenuPosition(menu.menu.get_restore_state());
    return ret;
}
