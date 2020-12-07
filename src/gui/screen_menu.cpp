#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"
#include "resource.h"

string_view_utf8 IScreenMenu::no_label = string_view_utf8::MakeCPUFLASH((const uint8_t *)no_labelS);

static uint16_t get_help_h(size_t helper_lines, uint32_t font_id) {
    //I have no clue why +1, should be + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom
    return helper_lines * (resource_font(font_id)->h + 1);
}

IScreenMenu::IScreenMenu(window_t *parent, string_view_utf8 label, Rect16 menu_item_rect, EFooter FOOTER, size_t helper_lines, uint32_t font_id)
    : AddSuperWindow<screen_t>(parent, GuiDefaults::RectScreen, parent != nullptr ? win_type_t::dialog : win_type_t::normal)
    , header(this)
    , menu(this, Rect16(0, 0, 0, 0), nullptr)
    , help(this, Rect16(0, 0, 0, 0), is_multiline::yes)
    , footer(this) {

    /// Split window to menu and helper
    const int help_h = get_help_h(helper_lines, font_id);
    help.rect = Rect16(menu_item_rect.Left(), menu_item_rect.Top() + menu_item_rect.Height() - help_h, menu_item_rect.Width(), help_h);
    menu.rect = menu_item_rect - Rect16::Height_t(help_h);

    //pointer to container shall be provided by child

    //todo bind those numeric constants to fonts and guidefaults
    //padding = { 0, 6, 2, 6 }; //textrolling cannot handle left padding
    //icon_rect = Rect16(0, 0, 16 + 20, 30);
    //const uint16_t help_h = get_help_h(helper_lines, font_id);
    //const uint16_t header_h = GuiDefaults::RectScreenBody.y;
    //const uint16_t item_h = GuiDefaults::Font->h + padding.top + padding.bottom;
    //const uint16_t menu_rect_h = win_h - help_h - header_h - (FOOTER == EFooter::On ? footer_h : 0);
    //const Rect16 menu_rect = Rect16(win_x, header_h, win_w, menu_rect_h - menu_rect_h % item_h);

    header.SetText(label);

    FOOTER == EFooter::On ? footer.Show() : footer.Hide();

    CaptureNormalWindow(menu); // set capture to list

    if (helper_lines > 0) {
        help.font = resource_font(font_id);
    }
}

void IScreenMenu::unconditionalDrawItem(uint8_t index) {
    menu.unconditionalDrawItem(index);
}
