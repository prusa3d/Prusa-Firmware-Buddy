#include "screen_menu.hpp"
#include "config.h"
#include "stdlib.h"
#include "resource.h"

static const uint32_t HasFooter_FLAG = WINDOW_FLG_USER;
static const uint32_t HasHeaderEvents_FLAG = WINDOW_FLG_USER << 1;

IScreenMenu::IScreenMenu(const char *label, EFooter FOOTER, size_t helper_lines, uint32_t font_id)
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

    int16_t root_id = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(root));
    window_disable(root_id);

    int16_t id = window_create_ptr(WINDOW_CLS_HEADER, root_id, gui_defaults.header_sz, &(header));
    // p_window_header_set_icon(&(header), IDR_PNG_status_icon_menu);
    p_window_header_set_text(&(header), label);

    id = window_create_ptr(WINDOW_CLS_MENU, root_id, menu_rect, this);

    win.flg |= WINDOW_FLG_ENABLED | (FOOTER == EFooter::On ? HasFooter_FLAG : 0) | (helper_lines > 0 ? HasHeaderEvents_FLAG : 0);

    window_set_capture(id); // set capture to list
    window_set_focus(id);

    if (helper_lines > 0) {
        id = window_create_ptr(WINDOW_CLS_TEXT, root_id,
            rect_ui16(win_x, win_h - (FOOTER == EFooter::On ? footer_h : 0) - help_h, win_w, help_h),
            &help);
        help.font = resource_font(font_id);
    }

    if (FOOTER == EFooter::On) {
        status_footer_init(&footer, root_id);
    }
}

void IScreenMenu::Done() {
    window_destroy(root.id);
}

int IScreenMenu::Event(window_t *window, uint8_t event, void *param) {
    if (win.flg & HasFooter_FLAG) {
        status_footer_event(&footer, window, event, param);
    }
    if (win.flg & HasHeaderEvents_FLAG) {
        window_header_events(&header);
    }

    //on return 0 screen_dispatch_event will
    //call window_dispatch_event
    return 0;
}
