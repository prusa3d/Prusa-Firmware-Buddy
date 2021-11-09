#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"
#include "resource.h"

string_view_utf8 IScreenMenu::no_label = string_view_utf8::MakeCPUFLASH((const uint8_t *)no_labelS);

IScreenMenu::IScreenMenu(window_t *parent, string_view_utf8 label, EFooter FOOTER)
    : AddSuperWindow<screen_t>(parent, parent != nullptr ? win_type_t::dialog : win_type_t::normal)
    , header(this)
    , menu(this, FOOTER == EFooter::On ? GuiDefaults::RectScreenBody : GuiDefaults::RectScreenNoHeader, nullptr)
    , footer(this) {

    header.SetText(label);

    FOOTER == EFooter::On ? footer.Show() : footer.Hide();

    CaptureNormalWindow(menu); // set capture to list
}

void IScreenMenu::unconditionalDrawItem(uint8_t index) {
    menu.unconditionalDrawItem(index);
}
