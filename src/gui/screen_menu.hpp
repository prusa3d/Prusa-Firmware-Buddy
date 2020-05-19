#pragma once

#include "gui.h"
#include "window_header.h"
#include "status_footer.h"
#include "window_menu.hpp"

#pragma pack(push)
#pragma pack(1)

struct menu_flags_t {
    uint8_t has_footer : 1;
    uint8_t has_help : 1;
};

struct menu_item_t {
    window_menu_item_t item;
    screen_t *screen;
};

struct screen_menu_data_t {
    window_frame_t root;
    window_header_t header;
    window_menu_t menu;

    menu_item_t *items;

    menu_flags_t flags;
    window_text_t help;
    status_footer_t footer;
};

#pragma pack(pop)

#define SCREEN_MENU_RETURN    (screen_t *)SIZE_MAX
#define SCREEN_MENU_NO_SCREEN NULL
#define psmd                  ((screen_menu_data_t *)screen->pdata)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const menu_item_t menu_item_return;

void screen_menu_init(screen_t *screen, const char *label,
    menu_item_t *p_items, size_t count, uint8_t footer, uint8_t help);

void screen_menu_done(screen_t *screen);

int screen_menu_event(screen_t *screen, window_t *window,
    uint8_t event, void *param);

void screen_menu_draw(screen_t *screen);

#ifdef __cplusplus
}
#endif //__cplusplus
