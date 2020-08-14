#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"
#include "resource.h"

string_view_utf8 IScreenMenu::no_label = string_view_utf8::MakeCPUFLASH((const uint8_t *)no_labelS);

static constexpr uint16_t win_x = 10;
static constexpr uint16_t win_w = 240 - 20;
static constexpr uint16_t win_h = 320;
static constexpr uint16_t footer_h = win_h - 269; //269 is the smallest number I found in footer implementation, todo it should be in guidefaults

static uint16_t get_help_h(size_t helper_lines, uint32_t font_id) {
    //I have no clue why +1, should be + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom
    return helper_lines * (resource_font(font_id)->h + 1);
}

IScreenMenu::IScreenMenu(window_t *parent, string_view_utf8 label, Rect16 rect, EFooter FOOTER, size_t helper_lines, uint32_t font_id)
    : window_menu_t(parent, rect, nullptr)
    , header(this)
    , help(this, helper_lines > 0 ? Rect16(win_x, win_h - (FOOTER == EFooter::On ? footer_h : 0) - get_help_h(helper_lines, font_id), win_w, get_help_h(helper_lines, font_id)) : Rect16(0, 0, 0, 0))
    , footer(this) {
    //pointer to container shall be provided by child

    //todo bind those numeric constants to fonts and guidefaults
    //padding = { 0, 6, 2, 6 }; //textrolling cannot handle left padding
    //icon_rect = Rect16(0, 0, 16 + 20, 30);
    //const uint16_t help_h = get_help_h(helper_lines, font_id);
    //const uint16_t header_h = GuiDefaults::RectScreenBody.y;
    //const uint16_t item_h = GuiDefaults::Font->h + padding.top + padding.bottom;
    //const uint16_t menu_rect_h = win_h - help_h - header_h - (FOOTER == EFooter::On ? footer_h : 0);
    //const Rect16 menu_rect = Rect16(win_x, header_h, win_w, menu_rect_h - menu_rect_h % item_h);

    Disable(); //used to have member window_frame_t root, now it is parent

    header.SetText(label);

    FOOTER == EFooter::On ? footer.Show() : footer.Hide();

    Enable();
    if (!IsDialog())  // dialog needs to save actual value of caption first
        SetCapture(); // set capture to list
    SetFocus();

    if (helper_lines > 0) {
        help.font = resource_font(font_id);
    }
}

void IScreenMenu::windowEvent(window_t *sender, uint8_t event, void *param) {
    header.EventClr();
    window_menu_t::windowEvent(sender, event, param);
    if ((event == WINDOW_EVENT_ENC_DN) || (event == WINDOW_EVENT_ENC_UP)) { // hack because we want prevent redrawing header/footer to prevent blinking
        header.Validate();
        footer.Validate();
    }
}
