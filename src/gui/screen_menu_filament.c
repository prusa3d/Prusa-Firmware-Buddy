// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.h"
#include "filament.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load.h"
#include "window_dlg_unload.h"
#include "window_dlg_purge.h"
#include "dbg.h"

extern screen_t screen_menu_preheat;
extern screen_t screen_preheating;
extern uint8_t menu_preheat_type;

typedef enum {
    MI_RETURN,
    MI_LOAD,
    MI_UNLOAD,
    MI_PURGE,
} MI_t;

const menu_item_t _menu_filament_items[] = {
    { { "Load Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Unload Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Purge Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

void screen_menu_filament_init(screen_t *screen) {
    int count = sizeof(_menu_filament_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "FILAMENT", count + 1, 1, 0);
    psmd->items[0] = menu_item_return;
    memcpy(psmd->items + 1, _menu_filament_items, count * sizeof(menu_item_t));
}

int screen_menu_filament_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case MI_LOAD:
            p_window_header_set_text(&(psmd->header), "LOAD FILAMENT");
            gui_dlg_load();
            p_window_header_set_text(&(psmd->header), "FILAMENT");
            break;
        case MI_UNLOAD:
            p_window_header_set_text(&(psmd->header), "UNLOAD FILAM.");
            gui_dlg_unload();
            p_window_header_set_text(&(psmd->header), "FILAMENT");
            break;
        case MI_PURGE:
            p_window_header_set_text(&(psmd->header), "PURGE FILAM.");
            gui_dlg_purge();
            p_window_header_set_text(&(psmd->header), "FILAMENT");
            break;
        }
    return screen_menu_event(screen, window, event, param);
}

screen_t screen_menu_filament = {
    0,
    0,
    screen_menu_filament_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_filament_event,
    sizeof(screen_menu_data_t), //data_size
    0, //pdata
};

const screen_t *pscreen_menu_filament = &screen_menu_filament;
