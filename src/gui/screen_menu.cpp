#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"
#include "resource.h"

static const uint32_t HasFooter_FLAG = WINDOW_FLG_USER;
static const uint32_t HasHeaderEvents_FLAG = WINDOW_FLG_USER << 1;

string_view_utf8 IScreenMenu::no_label = string_view_utf8::MakeCPUFLASH((const uint8_t *)no_labelS);

IScreenMenu::IScreenMenu(string_view_utf8 label, EFooter FOOTER, size_t helper_lines, uint32_t font_id)
    : window_menu_t(nullptr) { //pointer to container shall be provided by child

    //todo bind those numeric constants to fonts and guidefaults
    padding = { 0, 6, 2, 6 }; //textrolling cannot handle left padding
    icon_rect = rect_ui16(0, 0, 16 + 20, 30);
    const uint16_t win_h = 320;
    const uint16_t footer_h = win_h - 269; //269 is the smallest number I found in footer implementation, todo it should be in guidefaults
    const uint16_t help_h = helper_lines * (resource_font(font_id)->h + 1);
    //I have no clue why +1, should be + gui_defaults.padding.top + gui_defaults.padding.bottom
    const uint16_t win_x = 10;
    const uint16_t win_w = 240 - 20;

    const uint16_t header_h = gui_defaults.scr_body_sz.y;
    const uint16_t item_h = gui_defaults.font->h + padding.top + padding.bottom;

    const uint16_t menu_rect_h = win_h - help_h - header_h - (FOOTER == EFooter::On ? footer_h : 0);

    const rect_ui16_t menu_rect = rect_ui16(win_x, header_h, win_w, menu_rect_h - menu_rect_h % item_h);

    int16_t root_id = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), this);
    Disable(); //used to have member window_frame_t root, now it is parent

    window_create_ptr(WINDOW_CLS_HEADER, root_id, gui_defaults.header_sz, &(header));
    header.SetText(label);

    window_create_ptr(WINDOW_CLS_MENU, root_id, menu_rect, this);

    flg |= WINDOW_FLG_ENABLED | (FOOTER == EFooter::On ? HasFooter_FLAG : 0) | (helper_lines > 0 ? HasHeaderEvents_FLAG : 0);

    SetCapture(); // set capture to list
    SetFocus();

    if (helper_lines > 0) {
        window_create_ptr(WINDOW_CLS_TEXT, root_id,
            rect_ui16(win_x, win_h - (FOOTER == EFooter::On ? footer_h : 0) - help_h, win_w, help_h),
            &help);
        help.font = resource_font(font_id);
    }

    if (FOOTER == EFooter::On) {
        status_footer_init(&footer, root_id);
    }
}

void IScreenMenu::Done() {
    window_destroy(id);
}

int IScreenMenu::Event(window_t *sender, uint8_t event, void *param) {
    if (flg & HasFooter_FLAG) {
        status_footer_event(&footer, sender, event, param);
    }
    if (flg & HasHeaderEvents_FLAG) {
        header.EventClr();
    }

    //on return 0 screen_dispatch_event will call DispatchEvent
    return 0;
}
