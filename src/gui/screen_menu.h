/*
 * screen_menu.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#ifndef SCREEN_MENU_H_
#define SCREEN_MENU_H_

#include "gui.h"
#include "window_header.h"
#include "status_footer.h"
#include "window_menu.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _menu_item_t {
    window_menu_item_t item;
    screen_t *screen;
} menu_item_t;

typedef struct
{
    window_frame_t root;
    window_header_t header;
    window_menu_t menu;
    menu_item_t *items;

    window_text_t *phelp;
    status_footer_t *pfooter;

} screen_menu_data_t;

#pragma pack(pop)

#define SCREEN_MENU_RETURN    (screen_t *)SIZE_MAX
#define SCREEN_MENU_NO_SCREEN NULL
#define psmd                  ((screen_menu_data_t *)screen->pdata)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const menu_item_t menu_item_return;

void screen_menu_init(screen_t *screen,
    const char *label, int count, uint8_t footer, uint8_t help);

void screen_menu_done(screen_t *screen);

int screen_menu_event(screen_t *screen, window_t *window,
    uint8_t event, void *param);

void screen_menu_draw(screen_t *screen);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* SCREEN_MENU_H_ */
