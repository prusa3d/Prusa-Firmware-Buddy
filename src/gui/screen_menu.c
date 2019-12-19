/*
 * screen_menu.c
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#include "screen_menu.h"
#include "config.h"
#include "stdlib.h"

void window_set_capture(int16_t id);

const menu_item_t menu_item_return = {
    { "Return", IDR_PNG_filescreen_icon_up_folder, WI_LABEL },
    SCREEN_MENU_RETURN
};

void screen_menu_item(window_menu_t *pwindow_menu, uint16_t index,
    window_menu_item_t **ppitem, void *data) {
    screen_t *screen = (screen_t *)data;
    *ppitem = &(psmd->items[index].item);
}

void screen_menu_init(screen_t *screen,
    const char *label, int count, uint8_t footer, uint8_t help) {
    psmd->items = (menu_item_t *)malloc(sizeof(menu_item_t) * count);
    memset(psmd->items, '\0', sizeof(menu_item_t) * count);

    rect_ui16_t menu_rect = rect_ui16(10, 32, 220, 278);
    if (help) {
        menu_rect.h -= 115;
    }
    if (footer) {
        menu_rect.h -= 41;
    }

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(psmd->root));
    window_disable(root);

    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(psmd->header));
    // p_window_header_set_icon(&(psmd->header), IDR_PNG_status_icon_menu);
    p_window_header_set_text(&(psmd->header), label);

    id = window_create_ptr(WINDOW_CLS_MENU, root,
        menu_rect, &(psmd->menu));
    psmd->menu.padding = padding_ui8(20, 6, 2, 6);
    psmd->menu.icon_rect = rect_ui16(0, 0, 16, 30);
    psmd->menu.count = count;
    psmd->menu.menu_items = screen_menu_item;
    psmd->menu.data = (void *)screen;
    //window_set_item_index(id, 1);	// 0 = return
    window_set_capture(id); // set capture to list
    window_set_focus(id);

    if (help) {
        psmd->phelp = (window_text_t *)gui_malloc(sizeof(window_text_t));
        id = window_create_ptr(WINDOW_CLS_TEXT, root,
            (footer) ? rect_ui16(10, 154, 220, 115) : rect_ui16(10, 195, 220, 115),
            psmd->phelp);
        psmd->phelp->font = resource_font(IDR_FNT_SPECIAL);
    } else {
        psmd->phelp = NULL;
    }

    if (footer) {
        psmd->pfooter = (status_footer_t *)gui_malloc(sizeof(status_footer_t));
        status_footer_init(psmd->pfooter, root);
    } else {
        psmd->pfooter = NULL;
    }
}

void screen_menu_done(screen_t *screen) {
    window_destroy(psmd->root.win.id);
    if (psmd->phelp) {
        free(psmd->phelp);
    }

    if (psmd->pfooter) {
        free(psmd->pfooter);
    }
    free(psmd->items);
}

void screen_menu_draw(screen_t *screen) {}

int screen_menu_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    if (psmd->pfooter) {
        status_footer_event(psmd->pfooter, window, event, param);
    }

    window_header_events(&(psmd->header));

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    const menu_item_t *item = &(psmd->items[(int)param]);
    if (!(item->item.type & WI_DISABLED) && item->screen == SCREEN_MENU_RETURN) {
        screen_close();
        return 1;
    }

    if (!(item->item.type & WI_DISABLED) && item->screen != SCREEN_MENU_NO_SCREEN) {
        screen_open(item->screen->id);
        return 1;
    }
    return 0;
}
