#pragma once

#include "gui.h"
#include "window_header.h"
#include "status_footer.h"
#include "window_menu.hpp"
#include "WinMenuContainer.hpp"
#include "WindowMenuItems.hpp"
#include <stdint.h>
#include "resource.h"
#include <new>

enum class EHeader { On,
    Off }; //affect only events
enum class EFooter { On,
    Off };

constexpr static const size_t HelpLines_None = 0;
constexpr static const size_t HelpLines_Default = 4;

template <EHeader HEADER, EFooter FOOTER, size_t HELP_LINES, class... T>
class ScreenMenu : protected window_menu_t {
protected:
    constexpr static const char *no_label = "MISSING";
    window_frame_t root;
    window_header_t header;
    window_text_t help;
    status_footer_t footer;
    WinMenuContainer<T...> container;

public:
    ScreenMenu(const char *label);
    void Done();
    void Draw() {}
    int Event(window_t *window, uint8_t event, void *param);

    //compiletime access by index
    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(container.menu_items);
    }
    //compiletime access by type
    template <class TYPE>
    decltype(auto) Item() {
        return std::get<TYPE>(container.menu_items);
    }

    //C code binding
    static void Create(screen_t *screen, const char *label = no_label) {
        auto *ths = reinterpret_cast<ScreenMenu<HEADER, FOOTER, HELP_LINES, T...> *>(screen->pdata);
        ::new (ths) ScreenMenu<HEADER, FOOTER, HELP_LINES, T...>(label);
    }
    static void CDone(screen_t *screen) {
        reinterpret_cast<ScreenMenu<HEADER, FOOTER, HELP_LINES, T...> *>(screen->pdata)->Done();
    }
    static void CDraw(screen_t *screen) {
        reinterpret_cast<ScreenMenu<HEADER, FOOTER, HELP_LINES, T...> *>(screen->pdata)->Draw();
    }
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
        return reinterpret_cast<ScreenMenu<HEADER, FOOTER, HELP_LINES, T...> *>(screen->pdata)->Event(window, event, param);
    }
};

template <EHeader HEADER, EFooter FOOTER, size_t HELP_LINES, class... T>
ScreenMenu<HEADER, FOOTER, HELP_LINES, T...>::ScreenMenu(const char *label)
    : window_menu_t(nullptr) {
    pContainer = &container;
    GetActiveItem()->SetFocus(); //set focus on new item//containder was not valid during construction, have to set its index again

    //todo bind those numeric constants to fonts and guidefaults
    padding = { 20, 6, 2, 6 };
    icon_rect = rect_ui16(0, 0, 16, 30);
    const uint16_t win_h = 320;
    const uint16_t footer_h = win_h - 269; //269 is smallest number i founs in footer implementation, todo it should be in guidefaults
    const uint16_t help_h = HELP_LINES * (resource_font(IDR_FNT_SPECIAL)->h + gui_defaults.padding.top + gui_defaults.padding.bottom);
    const uint16_t win_x = 10;
    const uint16_t win_w = 240 - 20;

    const uint16_t header_h = gui_defaults.msg_box_sz.y;
    const uint16_t item_h = gui_defaults.font->h + padding.top + padding.bottom;

    const uint16_t menu_rect_h = win_h - help_h - header_h - (FOOTER == EFooter::On ? footer_h : 0);

    const rect_ui16_t menu_rect = rect_ui16(win_x, header_h, win_w, menu_rect_h - menu_rect_h % item_h);

    int16_t id;
    int16_t root_id = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(root));
    window_disable(root_id);

    id = window_create_ptr(WINDOW_CLS_HEADER, root_id,
        rect_ui16(0, 0, 240, 31), &(header));
    // p_window_header_set_icon(&(header), IDR_PNG_status_icon_menu);
    p_window_header_set_text(&(header), label);

    id = window_create_ptr(WINDOW_CLS_MENU, root_id, menu_rect, this);

    win.flg |= WINDOW_FLG_ENABLED;

    window_set_capture(id); // set capture to list
    window_set_focus(id);

    if (HELP_LINES > 0) {
        id = window_create_ptr(WINDOW_CLS_TEXT, root_id,
            rect_ui16(win_x, win_h - (FOOTER == EFooter::On ? footer_h : 0) - help_h, win_w, help_h),
            &help);
        help.font = resource_font(IDR_FNT_SPECIAL);
    }

    if (FOOTER == EFooter::On) {
        status_footer_init(&footer, root_id);
    }
}

template <EHeader HEADER, EFooter FOOTER, size_t HELP_LINES, class... T>
void ScreenMenu<HEADER, FOOTER, HELP_LINES, T...>::Done() {
    window_destroy(root.win.id);
}

template <EHeader HEADER, EFooter FOOTER, size_t HELP_LINES, class... T>
int ScreenMenu<HEADER, FOOTER, HELP_LINES, T...>::Event(window_t *window, uint8_t event, void *param) {
    if (FOOTER == EFooter::On) {
        status_footer_event(&footer, window, event, param);
    }
    if (HEADER == EHeader::On) {
        window_header_events(&header);
    }

    //on return 0 screen_dispatch_event will
    //call window_dispatch_event
    return 0;
}
