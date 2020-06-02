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

#pragma pack(push, 1)

template <bool HEADER, bool FOOTER, bool HELP, class... T>
struct screen_menu_data_t {
    constexpr static const char *no_label = "MISSING";
    window_frame_t root;
    window_header_t header;
    window_text_t help;
    status_footer_t footer;
    WinMenuContainer<T...> container;
    window_menu_t menu;

    screen_menu_data_t(const char *label);
    void Done();
    void Draw() {}
    int Event(window_t *window, uint8_t event, void *param);

    //C code binding
    static void Create(screen_t *screen, const char *label = no_label) {
        auto *ths = reinterpret_cast<screen_menu_data_t<HEADER, FOOTER, HELP, T...> *>(screen->pdata);
        ::new (ths) screen_menu_data_t<HEADER, FOOTER, HELP, T...>(label);
    }
    static void CDone(screen_t *screen) {
        reinterpret_cast<screen_menu_data_t<HEADER, FOOTER, HELP, T...> *>(screen->pdata)->Done();
    }
    static void CDraw(screen_t *screen) {
        reinterpret_cast<screen_menu_data_t<HEADER, FOOTER, HELP, T...> *>(screen->pdata)->Draw();
    }
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
        return reinterpret_cast<screen_menu_data_t<HEADER, FOOTER, HELP, T...> *>(screen->pdata)->Event(window, event, param);
    }
};

#pragma pack(pop)

template <bool HEADER, bool FOOTER, bool HELP, class... T>
screen_menu_data_t<HEADER, FOOTER, HELP, T...>::screen_menu_data_t(const char *label)
    : menu(&container) {
    // menu.pContainer = &container;

    rect_ui16_t menu_rect = rect_ui16(10, 32, 220, 278);
    if (HELP) {
        menu_rect.h -= 115;
    }
    if (FOOTER) {
        menu_rect.h -= 41;
    }

    int16_t id;
    int16_t root_id = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(root));
    window_disable(root_id);

    id = window_create_ptr(WINDOW_CLS_HEADER, root_id,
        rect_ui16(0, 0, 240, 31), &(header));
    // p_window_header_set_icon(&(header), IDR_PNG_status_icon_menu);
    p_window_header_set_text(&(header), label);

    id = window_create_ptr(WINDOW_CLS_MENU, root_id,
        menu_rect, &(menu));
    menu.padding = padding_ui8(20, 6, 2, 6);
    menu.icon_rect = rect_ui16(0, 0, 16, 30);
    menu.win.flg |= WINDOW_FLG_ENABLED;

    //window_set_item_index(id, 1);	// 0 = return
    window_set_capture(id); // set capture to list
    window_set_focus(id);

    if (HELP) {
        id = window_create_ptr(WINDOW_CLS_TEXT, root_id,
            (FOOTER) ? rect_ui16(10, 154, 220, 115) : rect_ui16(10, 195, 220, 115),
            &help);
        help.font = resource_font(IDR_FNT_SPECIAL);
    }

    if (FOOTER) {
        status_footer_init(&footer, root_id);
    }
}

template <bool HEADER, bool FOOTER, bool HELP, class... T>
void screen_menu_data_t<HEADER, FOOTER, HELP, T...>::Done() {
    window_destroy(root.win.id);
}

//helper functions to get Nth element in event at runtime
template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_index_OnClick(int, std::tuple<Tp...> &) {}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if < I<sizeof...(Tp), void>::type
    for_index_OnClick(int index, std::tuple<Tp...> &t) {
    if (index == 0)
        (std::get<I>(t)).OnClick();
    for_index_OnClick<I + 1, Tp...>(index - 1, t);
}

template <bool HEADER, bool FOOTER, bool HELP, class... T>
int screen_menu_data_t<HEADER, FOOTER, HELP, T...>::Event(window_t *window, uint8_t event, void *param) {
    if (FOOTER) {
        status_footer_event(&footer, window, event, param);
    }
    if (HEADER) {
        window_header_events(&header);
    }

    return 0;
    //on return 0 screen_dispatch_event will
    //call window_dispatch_event

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }
#if (0)
    const menu_item_t *item = &(items[(int)param]);
    if (!(!item->item.IsEnabled()) && item->screen == SCREEN_MENU_RETURN) {
        screen_close();
        return 1;
    }

    if (!(!item->item.IsEnabled()) && item->screen != SCREEN_MENU_NO_SCREEN) {
        screen_open(item->screen->id);
        return 1;
    }
#endif
    //return for_index_OnClick((int)param, WinMenuContainer<T...>::menu_items);
}
